#include "pcf85063.h"
#include "bus_i2c.h"
#include "esp_check.h"
#include "esp_log.h"

#define PCF85063_ADRESSE 0x51
#define REG_CONTROLE_1 0x00
#define REG_SECONDES 0x04  // puis minutes, heures, jours, jours de la semaines, mois, année
#define BIT_STOP (1 << 5)  // registre 0x00 : 1 = horloge arrêt
#define BIT_12_24 (1 << 1) // registre 0x00 : 1 = mode 12H
#define BIT_OS (1 << 7)    // registre 0x04 : 1 = l'horloge s'est arrêtée, heure non fiable

static const char *TAG = "pcf85063";
static i2c_master_dev_handle_t s_puce;

// BCD -> décimal : 0x59 -> 59
static int bcd_vers_decimal(uint8_t v)
{
    return (v >> 4) * 10 + (v & 0x0F);
}

// décimal -> BCD : 59 -> 0x59
static uint8_t decimal_vers_bcd(int n)
{
    return ((n / 10) << 4) | (n % 10);
}

esp_err_t pcf85063_init(void)
{
    ESP_RETURN_ON_ERROR(bus_i2c_ajouter(PCF85063_ADRESSE, &s_puce), TAG, "ajout sur le bus");

    // état connu, horloge en marche et en mode 24H (lire, modifier, réécrire)
    uint8_t controle;
    ESP_RETURN_ON_ERROR(bus_i2c_lire(s_puce, REG_CONTROLE_1, &controle, 1), TAG, "lecture contrôle");
    controle &= ~(BIT_STOP | BIT_12_24);
    ESP_RETURN_ON_ERROR(bus_i2c_ecrire(s_puce, REG_CONTROLE_1, &controle, 1), TAG, "écriture contrôle");

    ESP_LOGI(TAG, "Horloge prête (mode 24H)");
    return ESP_OK;
}

esp_err_t pcf85063_lire(pcf85063_heure_t *h, bool *fiable)
{
    // les 7registres d'un seul coup, la puce garantit qu'ils sont cohérents
    uint8_t r[7];
    ESP_RETURN_ON_ERROR(bus_i2c_lire(s_puce, REG_SECONDES, r, 7), TAG, "lecture de l'heure");

    *fiable = !(r[0] & BIT_OS);
    h->secondes = bcd_vers_decimal(r[0] & 0x7F);    // le masque efface le drapeau OS
    h->minutes = bcd_vers_decimal(r[1] & 0x7F);
    h->heures = bcd_vers_decimal(r[2] & 0x3F);
    h->jour = bcd_vers_decimal(r[3] & 0x3F);
    h->jour_semaine = r[4] & 0x07;      // de 0 à 6, dimanche = 0
    h->mois = bcd_vers_decimal(r[5] & 0x1F);
    h->annee = bcd_vers_decimal(r[6]) + 2000;
    return ESP_OK;
}

esp_err_t pcf85063_regler(const pcf85063_heure_t *h)
{
    if (h->annee < 2000 || h->annee > 2099) {
        return ESP_ERR_INVALID_ARG;
    }
    // le bit 7 des secondes est écrit à 0, ce qui efface le drapeau OS : l'heure redevient fiable
    uint8_t r[7] = {
        decimal_vers_bcd(h->secondes),
        decimal_vers_bcd(h->minutes),
        decimal_vers_bcd(h->heures),
        decimal_vers_bcd(h->jour),
        h->jour_semaine,
        decimal_vers_bcd(h->mois),
        decimal_vers_bcd(h->annee - 2000),
    };
    return bus_i2c_ecrire(s_puce, REG_SECONDES, r, 7);
}