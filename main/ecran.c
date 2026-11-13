#include "ecran.h"
#include "co5300.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xl9555.h"

static const char *TAG = "ecran";

esp_err_t ecran_init(void)
{
    // ALDO2 est déjà allumée par l'alimentation_init, on active la dalle par XL9555
    ESP_RETURN_ON_ERROR(xl9555_ecrire(XL9555_ECRAN_EN, true), TAG, "activation écran");
    vTaskDelay(pdMS_TO_TICKS(200)); // laisse l'écran démarrer avant de lui parler

    const co5300_config_t cfg = {
        .cs = 41,
        .sck = 40,
        .d0 = 38,
        .d1 = 39,
        .d2 = 42,
        .d3 = 45,
        .frequence_mhz = 80, // comme LilyGo 
        .largeur = 410,
        .hauteur = 502,
        .decalage_x = 22,
        .decalage_y = 0,
    };
    return co5300_init(&cfg);
}