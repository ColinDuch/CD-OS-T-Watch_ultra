#include "xl9555.h"
#include "esp_check.h"
#include "esp_log.h"

#define XL9555_ADRESSE 0x20
#define REG_ENTREE 0x00     // + port : état réel des broches
#define REG_SORTIE 0x02     // + port : niveau imposé aux broches en sortie
#define REG_CONFIG 0x06     // + port : 1 = entrée, 0 = sortie
#define DELAI_MS 100        // attente maximale d'une transaction I2C

static const char *TAG = "xl9555";
static i2c_master_dev_handle_t s_puce;  // "la puce à l'adresse 0x20, sur ce bus"

// lit un registre : on envoie son munéro, puis on reçoit un octet
static esp_err_t lire_registre(uint8_t reg, uint8_t *valeur)
{
    return i2c_master_transmit_receive(s_puce, &reg, 1, valeur, 1, DELAI_MS);
}

// écrit un registre : on envoie son numéro, suivi de la valeur
static esp_err_t ecrire_registre(uint8_t reg, uint8_t valeur)
{
    uint8_t trame[2] = {reg, valeur};
    return i2c_master_transmit(s_puce, trame, sizeof(trame), DELAI_MS);
}

esp_err_t xl9555_init(i2c_master_bus_handle_t bus)
{
    // on déclare la puce sur le bus : ESP-IDF mettra son adresse devant chaque échange
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = XL9555_ADRESSE,
        .scl_speed_hz = 400000,     // 400 kHz : le mode "rapide" de l'I2C
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &cfg, &s_puce), TAG, "ajout sur le bus");

    // première lecture, pour vérifier que la puce répond
    uint8_t config;
    ESP_RETURN_ON_ERROR(lire_registre(REG_CONFIG, &config), TAG, "lecture config");
    ESP_LOGI(TAG, "Configuration du port 0 : 0x%02X", config);
    return ESP_OK;
}

esp_err_t xl9555_mode(int broche, bool sortie)
{
    if (broche < 0 || broche > 15) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = REG_CONFIG + broche / 8;
    int bit = broche % 8;

    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(reg, &valeur), TAG, "lecture config");
    if (sortie) {
        valeur &= ~(1 << bit);  // bit à 0 : sortie
    } else {
        valeur |= (1 << bit);   // bit à 1 : entrée 
    }
    return ecrire_registre(reg, valeur);
}

esp_err_t xl9555_lire(int broche, bool *niveau)
{
    if (broche < 0 || broche > 15) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(REG_ENTREE + broche / 8, &valeur), TAG, "lecture entrées");
    *niveau = (valeur >> (broche % 8)) & 1;
    return ESP_OK;
}

esp_err_t xl9555_ecrire(int broche, bool niveau)
{
    if (broche < 0 || broche > 15) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = REG_SORTIE + broche / 8;
    int bit = broche % 8;

    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(reg, &valeur), TAG, "lecture sortie");

    if (niveau == true) {
        valeur |= (1 << bit);
    } else {
        valeur &= ~(1 << bit);
    }

    return ecrire_registre(reg, valeur);
}