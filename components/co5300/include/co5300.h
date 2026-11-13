#pragma once

#include "esp_err.h"
#include <stdint.h>

typedef struct {
    int cs, sck, d0, d1, d2, d3; // broches du bus QSPI
    int frequence_mhz;           // vitresse de l'horloge
    int largeur, hauteur;        // taille de la dalle en pixels
    int decalage_x, decalage_y;  // position de la dalle dans la mémoire du contrôleur
} co5300_config_t;

esp_err_t co5300_init(const co5300_config_t *cfg);
esp_err_t co5300_commande(uint8_t cmd, const uint8_t *donnees, int longueur);
esp_err_t co5300_luminosite(uint8_t niveau);
esp_err_t co5300_remplir(uint16_t couleur);
esp_err_t co5300_rectangle(int x, int y, int largeur, int hauteur, uint16_t couleur);
esp_err_t co5300_dessiner(int x, int y, int largeur, int hauteur, const uint16_t *pixels);