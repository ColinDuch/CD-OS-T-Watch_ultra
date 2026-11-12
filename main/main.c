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

    // test 1 : écrire sur la broche 6, puis relire son état réel
    ESP_ERROR_CHECK(xl9555_mode(6, true));
    bool niveau = false;

    ESP_ERROR_CHECK(xl9555_ecrire(6, false));
    ESP_ERROR_CHECK(xl9555_lire(6, &niveau));
    ESP_LOGI(TAG, "Broche 6 après écriture de 0 : %d", niveau);

    ESP_ERROR_CHECK(xl9555_ecrire(6, true));
    ESP_ERROR_CHECK(xl9555_lire(6, &niveau));
    ESP_LOGI(TAG, "Broche 6 après écriture de 1 : %d", niveau);

    ESP_ERROR_CHECK(axp2101_init(bus_i2c_poignee()));

    uint8_t sorties = 0;
    ESP_ERROR_CHECK(axp2101_etat_sorties(&sorties));
    ESP_LOGI(TAG, "Sorties LDO allumées : 0x%02X", sorties);

    for (int n = 1; n <= 4; n++) {
        int mv = 0;
        ESP_ERROR_CHECK(axp2101_tension_aldo(n, &mv));
        ESP_LOGI(TAG, "ALDO%d : %d mV", n, mv);
    }

    int battement = 0;
    while (1) {
        uint16_t etats = 0;
        for (int broche = 0; broche < 16; broche++) {
            bool n = false;
            xl9555_lire(broche, &n);
            if (n) {
                etats |= (1 << broche);   // range l'état de chaque broche dans son bit
            }
        }
        int batterie = 0;
        axp2101_tension_batterie(&batterie);
        ESP_LOGI(TAG, "Battement %d | batterie : %d mV", battement++, batterie);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}