#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"     // fonctions liées à la PSRAM
#include "esp_heap_caps.h" // gestionnaire de mémoire, par "capacités"
#include "driver/gpio.h"
#include "lvgl.h"
#include "bus_i2c.h"
#include "xl9555.h"
#include "axp2101.h"
#include "alimentation.h"
#include "ecran.h"
#include "co5300.h"
#include "interface.h"

static const char *TAG = "main";

#define BROCHE_BOUTON 0              // bouton GPIO 0 : 1 au repos, 0 quand on appuie
#define TOURS_AVANT_EXTINCTION 100   // 100 tours de 50 ms = 5 secondes

static void bouton_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BROCHE_BOUTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // au repos, la broche est tirée vers 1
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

static bool bouton_appuye(void)
{
    return gpio_get_level(BROCHE_BOUTON) == 0;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage de CD-OS (T-Watch Ultra)");

    // mémoire
    ESP_LOGI(TAG, "PSRAM détectée : %zu octets", esp_psram_get_size());
    ESP_LOGI(TAG, "RAM interne libre : %zu octets", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // pilotes
    ESP_ERROR_CHECK(bus_i2c_init());
    ESP_ERROR_CHECK(xl9555_init(bus_i2c_poignee()));
    ESP_ERROR_CHECK(axp2101_init(bus_i2c_poignee()));

    // état d'alimentation connu
    ESP_ERROR_CHECK(alimentation_init());

    // écran
    ESP_ERROR_CHECK(ecran_init());
    ESP_ERROR_CHECK(co5300_luminosite(100));   // de 0 à 255

    // interface
    ESP_ERROR_CHECK(interface_init());
    ESP_LOGI(TAG, "RAM interne libre après LVGL : %zu octets",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // premier test de LVGL : un texte au centre de l'écran
    interface_verrouiller();
    lv_obj_t *texte = lv_label_create(lv_screen_active());
    lv_label_set_text(texte, "CD-OS");
    lv_obj_set_style_text_font(texte, &lv_font_montserrat_48, 0);
    lv_obj_center(texte);
    interface_deverrouiller();

    // veille automatique de l'écran
    bouton_init();
    int compteur = 0;
    bool ecran_allume = true;   // l'écran est allumé au démarrage
    int tour = 0;

    while (1) {
        if (bouton_appuye()) {
            compteur = 0;
            if (!ecran_allume) {
                interface_verrouiller();
                ESP_ERROR_CHECK(co5300_reveil());
                interface_deverrouiller();
                ecran_allume = true;
                ESP_LOGI(TAG, "écran rallumé");
            }
        }

        if (compteur >= TOURS_AVANT_EXTINCTION && ecran_allume) {
            interface_verrouiller();
            ESP_ERROR_CHECK(co5300_veille());
            interface_deverrouiller();
            ecran_allume = false;
            ESP_LOGI(TAG, "écran éteint");
        }
        compteur++;

        // batterie : une fois par seconde, soit un tour sur 20
        if (tour % 20 == 0) {
            int batterie = 0;
            axp2101_tension_batterie(&batterie);
            ESP_LOGI(TAG, "Batterie : %d mV | écran %s", batterie, ecran_allume ? "allumé" : "éteint");
        }
        tour++;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}