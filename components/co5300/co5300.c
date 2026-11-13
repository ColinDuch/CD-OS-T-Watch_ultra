#include "co5300.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define HOTE_SPI SPI2_HOST   // SPI1 est utilisé par la flash
#define OPCODE_COMMANDE 0x02 // trame de commande, sur 1 fil
#define OPCODE_PIXELS 0x32   // trame de pixels, données sur 4 fils
#define CMD_COLONNES 0x2A
#define CMD_LIGNES 0x2B
#define CMD_ECRIRE_MEMOIRE 0x2C
#define CMD_LUMINOSITE 0x51
#define PIXELS_PAR_MORCEAU 4096 // 8 Ko envoyés à chaque transaction
#define ATTENDRE 0x80           // bit 7 de "longueur" : attendre 120 ms après la commande
#define NB_PASSAGES 1           // LilyGo envoie la séquence 2 fois (voir README)

static const char *TAG = "co5300";
static spi_device_handle_t s_ecran;
static co5300_config_t s_cfg;
static uint16_t *s_tampon; // tampon de transit, en RAM interne accessible au DMA

// séquence d'initialisation de la dalle de 2,06 pouces (LilyGoWatchUltra.cpp)
typedef struct {
    uint8_t cmd;
    uint8_t donnees[4];
    uint8_t longueur; // bits 0 à 4 : nombre d'octets ; bit 7 : ATTENDRE
} commande_init_t;

static const commande_init_t SEQUENCE[] = {
    {0xFE, {0x00}, 1},
    {0xC4, {0x80}, 1},
    {0x3A, {0x55}, 1}, // 16 bits par pixel (RGB565)
    {0x35, {0x00}, 1}, // signal TE actif
    {0x53, {0x20}, 1}, // commande de luminosité active
    {0x63, {0xFF}, 1},
    {0x2A, {0x00, 0x16, 0x01, 0xAF}, 4}, // colonnes 22 à 431
    {0x2B, {0x00, 0x00, 0x01, 0xF5}, 4}, // lignes 0 à 501
    {0x11, {0}, ATTENDRE},               // sortie de veille
    {0x29, {0}, ATTENDRE},               // affichage allumé
    {0x51, {0x00}, 1},                   // luminosité à 0
};
#define NB_COMMANDES (sizeof(SEQUENCE) / sizeof(SEQUENCE[0]))

esp_err_t co5300_commande(uint8_t cmd, const uint8_t *donnees, int longueur)
{
    // trame : [0x02] [0x00 cmd 0x00] [données], tout sur 1 fil
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.cmd = OPCODE_COMMANDE;
    t.addr = (uint32_t)cmd << 8;
    t.length = longueur * 8; // en bits
    t.tx_buffer = longueur > 0 ? donnees : NULL;

    gpio_set_level(s_cfg.cs, 0);
    esp_err_t err = spi_device_polling_transmit(s_ecran, &t);
    gpio_set_level(s_cfg.cs, 1);
    return err;
}

// zone de l'écran où iront les prochains pixels, décalage compris
static esp_err_t fenetre(int x, int y, int largeur, int hauteur)
{
    int x0 = x + s_cfg.decalage_x;
    int x1 = x0 + largeur - 1;
    int y0 = y + s_cfg.decalage_y;
    int y1 = y0 + hauteur - 1;
    uint8_t colonnes[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    uint8_t lignes[4] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    ESP_RETURN_ON_ERROR(co5300_commande(CMD_COLONNES, colonnes, 4), TAG, "colonnes");
    return co5300_commande(CMD_LIGNES, lignes, 4);
}

// envoie les n premiers pixels du tampon de transit ; CS doit déjà être à 0
static esp_err_t envoyer_tampon(int n, bool premier)
{
    spi_transaction_ext_t t;
    memset(&t, 0, sizeof(t));
    t.base.tx_buffer = s_tampon;
    t.base.length = n * 16; // 16 bits par pixel
    if (premier) {
        // trame : [0x32] [0x00 0x2C 0x00] [pixels sur 4 fils]
        t.base.flags = SPI_TRANS_MODE_QIO;
        t.base.cmd = OPCODE_PIXELS;
        t.base.addr = CMD_ECRIRE_MEMOIRE << 8;
    } else {
        // morceaux suivants : seulement des pixels, sans code ni adresse
        t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR |
                       SPI_TRANS_VARIABLE_DUMMY;
    }
    return spi_device_polling_transmit(s_ecran, &t.base);
}

esp_err_t co5300_init(const co5300_config_t *cfg)
{
    s_cfg = *cfg;

    // CS pilotée à la main : elle doit rester à 0 pendant tout l'envoi d'une image
    gpio_config_t broche_cs = {
        .pin_bit_mask = 1ULL << cfg->cs,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&broche_cs), TAG, "broche CS");
    gpio_set_level(cfg->cs, 1);

    // le bus : les fils
    spi_bus_config_t bus = {
        .sclk_io_num = cfg->sck,
        .data0_io_num = cfg->d0,
        .data1_io_num = cfg->d1,
        .data2_io_num = cfg->d2,
        .data3_io_num = cfg->d3,
        .data4_io_num = -1, // -1 = pas de broche (0 serait le GPIO 0 !)
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = PIXELS_PAR_MORCEAU * sizeof(uint16_t),
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(HOTE_SPI, &bus, SPI_DMA_CH_AUTO), TAG, "bus SPI");

    // le périphérique : la vitesse et le format des trames
    spi_device_interface_config_t peripherique = {
        .command_bits = 8,  // le code d'opération
        .address_bits = 24, // l'adresse 0x00 CMD 0x00
        .mode = 0,
        .clock_speed_hz = cfg->frequence_mhz * 1000 * 1000,
        .spics_io_num = -1, // CS gérée à la main
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX, // obligatoire pour les 4 fils
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(HOTE_SPI, &peripherique, &s_ecran), TAG,
                        "périphérique SPI");

    s_tampon = heap_caps_malloc(PIXELS_PAR_MORCEAU * sizeof(uint16_t),
                                MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (s_tampon == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // séquence d'initialisation
    for (int passe = 0; passe < NB_PASSAGES; passe++) {
        for (int i = 0; i < NB_COMMANDES; i++) {
            const commande_init_t *c = &SEQUENCE[i];
            ESP_RETURN_ON_ERROR(co5300_commande(c->cmd, c->donnees, c->longueur & 0x1F), TAG,
                                "commande 0x%02X", c->cmd);
            if (c->longueur & ATTENDRE) {
                vTaskDelay(pdMS_TO_TICKS(120));
            }
        }
    }

    ESP_LOGI(TAG, "Écran initialisé (%dx%d, %d MHz, %d passage(s))", cfg->largeur, cfg->hauteur,
             cfg->frequence_mhz, NB_PASSAGES);
    return ESP_OK;
}

esp_err_t co5300_luminosite(uint8_t niveau) { return co5300_commande(CMD_LUMINOSITE, &niveau, 1); }

esp_err_t co5300_rectangle(int x, int y, int largeur, int hauteur, uint16_t couleur)
{
    // 1. découpage : rogner le rectangle pour qu'il reste dans l'écran
    if (x < 0) {
        largeur += x;
        x = 0;
    }
    if (y < 0) {
        hauteur += y;
        y = 0;
    }
    if (x + largeur > s_cfg.largeur) {
        largeur = s_cfg.largeur - x;
    }
    if (y + hauteur > s_cfg.hauteur) {
        hauteur = s_cfg.hauteur - y;
    }
    if (largeur <= 0 || hauteur <= 0) {
        return ESP_OK; // rien de visible à dessiner
    }

    // 2. la fenêtre
    ESP_RETURN_ON_ERROR(fenetre(x, y, largeur, hauteur), TAG, "fenêtre");

    // 3. un tampon rempli de la couleur, octets inversés, une seule fois
    uint16_t inverse = (uint16_t)((couleur >> 8) | (couleur << 8));
    for (int i = 0; i < PIXELS_PAR_MORCEAU; i++) {
        s_tampon[i] = inverse;
    }

    // 4. envoyer largeur × hauteur pixels, CS maintenue à 0
    int total = largeur * hauteur;
    int envoyes = 0;
    esp_err_t err = ESP_OK;

    gpio_set_level(s_cfg.cs, 0);
    while (envoyes < total && err == ESP_OK) {
        int n = total - envoyes < PIXELS_PAR_MORCEAU ? total - envoyes : PIXELS_PAR_MORCEAU;
        err = envoyer_tampon(n, envoyes == 0);
        envoyes += n;
    }
    gpio_set_level(s_cfg.cs, 1);
    return err;
}

esp_err_t co5300_dessiner(int x, int y, int largeur, int hauteur, const uint16_t *pixels)
{
    // pas de découpage pour une image : on refuse ce qui dépasse de l'écran
    if (x < 0 || y < 0 || largeur <= 0 || hauteur <= 0 || x + largeur > s_cfg.largeur ||
        y + hauteur > s_cfg.hauteur) {
        ESP_LOGE(TAG, "Image hors de l'écran : (%d, %d), %dx%d", x, y, largeur, hauteur);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(fenetre(x, y, largeur, hauteur), TAG, "fenêtre");

    int total = largeur * hauteur;
    int envoyes = 0;
    esp_err_t err = ESP_OK;

    gpio_set_level(s_cfg.cs, 0);
    while (envoyes < total && err == ESP_OK) {
        int n = total - envoyes < PIXELS_PAR_MORCEAU ? total - envoyes : PIXELS_PAR_MORCEAU;

        // recopie du morceau suivant dans le tampon de transit, octets inversés au passage
        for (int i = 0; i < n; i++) {
            uint16_t p = pixels[envoyes + i];
            s_tampon[i] = (uint16_t)((p >> 8) | (p << 8));
        }
        err = envoyer_tampon(n, envoyes == 0);
        envoyes += n;
    }
    gpio_set_level(s_cfg.cs, 1);
    return err;
}

esp_err_t co5300_remplir(uint16_t couleur)
{
    // remplir l'écran, c'est dessiner un rectangle aussi grand que lui
    return co5300_rectangle(0, 0, s_cfg.largeur, s_cfg.hauteur, couleur);
}