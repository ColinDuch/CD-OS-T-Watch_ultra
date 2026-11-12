#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

esp_err_t axp2101_init(i2c_master_bus_handle_t bus);
esp_err_t axp2101_etat_sorties(uint8_t *masque);
esp_err_t axp2101_tension_aldo(int numero, int *mv);
esp_err_t axp2101_tension_batterie(int *mv);