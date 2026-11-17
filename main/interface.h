#pragma once 

#include "esp_err.h"

// branche LVGL sur l'écran et lance sa tâche, à appeler après écran_init
esp_err_t interface_init(void);

// à prendre avant tout appel à LVGL ou à l'écran depuis une autre tâche 
void interface_verrouiller(void);
void interface_deverrouiller(void);