#include "alimentation.h"
#include "axp2101.h"
#include "bus_i2c.h"
#include "co5300.h"
#include "ecran.h"
#include "esp_heap_caps.h" // gestionnaire de mémoire, par capacités
#include "esp_log.h"
#include "esp_psram.h" // fonctions liées à la PSRAM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xl9555.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage de CD-OS (T-Watch Ultra)");

    // mémoire
    ESP_LOGI(TAG, "PSRAM détectée : %zu octets", esp_psram_get_size());
    ESP_LOGI(TAG, "PSRAM libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "RAM interne libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // pilotes
    ESP_ERROR_CHECK(bus_i2c_init());
    ESP_ERROR_CHECK(xl9555_init(bus_i2c_poignee()));
    ESP_ERROR_CHECK(axp2101_init(bus_i2c_poignee()));

    // état d'alimentation connu
    uint8_t sorties = 0;
    ESP_ERROR_CHECK(axp2101_etat_sorties(&sorties));
    ESP_LOGI(TAG, "Sorties LDO héritées : 0x%02X", sorties);

    uint8_t dcdc = 0;
    ESP_ERROR_CHECK(axp2101_etat_dcdc(&dcdc));
    ESP_LOGI(TAG, "DCDC hérités : 0x%02X", dcdc & 0x1F);

    ESP_ERROR_CHECK(alimentation_init());

    ESP_ERROR_CHECK(axp2101_etat_sorties(&sorties));
    ESP_LOGI(TAG, "Sorties LDO imposées : 0x%02X", sorties);

    ESP_ERROR_CHECK(axp2101_etat_dcdc(&dcdc));
    ESP_LOGI(TAG, "DCDC imposés : 0x%02X", dcdc & 0x1F);

    for (axp2101_sortie_t s = AXP2101_ALDO1; s <= AXP2101_BLDO2; s++) {
        int mv = 0;
        ESP_ERROR_CHECK(axp2101_tension(s, &mv));
        ESP_LOGI(TAG, "Sortie %d : %d mV", s, mv);
    }

    bus_i2c_scanner();

    // écran
    ESP_ERROR_CHECK(ecran_init());
    ESP_ERROR_CHECK(co5300_luminosite(100)); // de 0 à 255

    // test des bandes concentriques de 5 pixels (algorithme du peintre)
    static const uint16_t BANDES[] = {
        0xFFFF,   // 0 : blanc
        0xF800,   // 1 : rouge
        0x07E0,   // 2 : vert
        0x001F,   // 3 : bleu
        0xFFE0,   // 4 : jaune
        0xF81F,   // 5 : magenta
        0x07FF,   // 6 : cyan
        0x0000,   // 7 : noir, pour le centre
    };
    const int nb_bandes = sizeof(BANDES) / sizeof(BANDES[0]);
    const int e = 5;    // épaisseur des bandes en pixels

    for (int k = 0; k < nb_bandes; k++) {
        ESP_ERROR_CHECK(co5300_rectangle(k * e,k * e, 410 - 2* k * e, 502 -2 * k * e,BANDES[k]));
    }

    int battement = 0;
    while (1) {
        int batterie = 0;
        axp2101_tension_batterie(&batterie);
        ESP_LOGI(TAG, "Battement %d | batterie : %d mV", battement++, batterie);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}