#pragma once

#include <stdint.h>

// RGB565 : rouge sur 5 bits (15 à 11), vert sur 6 bits (10 à 5), bleu sur 5 bits (4 à 0)
// convertit une couleur "classique" (r, g, b de 0 à 255) en RGB565
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define COULEUR_NOIR RGB565(0, 0, 0)
#define COULEUR_BLANC RGB565(255, 255, 255)
#define COULEUR_GRIS RGB565(128, 128, 128)
#define COULEUR_ROUGE RGB565(255, 0, 0)
#define COULEUR_VERT RGB565(0, 255, 0)
#define COULEUR_BLEU RGB565(0, 0, 255)
#define COULEUR_JAUNE RGB565(255, 255, 0)
#define COULEUR_CYAN RGB565(0, 255, 255)
#define COULEUR_MAGENTA RGB565(255, 0, 255)
#define COULEUR_ORANGE RGB565(255, 128, 0)