
# Projet Robotique – Groupe 05
**Université Évry Paris-Saclay – L3 – 2025-2026**

## Équipe
| Nom | Rôle |
|-----|------|
| Dan-Arias KALEMA MASUDI | Chef de projet / Code FSM |
| Ndoumbe DIAKHOUMPA | Responsable matériel et passerel |
| Karim Zerdali | Responsable de la gestion global du programme|
|Adame Gun Nouni| Responsable Structure,Design coque |
|David Papagiorgou | Responsable plan d'experience|
|Ines MELAB | Responsable Chrono et Capteur|
|Kevin RAZAFINDRATSIORY | Responsable Chrono|

## Description du robot
Robot autonome 3 roues sur base Arduino Uno (kit Makeblock).  
Il suit une ligne noire, évite des obstacles, détecte des couleurs et lance un p
**Matériel :** DRV8830 I2C, capteur ligne Me RGB (0x20), ultrason Grove D4, servo D3, NeoPixel 30 LEDs, TCS34725, LCD Grove 16×2 I2C.

## FSM – 8 états
| État | Rôle |
|------|------|
| `SUIVI_LIGNE` | Suivi de la ligne noire |
| `TUNNEL` | Traversée sans capteur ligne |
| `EVITEMENT_O1` | Contournement gauche |
| `EVITEMENT_O2` | Contournement droit |
| `DETECTION_COULEUR` | Identification zone colorée |
| `DEMI_TOUR` | Demi-tour en bout de parcours |
| `REPRENDRE_LIGNE` | Réacquisition ligne après évitement |
| `ARRIVE` | Fin de parcours |

## Structure du dépôt
Groupe_05_Projet_robotique/
├── code/               # Code Arduino (FSM)
├── electronique/       # Schémas et PCB chronomètre
├── etude_analytique/   # Plans d’expérience et CSV
└── design_coque/       # Réflexions conception mécanique

## Règles de contribution
- Ne jamais travailler directement sur `main`
- Un commit = une modification claire avec message précis
- Chaque dossier contient un `README.md`
- Ne pas committer de fichiers binaires (.pdf, .docx, .stl…)
- Toute fusion passe par une **Pull Request** validée par le chef de projet

- ## Gestion globale du programme – Karim Zerdali

### Rôle
Conception et développement de l'ensemble de la logique de pilotage :
machine à états finis (FSM), suivi de ligne, tunnel, évitement obstacles,
détection couleur, LEDs NeoPixel, afficheur LCD et tir catapulte.

### Fichiers
| Fichier | Description |
|---------|-------------|
| `code/robot_final/robot_final.ino` | Code global complet — FSM 8 états |
| `code/tests/Ultrason_ino.ino` | Test unitaire capteur ultrason |
| `code/tests/suiveur_ligne_V2.ino` | Test capteur ligne (version préliminaire) |
| `code/tests/test_tunel_obs_ino.ino` | Test tunnel + évitement obstacles |
| `code/tests/test_parcour_led_ino.ino` | Test parcours complet + LEDs + couleur |

### Dépendances
- `Wire.h` — communication I2C
- `Servo.h` — pilotage servo ultrason
- `Adafruit_TCS34725` — capteur couleur
- `Adafruit_NeoPixel` — LEDs RGB
- `rgb_lcd` — afficheur LCD Grove
# Code Arduino – Groupe 05

##  Programme et composants de la séquence de tir- Dan-Arias KALEMA MASUDI

### Fichiers
| Fichier | Description |
|---------|-------------|
| `code/robot_final/robot_final.ino` | Code global complet — FSM 8 états |
| `code/tests/test_parcour_led_ino.ino` | Test parcours complet + LEDs + tir catapulte |

### Dépendances
- `Servo.h` — pilotage servo catapulte D3
- `Adafruit_NeoPixel` — LEDs RGB signal de tir

---

## Programme et composants suivi de ligne, détection de couleur et allumage des LEDs - Dan-Arias KALEMA MASUDI

### Fichiers
| Fichier | Description |
|---------|-------------|
| `code/robot_final/robot_final.ino` | États SUIVI_LIGNE, TUNNEL, DETECTION_COULEUR |
| `code/tests/suiveur_ligne_V2.ino` | Test unitaire capteur ligne |
| `code/tests/test_parcour_led_ino.ino` | Test LEDs + détection couleur |

### Dépendances
- `Wire.h` — communication I2C capteur ligne Me RGB (0x20)
- `Adafruit_TCS34725` — capteur couleur
- `Adafruit_NeoPixel` — LEDs RGB
- `rgb_lcd` — afficheur LCD Grove 16×2

---

## Programme et composants évitement d'obstacles, suivi de mur et récupération de ligneResponsable - Dan-Arias KALEMA MASUDI

### Fichiers
| Fichier | Description |
|---------|-------------|
| `code/robot_final/robot_final.ino` | États EVITEMENT_O1, EVITEMENT_O2, REPRENDRE_LIGNE |
| `code/tests/Ultrason_ino.ino` | Test unitaire capteur ultrason |
| `code/tests/test_tunel_obs_ino.ino` | Test tunnel + évitement obstacles |

### Dépendances
- `Wire.h` — communication I2C
- `Servo.h` — orientation capteur ultrason D3
- 
# Électronique – Groupe 05

## Thématique 8 – Circuit de chronométrage- Kevin RAZAFINDRATSIORY & Inès MELAB 

### Fichiers
| Fichier | Description |
|---------|-------------|
| `schema_chrono.pdf` | Schéma complet du chronomètre (NE555 + CD4013 + CD4518 + CD4511) |
| `typon/chrono_kicad/` | Projet KiCad complet (routage + typon) |
| `tests/labdec_test_chrono.png` | Tests Labdec : horloge, bascule, compteur |
| `carte_finale/chrono_pcb.jpg` | Carte finale gravée et soudée |
| `debug/debug_notes.md` | Notes de débogage (capteurs, horloge, segments) |

### Dépendances
- `NE555` — génération d'horloge 1 Hz
- `CD4013` — bascule D pour stabiliser le déclenchement
- `CD4518` — compteur BCD double
- `CD4511` — décodeur BCD → 7 segments
- Barrières optiques IR — détection du robot
- KiCad 8/9 — routage PCB
