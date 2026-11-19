#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    int annee;          // 2000 à 2099
    int mois;           // 1 à 12
    int jour;           // 1 à 31
    int jour_semaine;   // 0 à 6 (0 = dimanche)
    int heures;         // 0 à 23
    int minutes;        // 0 à 59
    int secondes;       // 0 à 59
} pcf85063_heure_t;

esp_err_t pcf85063_init(void);
esp_err_t pcf85063_lire(pcf85063_heure_t *h, bool *fiable);
esp_err_t pcf85063_regler(const pcf85063_heure_t *h);