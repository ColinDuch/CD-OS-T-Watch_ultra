#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"     // fonctions liées à la PSRAM
#include "esp_heap_caps.h" // gestionnaire de mémoire, par "capacités"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage de CD-OS (T-Watch Ultra)");

    // taille totale de la PSRAM trouvée au démarrage
    ESP_LOGI(TAG, "PSRAM détectée : %zu octets", esp_psram_get_size());

    // mémoire encore libre en PSRAM
    ESP_LOGI(TAG, "PSRAM libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // À TOI : afficher la RAM interne libre
    // (même fonction, autre capacité : Ctrl+clic sur MALLOC_CAP_SPIRAM pour voir la liste)

    int battement = 0;
    while (1) {
        ESP_LOGI(TAG, "Battement %d", battement++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
