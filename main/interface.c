#include "interface.h"
#include "co5300.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#define LARGEUR 410
#define HAUTEUR 502
#define LIGNES_PAR_TAMPON 50 // 410 x 50 x 2 octets = 41Ko par tampon

static const char *TAG = "interface";
static SemaphoreHandle_t s_verrou;
static lv_display_t *s_affichage;

// l'heure de LVGL, en millisecondes
static uint32_t heure_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void arrondir(lv_event_t *e)
{
    lv_area_t *zone = lv_event_get_param(e);

    zone->x1 &= ~1;
    zone->y1 &= ~1;
    zone->x2 |= 1;
    zone->y2 |= 1;
}

// vidage : LVGL a fini de dessiner une zone, on l'envoie à l'écran
static void vider(lv_display_t *affichage, const lv_area_t *zone, uint8_t *pixels)
{
    int largeur = lv_area_get_width(zone);
    int hauteur = lv_area_get_height(zone);

    // inverser les octets directement dans le brouillon de LVGL
    lv_draw_sw_rgb565_swap(pixels, largeur * hauteur);

    esp_err_t err = co5300_envoyer(zone->x1, zone->y1, largeur, hauteur, (uint16_t *)pixels);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Envoie de la zone impossible (%s)", esp_err_to_name(err));
    }

    // prévenir LVGL que le tampon est de nouveau libre
    lv_display_flush_ready(affichage);
}

// le battement de LVGL : animations, entrées, redissins
static void tache_lvgl(void *arg)
{
    while (1) {
        interface_verrouiller();
        uint32_t attente = lv_timer_handler(); // LVGL dit dans combien de ms il veut être rappeler
        interface_deverrouiller();

        if (attente > 50) {
            attente = 50;
        }

        if (attente < 5) {
            attente = 5;
        }
        vTaskDelay(pdMS_TO_TICKS(attente));
    }
}

void interface_verrouiller(void)
{
    xSemaphoreTake(s_verrou, portMAX_DELAY);
}

void interface_deverrouiller(void)
{
    xSemaphoreGive(s_verrou);
}

esp_err_t interface_init(void)
{
    s_verrou = xSemaphoreCreateMutex();
    if (s_verrou == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_init();
    lv_tick_set_cb(heure_ms);

    s_affichage = lv_display_create(LARGEUR, HAUTEUR);
    lv_display_set_color_format(s_affichage, LV_COLOR_FORMAT_RGB565);

    // deux brouillons en ram interne lisible par le DMA : pas de recopie avant l'envoie 
    size_t taille = LARGEUR * LIGNES_PAR_TAMPON * sizeof(uint16_t);
    void *tampon1 = heap_caps_malloc(taille, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *tampon2 = heap_caps_malloc(taille, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (tampon1 == NULL || tampon2 == NULL) {
        return ESP_ERR_NO_MEM;
    }
    lv_display_set_buffers(s_affichage, tampon1, tampon2, taille, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_display_set_flush_cb(s_affichage, vider);
    lv_display_add_event_cb(s_affichage, arrondir, LV_EVENT_INVALIDATE_AREA, NULL);

    // la tâche est créée en dernier, une fois tout prêt
    xTaskCreate(tache_lvgl, "lvgl", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "LVGL prêt (2 tampons de %u octets)", (unsigned)taille);
    return ESP_OK;
}