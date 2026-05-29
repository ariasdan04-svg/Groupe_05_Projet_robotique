
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
## Passerelle en bâtonnets de glace — DIAKHOUMPA Ndoumbé

### Rôle
Conception, modélisation et réalisation d'une passerelle structurelle
en bâtonnets de glace permettant au robot de franchir la section
reliant la rampe R2 à la plateforme de tir P2. Modélisation numérique
sur RDM6, fabrication par assemblage de treillis triangulés doublés
et validation expérimentale par test de passage du robot.

### Fichiers
| Fichier | Description |
|---------|-------------|
| `passerelle/rapport_technique.docx` | Rapport technique complet — conception, RDM6, résultats |
| `passerelle/poster_passerelle.pptx` | Poster de présentation |
| `passerelle/simulation/RDM6_contrainte.png` | Simulation RDM6 — contrainte max 24,98 MPa |
| `passerelle/photos/arcs_lateraux.jpg` | Fabrication des arcs latéraux en triangles doublés |
| `passerelle/photos/passerelle_terminee.jpg` | Vue d'ensemble — 900 mm × 300 mm |
| `passerelle/photos/test_robot.jpg` | Test de passage du robot — validé ✓ |
### Dépendances
- `Wire.h` — communication I2C
- `Servo.h` — pilotage servo ultrason
- `Adafruit_TCS34725` — capteur couleur
- `Adafruit_NeoPixel` — LEDs RGB
- `rgb_lcd` — afficheur LCD Grove
