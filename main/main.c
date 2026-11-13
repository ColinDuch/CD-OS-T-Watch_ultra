#include "alimentation.h"
#include "axp2101.h"
#include "bus_i2c.h"
#include "co5300.h"
#include "couleurs.h"
#include "esp_timer.h"
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

    // test A : chronométrer le remplissage de l'écran 
    int64_t debut = esp_timer_get_time();
    ESP_ERROR_CHECK(co5300_remplir(COULEUR_NOIR));
    int64_t duree = esp_timer_get_time() - debut;
    ESP_LOGI(TAG, "Remplissage : %lld µs", duree);

    // test B : un dégradé calculé en PSRAM, puis envoyé à l'écran
    const int L = 410;
    const int H = 502;
    uint16_t *image = heap_caps_malloc(L * H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (image == NULL) {
        ESP_LOGE(TAG, "Pas assez de PSRAM pour l'image");
        return;
    }

    for (int y = 0; y < H; y++){
        int b = y * 31 / (H - 1);
        for (int x = 0; x < L; x++){
            int r = x * 31 / (L - 1);
            uint16_t couleur = (r << 11) | b;
            image[y * L +x] = couleur;
        }
    }

    debut = esp_timer_get_time();
    ESP_ERROR_CHECK(co5300_dessiner(0, 0, L, H, image));
    duree = esp_timer_get_time() - debut;
    ESP_LOGI(TAG, "Image : %lld µs", duree);

    int battement = 0;
    while (1) {
        int batterie = 0;
        axp2101_tension_batterie(&batterie);
        ESP_LOGI(TAG, "Battement %d | batterie : %d mV", battement++, batterie);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}