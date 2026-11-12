# CD-OS-T-Watch_ultra

Un système d’exploitation fait maison pour la montre LilyGo T-Watch Ultra (ESP32-S3), écrit en C avec ESP-IDF et FreeRTOS.

Comme CD-OS sur le Cardputer, le projet est construit étape par étape, dans un but d’apprentissage : chaque brique est écrite et comprise avant de passer à la suivante. Tous les pilotes des puces de la montre sont écrits à la main ; le code de LilyGo sert uniquement de référence de lecture.

## Fonctionnalités actuelles

- Démarrage avec la flash de 16 Mo et la PSRAM de 8 Mo correctement configurées.
- Affichage de la PSRAM détectée et de la mémoire libre, puis un battement par seconde dans le moniteur.

## Prérequis

- ESP-IDF v5.4, installé dans `~/esp/esp-idf`
- Sous Arch : être dans le groupe `uucp` pour accéder à `/dev/ttyACM0`

Charger l’environnement ESP-IDF dans chaque nouveau terminal :

```
get_idf
```

## Compiler et lancer

```
idf.py set-target esp32s3              # une seule fois, après avoir créé sdkconfig.defaults
idf.py build                           # compiler
idf.py -p /dev/ttyACM0 flash monitor   # flasher puis afficher les messages
idf.py menuconfig                      # modifier la configuration (sdkconfig)
```

Quitter le moniteur : `Ctrl + T` puis `Ctrl + X`.

Si le flash échoue : maintenir le bouton GPIO0 enfoncé, appuyer sur RESET, relâcher GPIO0, puis relancer le flash. Appuyer ensuite sur RESET pour démarrer le programme.

## Structure

```
CD-OS-T-Watch_ultra/
├── CMakeLists.txt          # projet ESP-IDF
├── sdkconfig.defaults      # réglages de la montre (flash, PSRAM)
├── sdkconfig               # configuration complète (générée, à conserver)
├── .clang-format           # style du code (le même que CD-OS)
└── main/
    ├── CMakeLists.txt      # sources du composant principal
    └── main.c              # app_main
```

## Feuille de route

- [x] **Étape 1 : projet ESP-IDF.** Flash 16 Mo, PSRAM 8 Mo détectée, journal.
- [x] **Étape 2 : bus I2C.** Initialisation et scan des adresses (6 puces attendues).
- [x] **Étape 3 : pilote XL9555.** Expandeur de GPIO (alimentation de l’écran, reset du tactile).
- [ ] **Étape 4 : pilote AXP2101 (minimal).** Allumer ALDO2 (écran), lire la batterie.
- [ ] **Étape 4 bis : Remise à zéro.** Remise à zéro électronique des périphériques au démarrage (coupure/rallumage des sorties de l'AXP2101).
- [ ] **Étape 5 : pilote d’écran CO5300.** QSPI, séquence d’initialisation, premiers pixels.
- [ ] **Étape 6 : LVGL.** Reprise de l’interface de CD-OS, tampons en PSRAM.
- [ ] **Étape 7 : pilote tactile CST9217.** Entrée de type pointeur pour LVGL.
- [ ] Ensuite : RTC, vibreur, veille, bus d’événements (repris de CD-OS), puis GPS, son, carte SD.
- [ ] Plus tard : capteur BHI260AP, NFC, LoRa (pilotes C officiels des fabricants + couche d’adaptation).

## Notes matérielles (T-Watch Ultra)

Source : `docs/hardware/lilygo-t-watch-ultra.md` du dépôt LilyGoLib, et la définition `twatch_ultra` du `boards.txt` du cœur Arduino ESP32.

**Mémoire :** flash externe 16 Mo (QSPI, mode DIO), PSRAM externe 8 Mo (QSPI). Ni l’une ni l’autre n’est intégrée à la puce. Avec la PSRAM en mode octal (OPI), la montre se bloque à l’initialisation de la mémoire (`tlsf_create`).

**USB :** USB-Serial/JTAG intégré à l’ESP32-S3 (`303A:1001`), apparaît en `/dev/ttyACM0`.

| Fonction | Broche |
|---|---|
| I2C partagé : SDA / SCL | 3 / 2 |
| SPI partagé (SD, NFC, LoRa) : MOSI / MISO / SCK | 34 / 33 / 35 |
| Écran CO5300 (QSPI) : CS / SCK / D0 / D1 / D2 / D3 | 41 / 40 / 38 / 39 / 42 / 45 |
| Écran : TE / RESET | 6 / 37 |
| Interruptions : RTC / NFC / AXP2101 / BHI260 | 1 / 5 / 7 / 8 |
| Carte SD : CS | 21 |
| NFC : CS | 4 |
| LoRa : CS / RESET / BUSY / IRQ | 36 / 47 / 48 / 14 |
| GPS : TX / RX / PPS | 43 / 44 / 13 |
| Ampli audio MAX98357A : BCLK / WCLK / DOUT | 9 / 10 / 11 |
| Bouton utilisateur / mode téléchargement | 0 |

**Adresses I2C :** tactile CST9217 `0x1A`, expandeur XL9555 `0x20`, capteur BHI260AP `0x28`, AXP2101 `0x34`, RTC PCF85063A `0x51`, vibreur DRV2605 `0x5A`.

**Broches de l’expandeur XL9555 :** 6 = activation du vibreur, 7 = alimentation de l’écran, 8 = reset du tactile, 10 = détection de la carte SD.

**Sorties de l’AXP2101 :**

| Sortie | Alimente |
|---|---|
| DC1 | ESP32-S3 |
| ALDO1 | Carte SD |
| ALDO2 | Écran |
| ALDO3 | LoRa |
| ALDO4 | Capteur |
| BLDO1 | GPS |
| BLDO2 | Haut-parleur |
| DLDO1 | NFC |
| LDO1 (VRTC) | Sauvegarde du GPS (toujours allumée) |
| VBACKUP | Pile du RTC |

**Écran :** AMOLED 2,06 pouces, 410 × 502, contrôleur CO5300 en QSPI. Une image complète en RGB565 pèse environ 400 Ko : elle ne tient qu’en PSRAM.

**Batterie :** 1100 mAh. Garder un courant de charge inférieur à 500 mA pour éviter que l’AXP2101 ne chauffe.

## Références

- [Guide de programmation ESP-IDF (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/index.html)
- [LilyGoLib](https://github.com/Xinyuan-LilyGO/LilyGoLib) : fiche matérielle et code de référence
- [Variante Arduino `lilygo_twatch_ultra`](https://github.com/espressif/arduino-esp32/tree/master/variants/lilygo_twatch_ultra) : brochage officiel
