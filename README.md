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
Il suit une ligne noire, évite des obstacles, détecte des couleurs et lance un projectile via catapulte.

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
