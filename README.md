# CD-OS-T-Watch_ultra

Un système d’exploitation fait maison pour la montre LilyGo T-Watch Ultra (ESP32-S3), écrit en C avec ESP-IDF et FreeRTOS.

Comme CD-OS sur le Cardputer, le projet est construit étape par étape, dans un but d’apprentissage : chaque brique est écrite et comprise avant de passer à la suivante. Tous les pilotes des puces de la montre sont écrits à la main ; le code de LilyGo sert uniquement de référence de lecture.

## Fonctionnalités actuelles

- **Mémoire :** flash de 16 Mo et PSRAM de 8 Mo correctement configurées, affichage de la mémoire libre (PSRAM et RAM interne).
- **Bus I2C** (SDA 3, SCL 2) : initialisation et scan des adresses.
- **Pilote XL9555** (expandeur de GPIO) : configuration des broches en entrée ou en sortie, lecture et écriture d’une broche.
- **Pilote AXP2101** (gestion de l’alimentation) : vérification de l’identifiant, lecture de l’état des sorties et de leurs tensions, mesure de la tension de la batterie.
- Un battement par seconde dans le moniteur, avec la tension de la batterie.

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
idf.py fullclean                       # effacer build/ (garde sdkconfig), après ajout d'un composant
```

Quitter le moniteur : `Ctrl + T` puis `Ctrl + X` (`Ctrl + ]` ne fonctionne pas avec un clavier AZERTY).

Si le flash échoue : maintenir le bouton GPIO0 enfoncé, appuyer sur RESET, relâcher GPIO0, puis relancer le flash. Appuyer ensuite sur RESET pour démarrer le programme.

Sans `set-target`, ESP-IDF compile pour l’ESP32 classique et le flash échoue (`--chip esp32` dans le message d’erreur).

## Structure

```
CD-OS-T-Watch_ultra/
├── CMakeLists.txt              # projet ESP-IDF
├── sdkconfig.defaults          # réglages de la montre (flash, PSRAM)
├── sdkconfig                   # configuration complète (générée, à conserver)
├── .clang-format               # style du code (le même que CD-OS)
├── main/
│   ├── CMakeLists.txt          # sources du composant principal
│   └── main.c                  # app_main : assemble les pilotes, tests
└── components/
    ├── bus_i2c/
    │   ├── CMakeLists.txt      # dépend de esp_driver_i2c
    │   ├── include/
    │   │   └── bus_i2c.h       # init, poignée du bus, scan
    │   └── bus_i2c.c
    ├── xl9555/
    │   ├── CMakeLists.txt      # dépend de esp_driver_i2c
    │   ├── include/
    │   │   └── xl9555.h        # mode, lecture, écriture d'une broche + noms des broches
    │   └── xl9555.c
    └── axp2101/
        ├── CMakeLists.txt      # dépend de esp_driver_i2c
        ├── include/
        │   └── axp2101.h       # sorties, tensions, batterie
        └── axp2101.c
```

Chaque pilote est un composant ESP-IDF : son dossier `include/` contient la partie publique (ce que les autres fichiers peuvent utiliser), son `.c` la partie privée (registres, fonctions internes). Le `CMakeLists.txt` de chaque composant déclare ses dépendances avec `REQUIRES`.

## Feuille de route

- [x] **Étape 1 : projet ESP-IDF.** Flash 16 Mo, PSRAM 8 Mo détectée, journal.
- [x] **Étape 2 : bus I2C.** Initialisation et scan des adresses (6 puces).
- [x] **Étape 3 : pilote XL9555.** Expandeur de GPIO (alimentation de l’écran, reset du tactile).
- [ ] **Étape 4 : pilote AXP2101 (minimal).**
  - [x] Partie 1 : identifiant, état des sorties, tensions, batterie.
  - [x] Réorganisation : pilotes en composants ESP-IDF.
  - [ ] Partie 2 : régler la tension et allumer ou éteindre une sortie.
- [ ] **Étape 4 bis : remise à zéro.** Remise à zéro électrique des périphériques au démarrage (coupure puis rallumage des sorties de l’AXP2101), sorties DCDC2 à DCDC5 coupées.
- [ ] **Étape 5 : pilote d’écran CO5300.** QSPI, séquence d’initialisation, premiers pixels.
- [ ] **Étape 6 : LVGL.** Reprise de l’interface de CD-OS, tampons en PSRAM.
- [ ] **Étape 7 : pilote tactile CST9217.** Entrée de type pointeur pour LVGL.
- [ ] Ensuite : RTC, vibreur, veille, bus d’événements (repris de CD-OS), puis GPS, son, carte SD.
- [ ] Plus tard : capteur BHI260AP, NFC, LoRa (pilotes C officiels des fabricants + couche d’adaptation).

## Règles retenues

- **Une puce externe n’est jamais dans un état connu au démarrage.** Flasher ou appuyer sur RESET ne redémarre que l’ESP32 : le XL9555, l’AXP2101 et l’écran gardent l’état laissé par le programme précédent. Chaque pilote doit imposer l’état qu’il attend.
- **On vérifie avant de supposer.** Les numéros de broches, les registres et les tensions sont confirmés par un test ou dans le code de référence avant d’être utilisés.
- **Toujours régler la tension d’une sortie AVANT de l’allumer.**
- **Prendre les réglages dans le fichier de la bonne carte** (`LilyGoWatchUltra.cpp`) : la T-Watch S3 utilise d’autres tensions pour les mêmes sorties.
- **Modifier un registre :** lire, changer uniquement les bits voulus, réécrire.
- **Dans un pilote d'alimentation, distinguer les registres d'allumage (un bit par sortie) des registres de tension (un registre par sortie).**
- **Faire attention au valeur mise dans les registres (#define), toujours mettre un valeur ne pas les laisser vides.**

## Notes matérielles (T-Watch Ultra)

Sources : `docs/hardware/lilygo-t-watch-ultra.md` et `src/LilyGoWatchUltra.cpp` du dépôt LilyGoLib, la définition `twatch_ultra` du `boards.txt` du cœur Arduino ESP32, et `src/REG/AXP2101Constants.h` de XPowersLib.

### Mémoire et USB

- **Flash :** externe, 16 Mo, QSPI, lue en mode DIO (Winbond).
- **PSRAM :** externe, 8 Mo, QSPI. Ni la flash ni la PSRAM ne sont intégrées à la puce. Avec la PSRAM en mode octal (OPI), la montre se bloque à l’initialisation de la mémoire (`tlsf_create`).
- **RAM interne :** environ 377 Ko libres au démarrage.
- **USB :** USB-Serial/JTAG intégré à l’ESP32-S3 (`303A:1001`), apparaît en `/dev/ttyACM0`.

### Brochage

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

### Adresses I2C

| Puce | Adresse |
|---|---|
| Tactile CST9217 | `0x1A` |
| Expandeur XL9555 | `0x20` |
| Capteur BHI260AP | `0x28` |
| Gestion d’alimentation AXP2101 | `0x34` |
| Horloge PCF85063A | `0x51` |
| Vibreur DRV2605 | `0x5A` |

Le DRV2605 continue de répondre en I2C même désactivé par sa broche d’activation.

### Expandeur XL9555

Registres : entrée `0x00`/`0x01`, sortie `0x02`/`0x03`, inversion `0x04`/`0x05`, configuration `0x06`/`0x07` (bit à 1 = entrée, 0 = sortie). Les broches sont numérotées de 0 à 15 : port = broche / 8, bit = broche % 8.

**Attention :** la fiche LilyGo utilise les noms du fabricant (P00 à P07, puis P10 à P17), où le premier chiffre est le port. Son « GPIO10 » est donc notre broche 8, et son « GPIO12 » notre broche 10 (vérifié en insérant et retirant une carte SD).

| Nom LilyGo | Port, bit | Notre numéro | Rôle |
|---|---|---|---|
| GPIO6 | port 0, bit 6 | 6 | Activation du vibreur |
| GPIO7 | port 0, bit 7 | 7 | Alimentation de l’écran |
| GPIO10 | port 1, bit 0 | 8 | Reset du tactile |
| GPIO12 | port 1, bit 2 | 10 | Détection de la carte SD (0 = carte présente) |

### Gestion d’alimentation AXP2101

Identifiant `0x4A` (registre `0x03`). Tension batterie sur 13 bits en mV : 5 bits de `0x34` (masque `0x1F`) et 8 bits de `0x35`. Interrupteurs des sorties LDO dans le registre `0x90` (un bit par sortie), tensions dans les registres `0x92` à `0x99` (500 mV + 100 mV × les 5 bits du bas, pour ALDO et BLDO).

| Sortie | Bit `0x90` | Registre tension | Alimente | Tension LilyGo | Tension d’usine |
|---|---|---|---|---|---|
| ALDO1 | 0 | `0x92` | Carte SD | 3300 mV | 1800 mV |
| ALDO2 | 1 | `0x93` | Écran | 3300 mV | 3300 mV |
| ALDO3 | 2 | `0x94` | LoRa | 3300 mV | 3300 mV |
| ALDO4 | 3 | `0x95` | Capteur | **1800 mV** | **3300 mV** (danger) |
| BLDO1 | 4 | `0x96` | GPS | 1800 mV | — |
| BLDO2 | 5 | `0x97` | Haut-parleur | 3300 mV | — |
| CPUSLDO | 6 | `0x98` | Inutilisée | éteinte | — |
| DLDO1 | 7 | `0x99` | NFC | allumée, tension par défaut | — |

Autres sorties : DC1 alimente l’ESP32-S3 ; DCDC2 à DCDC5 sont inutilisées (LilyGo les coupe) ; LDO1 (VRTC) alimente la sauvegarde du GPS (toujours allumée) ; VBACKUP charge la pile du RTC.

**État d’usine** (après une coupure complète) : sorties LDO à `0x17` (ALDO1, ALDO2, ALDO3 et BLDO1 allumées ; ALDO4, BLDO2 et DLDO1 éteintes). ALDO4 est réglée à 3300 mV alors que le capteur fonctionne en 1800 mV : il faut régler sa tension avant de l’allumer.

### Écran

AMOLED 2,06 pouces, 410 × 502, contrôleur CO5300 en QSPI. Une image complète en RGB565 pèse environ 400 Ko : elle ne tient qu’en PSRAM.

Après un flash, l’écran continue d’afficher la dernière image du programme précédent (il garde sa mémoire tant qu’il est alimenté). Éviter de laisser une image fixe trop longtemps : risque de marquage (*burn-in*) sur un AMOLED.

### Batterie et coupure complète

Batterie de 1100 mAh, 4200 mV une fois pleine. Garder un courant de charge inférieur à 500 mA pour éviter que l’AXP2101 ne chauffe.

**Remise à zéro de toutes les puces :** débrancher l’USB, maintenir PWR pendant 6 secondes, attendre quelques secondes, rallumer avec un appui d’une seconde. Seuls le RTC (pile de sauvegarde) et la sauvegarde du GPS restent alimentés.

## Références

- [Guide de programmation ESP-IDF (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/index.html)
- [LilyGoLib](https://github.com/Xinyuan-LilyGO/LilyGoLib) : fiche matérielle et code de référence (`src/LilyGoWatchUltra.cpp` pour la séquence d’alimentation)
- [XPowersLib](https://github.com/lewisxhe/XPowersLib) : registres de l’AXP2101 (`src/REG/AXP2101Constants.h`)
- [Variante Arduino `lilygo_twatch_ultra`](https://github.com/espressif/arduino-esp32/tree/master/variants/lilygo_twatch_ultra) : brochage officiel