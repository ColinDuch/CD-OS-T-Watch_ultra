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
#define REG_SORTIES_DCDC 0x80   // bits 0 à 4 = DCDC1 à DCDC5, bits 5 et 6 autres fonctions  
#define REG_TENSION_LDO 0x92    // ALDO1, puis les autres sorties dans l'ordre de l'énumération
#define TENSION_MIN_MV 500
#define TENSION_MAX_MV 3500
#define TENSION_PAS_MV 100
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

static bool tension_reglable(axp2101_sortie_t sortie) 
{
    return sortie >= AXP2101_ALDO1 && sortie <= AXP2101_BLDO2;
}

esp_err_t axp2101_tension(axp2101_sortie_t sortie, int *mv)
{
    if (!tension_reglable(sortie)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(REG_TENSION_LDO + sortie, &valeur), TAG, "lecture tension");
    *mv = TENSION_MIN_MV + (valeur & 0x1F) * TENSION_PAS_MV;
    return ESP_OK;
}

esp_err_t axp2101_regler_tension(axp2101_sortie_t sortie, int mv) 
{
    if (!tension_reglable(sortie)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    // refuser toute tension hors limites, ou qui ne tombe pas sur un pas de 100mV
    if (mv < TENSION_MIN_MV || mv > TENSION_MAX_MV || (mv - TENSION_MIN_MV) % TENSION_PAS_MV != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = REG_TENSION_LDO + sortie;
    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(reg, &valeur), TAG, "lecture tension");

    // garder les 3 bits du haut (0xE0), remplacer les 5 bits du bas par la nouvelle tension
    valeur = (valeur & 0xE0) | ((mv - TENSION_MIN_MV) / TENSION_PAS_MV);

    ESP_RETURN_ON_ERROR(ecrire_registre(reg, valeur), TAG, "écriture tension");

    // relire le registre pour vérifier que la puce a bien prise la nouvelle tension
    uint8_t relu;
    ESP_RETURN_ON_ERROR(lire_registre(reg, &relu), TAG, "relcture tension");
    if (relu != valeur) {
        ESP_LOGE(TAG, "Tension non appliquée (registre 0x%02X) : écrire 0x%02X, relu 0x%02X", reg, valeur, relu);
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

esp_err_t axp2101_activer_sortie(axp2101_sortie_t sortie, bool allumer)
{
    if (sortie < AXP2101_ALDO1 || sortie > AXP2101_DLDO1) {
        return ESP_ERR_INVALID_ARG;
    }

    // un seul registre pour les 8 interrupteurs : le bit numéro "sortie"
    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(REG_SORTIES_LDO, &valeur), TAG, "lecture sorties");

    if (allumer) {
        valeur |= (1 << sortie);
    } else {
        valeur &= ~(1 << sortie);
    }

    return ecrire_registre(REG_SORTIES_LDO, valeur);
}

esp_err_t axp2101_etat_dcdc(uint8_t *masque)
{
    return lire_registre(REG_SORTIES_DCDC, masque);
}

esp_err_t axp2101_activer_dcdc(axp2101_dcdc_t dcdc, bool allumer)
{
    // garde fou 1 : DCDC1 alimente l'ESP32, donc on refuse d'y toucher peu import le sens 
    if (dcdc == AXP2101_DCDC1) {
        ESP_LOGE(TAG, "Refus : DCDC1 alimente l'ESP32");
        return ESP_ERR_NOT_ALLOWED;
    }
    if (dcdc < AXP2101_DCDC2 || dcdc > AXP2101_DCDC5) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t valeur;
    ESP_RETURN_ON_ERROR(lire_registre(REG_SORTIES_DCDC, &valeur), TAG, "lecture DCDC");

    // garde fou 2 : si le programme tourne, DCDC1 est forcément allumé.
    // Lire un 0 voudrait dire que la lecture est faussé, donc on ne la réécrit surtout pas ça couperait l'ESP32
    if (!(valeur & (1 << AXP2101_DCDC1))) {
        ESP_LOGE(TAG, "Lecture incohérente du registre DCDC (0x%02X) : écriture annulée", valeur);
        return ESP_ERR_INVALID_STATE;
    }

    if (allumer) {
        valeur |= (1 << dcdc);
    } else {
        valeur &= ~(1 << dcdc);
    }
    return ecrire_registre(REG_SORTIES_DCDC, valeur);
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