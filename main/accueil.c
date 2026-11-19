#include "accueil.h"
#include "axp2101.h"
#include "pcf85063.h"
#include "lvgl.h"

#define MARGE 20 // zone sûre : marge intérieure de chaque côté
#define BATTERIE_VIDE_MV 3300
#define BATTERIE_PLEINE_MV 4200

static lv_obj_t *s_heure;
static lv_obj_t *s_batterie;
static lv_obj_t *s_date;

// appelée chauqe seconde par la tâche de LVGL (le verrou est dont déjà pris)
static void mettre_a_jour(lv_timer_t *minuteur)
{
    // l'heure et la date, lues dans l'horloge
    pcf85063_heure_t h;
    bool fiable = false;
    if (pcf85063_lire(&h, &fiable) == ESP_OK) {
        lv_label_set_text_fmt(s_heure, "%02d:%02d:%02d", h.heures, h.minutes, h.secondes);
        lv_label_set_text_fmt(s_date, "%02d/%02d/%04d", h.jour, h.mois, h.annee);
    }

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


    // la date, juste sous l'heure
    s_date = lv_label_create(zone);
    lv_label_set_text(s_date, "--/--/----");
    lv_obj_align_to(s_date, s_heure, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // le minuteur : la tâche de LVGL appellera mettre_a_jour toutes les 1000 ms
    lv_timer_t *minuteur = lv_timer_create(mettre_a_jour, 1000, NULL);
    lv_timer_ready(minuteur); // première mise à jour sans attendre une seconde
}