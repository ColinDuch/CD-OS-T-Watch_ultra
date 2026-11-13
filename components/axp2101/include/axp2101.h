#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// les sorties LDO, numérotées comme leurs bits dans le registre 0x90
typedef enum {
    AXP2101_ALDO1 = 0, // carte SD
    AXP2101_ALDO2,     // écran
    AXP2101_ALDO3,     // LoRa
    AXP2101_ALDO4,     // capteur : 1800 mV, jamais plus !
    AXP2101_BLDO1,     // GPS
    AXP2101_BLDO2,     // haut-parleur
    AXP2101_CPUSLDO,   // inutilisée
    AXP2101_DLDO1,     // NFC
} axp2101_sortie_t;

esp_err_t axp2101_init(i2c_master_bus_handle_t bus);
esp_err_t axp2101_etat_sorties(uint8_t *masque);
esp_err_t axp2101_tension(axp2101_sortie_t sortie, int *mv);
esp_err_t axp2101_regler_tension(axp2101_sortie_t sortie, int mv);
esp_err_t axp2101_activer_sortie(axp2101_sortie_t sortie, bool allumer);
esp_err_t axp2101_tension_batterie(int *mv);

// les convertisseurs DCDC, numérotés comme leurs bits dans le registre 0x80
typedef enum {
    AXP2101_DCDC1 = 0, // ESP32-S3 : ne jamais couper !
    AXP2101_DCDC2,     // inutilisé
    AXP2101_DCDC3,     // inutilisé
    AXP2101_DCDC4,     // inutilisé
    AXP2101_DCDC5,     // inutilisé
} axp2101_dcdc_t;

esp_err_t axp2101_etat_dcdc(uint8_t *masque);
esp_err_t axp2101_activer_dcdc(axp2101_dcdc_t dcdc, bool allumer);