#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"     // fonctions liées à la PSRAM
#include "esp_heap_caps.h" // gestionnaire de mémoire, par "capacités"
#include "bus_i2c.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage de CD-OS (T-Watch Ultra)");

    // taille totale de la PSRAM trouvée au démarrage
    ESP_LOGI(TAG, "PSRAM détectée : %zu octets", esp_psram_get_size());

    // mémoire encore libre en PSRAM
    ESP_LOGI(TAG, "PSRAM libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // mémoire encore libre en RAM interne 
    ESP_LOGI(TAG, "RAM interne libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    ESP_ERROR_CHECK(bus_i2c_init());
    bus_i2c_scanner();

    int battement = 0;
    while (1) {
        ESP_LOGI(TAG, "Battement %d", battement++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
