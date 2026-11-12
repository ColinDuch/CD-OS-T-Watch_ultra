#pragma once

#include "esp_err.h"

// met la montre dans une état d'alimentation connu, à appeler après les init des composants 
esp_err_t alimentation_init(void);