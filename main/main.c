#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"      // fonctions liées à la PSRAM
#include "esp_heap_caps.h"  // gestionnaire de mémoire, par capacités
#include "bus_i2c.h"
#include "xl9555.h"
#include "axp2101.h"
#include "alimentation.h"

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

    int battement = 0;
    while (1) {
        int batterie = 0;
        axp2101_tension_batterie(&batterie);
        ESP_LOGI(TAG, "Battements %d | batterie : %d mV", battement++, batterie);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}