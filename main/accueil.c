#include "accueil.h"
#include "axp2101.h"
#include "esp_timer.h"
#include "lvgl.h"

#define MARGE 20 // zone sûre : marge intérieure de chaque côté
#define BATTERIE_VIDE_MV 3300
#define BATTERIE_PLEINE_MV 4200

static lv_obj_t *s_heure;
static lv_obj_t *s_batterie;

// appelée chauqe seconde par la tâche de LVGL (le verrou est dont déjà pris)
static void mettre_a_jour(lv_timer_t *minuteur)
{
    // l'heure : pour l'instant, le temps écoulé depuis le démarrage
    int temp = (int)(esp_timer_get_time() / 1000000); // microsecondes -> secondes
    int heures = 0;
    int minutes = 0;
    int secondes = 0;

    heures = temp / 3600;
    temp %= 3600;
    minutes = temp / 60;
    temp %= 60;
    secondes = temp;

    lv_label_set_text_fmt(s_heure, "%02d:%02d:%02d", heures, minutes, secondes);

    // la batterie
    int mv = 0;
    if (axp2101_tension_batterie(&mv) == ESP_OK) {
        int pourcentage = 0;

        pourcentage = (mv - BATTERIE_VIDE_MV) * 100 / (BATTERIE_PLEINE_MV - BATTERIE_VIDE_MV);

        if (pourcentage > 100) {
            pourcentage = 100;
        }
        if (pourcentage < 0) {
            pourcentage = 0;
        }

        lv_label_set_text_fmt(s_batterie, "%d %%", pourcentage);
    }
}

void accueil_creer(void)
{
    lv_obj_t *ecran = lv_screen_active();

    // la zone sûre : un conteneur invisible, aussi grand que l'écran, avec une marge intérieure
    lv_obj_t *zone = lv_obj_create(ecran);
    lv_obj_remove_style_all(zone); // ni fond, ni bordure
    lv_obj_set_size(zone, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(zone, MARGE, 0);

    // la batterie, en haut à droite, un peu décalée vers l'intérieur à cause du coin arrondi
    s_batterie = lv_label_create(zone);
    lv_label_set_text(s_batterie, "-- %");
    lv_obj_align(s_batterie, LV_ALIGN_TOP_RIGHT, -40, 0);

    // l'heure, en grand, au centre
    s_heure = lv_label_create(zone);
    lv_obj_set_style_text_font(s_heure, &lv_font_montserrat_48, 0);
    lv_label_set_text(s_heure, "00:00:00");
    lv_obj_center(s_heure);

    // le minuteur : la tâche de LVGL appellera mettre_a_jour toutes les 1000 ms
    lv_timer_t *minuteur = lv_timer_create(mettre_a_jour, 1000, NULL);
    lv_timer_ready(minuteur); // première mise à jour sans attendre une seconde
}