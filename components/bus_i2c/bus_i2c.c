#include "bus_i2c.h"
#include "esp_log.h"
#include <string.h>

#define PIN_SDA 3
#define PIN_SCL 2

static const char *TAG = "bus_i2c";
static i2c_master_bus_handle_t s_bus;

esp_err_t bus_i2c_init(void)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0, // l'ESP32-S3 a deux contrôleurs I2C, on prend le 0
        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,               // filtre les parasites très courts sur les fils
        .flags.enable_internal_pullup = true, // résistances de tirage internes, en renfort
    };
    return i2c_new_master_bus(&cfg, &s_bus);
}

i2c_master_bus_handle_t bus_i2c_poignee(void) { return s_bus; }

void bus_i2c_scanner(void)
{
    ESP_LOGI(TAG, "Scan du bus I2C...");
    int trouves = 0;

    for (int adresse = 0x08; adresse <= 0x77; adresse++) {
        if (i2c_master_probe(s_bus, adresse, 50) == ESP_OK) {
            ESP_LOGI(TAG, "puce trouvé à l'adresse 0x%02x", adresse);
            trouves++;
        }
    }

    ESP_LOGI(TAG, "%d puce(s) trouvée(s)", trouves);
}

#define DELAI_MS 100  // attente maximale d'une trnasaction I2C
#define MAX_OCTETS 16 // nb max d'octets écrits en une fois

esp_err_t bus_i2c_ajouter(uint8_t adresse, i2c_master_dev_handle_t *puce)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = adresse,
        .scl_speed_hz = 400000,
    };
    return i2c_master_bus_add_device(s_bus, &cfg, puce);
}

// lit n registres consécutifs, à partir de "reg", la puce avance d'elle-même à chauqe octet
esp_err_t bus_i2c_lire(i2c_master_dev_handle_t puce, uint8_t reg, uint8_t *donnees, size_t n)
{
    return i2c_master_transmit_receive(puce, &reg, 1, donnees, n, DELAI_MS);
}

// écrit n registres consécutifs, la trame est [reg, donnée 1, donnée 2, ...]
esp_err_t bus_i2c_ecrire(i2c_master_dev_handle_t puce, uint8_t reg, const uint8_t *donnees,
                         size_t n)
{
    if (n > MAX_OCTETS) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t trame[MAX_OCTETS + 1];
    trame[0] = reg;
    memcpy(&trame[1], donnees, n); // recopie les n octets juste après le numéro du registre
    return i2c_master_transmit(puce, trame, n + 1, DELAI_MS);
}
