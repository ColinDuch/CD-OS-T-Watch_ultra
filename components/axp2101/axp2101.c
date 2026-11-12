#include "axp2101.h"
#include "esp_check.h"
#include "esp_log.h"

#define AXP2101_ADRESSE 0x34
#define AXP2101_ID 0x4A

#define REG_ID 0x03
#define REG_ADC_ACTIF 0x30
#define REG_VBAT_HAUT 0x34
#define REG_VBAT_BAS 0x35
#define REG_SORTIES_LDO 0x90
#define REG_TENSION_ALDO1 0x92   // ALDO2 = 0x93, ALDO3 = 0x94, ALDO4 = 0x95
#define DELAI_MS 100

static const char *TAG = "axp2101";
static i2c_master_dev_handle_t s_puce;

// mêmes fonctions que dans xl6555.c : àa mettre en commum plus tard
static esp_err_t lire_registre(uint8_t reg, uint8_t *valeur)
{
    return i2c_master_transmit_receive(s_puce, &reg, 1, valeur, 1, DELAI_MS);
}

static esp_err_t ecrire_registre(uint8_t reg, uint8_t valeur)
{
    uint8_t trame[2] = {reg, valeur};
    return i2c_master_transmit(s_puce, trame, sizeof(trame), DELAI_MS);
}

esp_err_t axp2101_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AXP2101_ADRESSE,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &cfg, &s_puce), TAG, "ajout sur le bus");

    // 1. vérifier qu'on parle bien à un AXP2101
    uint8_t id;
    ESP_RETURN_ON_ERROR(lire_registre(REG_ID, &id), TAG, "lecture identifiant");
    if (id != AXP2101_ID) {
        ESP_LOGE(TAG, "Identifiant inattendu : 0x%02X (attendu 0x%02X)", id, AXP2101_ID);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "AXP2101 détecté (identifiant 0x%02X)", id);

    // 2. activer la mesure de la tension batterie (bit 0), sans toucher aux autres mesures
    uint8_t adc;
    ESP_RETURN_ON_ERROR(lire_registre(REG_ADC_ACTIF, &adc), TAG, "lecture ADC");
    adc |= (1 << 0);
    return ecrire_registre(REG_ADC_ACTIF, adc);
}

esp_err_t axp2101_etat_sorties(uint8_t *masque)
{
    return lire_registre(REG_SORTIES_LDO, masque);
}

esp_err_t axp2101_tension_aldo(int numero, int *mv)
{
    if (numero < 1 || numero > 4) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(REG_TENSION_ALDO1 + numero - 1, &valeur), TAG, "lecture tension");
    *mv = 500 + (valeur & 0x1F) * 100;   // 0x1F = 0b00011111 : on garde les 5 bits de poids faible
    return ESP_OK;
}

esp_err_t axp2101_tension_batterie(int *mv)
{
    uint8_t haut;
    uint8_t bas;

    ESP_RETURN_ON_ERROR(lire_registre(REG_VBAT_HAUT, &haut), TAG, "lecture VBAT haut");
    ESP_RETURN_ON_ERROR(lire_registre(REG_VBAT_BAS, &bas), TAG, "lecture VBAT bas");

    *mv = ((haut & 0x1F) << 8) | bas;

    return ESP_OK;
}