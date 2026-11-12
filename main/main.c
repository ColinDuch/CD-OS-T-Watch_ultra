#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"     // fonctions liées à la PSRAM
#include "esp_heap_caps.h" // gestionnaire de mémoire, par "capacités"
#include "bus_i2c.h"
#include "xl9555.h"
#include "axp2101.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage de CD-OS (T-Watch Ultra)");

    // mémoire
    ESP_LOGI(TAG, "PSRAM détectée : %zu octets", esp_psram_get_size());
    ESP_LOGI(TAG, "PSRAM libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "RAM interne libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // bus I2C : initialisation, puis un scan
    ESP_ERROR_CHECK(bus_i2c_init());
    bus_i2c_scanner();

    // expandeur XL9555
    ESP_ERROR_CHECK(xl9555_init(bus_i2c_poignee()));

    // gestion d'alimentation AXP2101
    ESP_ERROR_CHECK(axp2101_init(bus_i2c_poignee()));

    // état des sorties avant le test
    uint8_t sorties = 0;
    ESP_ERROR_CHECK(axp2101_etat_sorties(&sorties));
    ESP_LOGI(TAG, "Sorties LDO avant : 0x%02X", sorties);

    // le capteur : d'abord la tension, ensuite l'allumage
    ESP_ERROR_CHECK(axp2101_regler_tension(AXP2101_ALDO4, 1800));
    ESP_ERROR_CHECK(axp2101_activer_sortie(AXP2101_ALDO4, true));
    vTaskDelay(pdMS_TO_TICKS(50));   // laisse au capteur le temps de démarrer

    // état des sorties après le test
    ESP_ERROR_CHECK(axp2101_etat_sorties(&sorties));
    ESP_LOGI(TAG, "Sorties LDO après : 0x%02X", sorties);

    // tension de chaque sortie réglable
    for (axp2101_sortie_t s = AXP2101_ALDO1; s <= AXP2101_BLDO2; s++) {
        int mv = 0;
        ESP_ERROR_CHECK(axp2101_tension(s, &mv));
        ESP_LOGI(TAG, "Sortie %d : %d mV", s, mv);
    }

    // le capteur doit être revenu sur le bus
    bus_i2c_scanner();

    int battement = 0;
    while (1) {
        int batterie = 0;
        axp2101_tension_batterie(&batterie);
        ESP_LOGI(TAG, "Battement %d | batterie : %d mV", battement++, batterie);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}