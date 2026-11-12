#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

esp_err_t bus_i2c_init(void);
i2c_master_bus_handle_t bus_i2c_poignee(void);   // pour que les futurs pilotes s'y branchent
void bus_i2c_scanner(void);