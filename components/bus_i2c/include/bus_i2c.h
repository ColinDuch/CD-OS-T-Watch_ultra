#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

esp_err_t bus_i2c_init(void);
i2c_master_bus_handle_t bus_i2c_poignee(void); // pour que les futurs pilotes s'y branchent
void bus_i2c_scanner(void);

// accès communs à tous les pilotes : déclarer une puce, lire et écrire plusiuers registres d'un
// coup
esp_err_t bus_i2c_ajouter(uint8_t adresse, i2c_master_dev_handle_t *puce);
esp_err_t bus_i2c_lire(i2c_master_dev_handle_t puce, uint8_t reg, uint8_t *donnees, size_t n);
esp_err_t bus_i2c_ecrire(i2c_master_dev_handle_t puce, uint8_t reg, const uint8_t *donnees, size_t n);