#pragma once 

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// broches de l'expandeur sur la T-Watch Ultra (numérotation 0 à 15, vérifiée par test)
#define XL9555_VIBREUR_EN   6   // LilyGo "GPIO6"  = P0_6
#define XL9555_ECRAN_EN     7   // LilyGo "GPIO7"  = P0_7
#define XL9555_TACTILE_RST  8   // LilyGo "GPIO10" = P1_0
#define XL9555_SD_DETECT   10   // LilyGo "GPIO12" = P1_2, 0 = carte présente

esp_err_t xl9555_init(i2c_master_bus_handle_t bus);
esp_err_t xl9555_mode(int broche, bool sortie);
esp_err_t xl9555_ecrire(int broche, bool niveau);
esp_err_t xl9555_lire(int broche, bool *niveau);