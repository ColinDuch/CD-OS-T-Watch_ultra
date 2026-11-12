#include "alimentation.h"
#include "axp2101.h"
#include "xl9555.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG ="alimentation";

// tensions de la T-Watch ultra (LilyGoWatchUltra.cpp, vérifiées)
static const struct {
    axp2101_sortie_t sortie;
    int mv;
} TENSIONS[] = {
    {AXP2101_ALDO1, 3300},  // carte SD
    {AXP2101_ALDO2, 3300},  // écran
    {AXP2101_ALDO3, 3300},  // LoRa
    {AXP2101_ALDO4, 1800},  // capteur : jamais plus !
    {AXP2101_BLDO1, 3300},  // GPS
    {AXP2101_BLDO2, 3300},  // haut-parleur
};
#define NB_TENSIONS (sizeof(TENSIONS) / sizeof(TENSIONS[0]))

// d'abord le nivrau, ensuite le passage en sortie : la broche ne passe jamais par un niveau impérvu
static esp_err_t sortie_xl9555(int broche, bool niveau)
{
    ESP_RETURN_ON_ERROR(xl9555_ecrire(broche, niveau), TAG, "niveau broche %d", broche);
    return xl9555_mode(broche, true);
}

esp_err_t alimentation_init(void)
{
    // 1 xl9555 on désactive l'écran et le vibreur, le tactile est maintenu en reset
    ESP_RETURN_ON_ERROR(sortie_xl9555(XL9555_ECRAN_EN, false), TAG, "écran");
    ESP_RETURN_ON_ERROR(sortie_xl9555(XL9555_VIBREUR_EN, false), TAG, "vibreur");
    ESP_RETURN_ON_ERROR(sortie_xl9555(XL9555_TACTILE_RST, false), TAG, "tactile");
    ESP_RETURN_ON_ERROR(sortie_xl9555(XL9555_SD_DETECT, false), TAG, "détection SD");

    // 2 on coupe toutes les sorties LDO, puis on laisse les condensateurs se vider
    for (axp2101_sortie_t s = AXP2101_ALDO1; s <= AXP2101_DLDO1; s++) {
        ESP_RETURN_ON_ERROR(axp2101_activer_sortie(s, false), TAG, "coupure sortie %d", s);
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3 on règle toutes les tensions
    for (int i = 0; i < NB_TENSIONS; i++) {
        ESP_RETURN_ON_ERROR(axp2101_regler_tension(TENSIONS[i].sortie, TENSIONS[i].mv), TAG, "tension sortie %d", TENSIONS[i].sortie);
    }

    // 4 on allume seulement ce dont l'os a besoin 
    ESP_RETURN_ON_ERROR(axp2101_activer_sortie(AXP2101_ALDO2, true), TAG, "allumage écran");

    ESP_LOGI(TAG, "Alimentation dans un état connu");
    return ESP_OK;
}