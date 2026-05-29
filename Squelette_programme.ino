
//  1. MODE DEBUG - mettre à 1 pour tester, 0 pour le concours
// ------------------------------------------------------------
#define MODE_TEST_MOTEURS  1   // 1 = test moteurs au démarrage
                               // 0 = démarrage direct parcours


// ------------------------------------------------------------
//  2. INCLUDES
// ------------------------------------------------------------
#include <Wire.h>               // I2C obligatoire (LCD, couleur)
#include <rgb_lcd.h>            // LCD Grove RGB 2x16
#include <Servo.h>              // Servomoteur lanceur
#include <Adafruit_NeoPixel.h>  // Ruban LED RGB 30 LEDs


// ------------------------------------------------------------
//  3. BROCHES - VÉRIFIER SELON VOTRE CÂBLAGE RÉEL
// ------------------------------------------------------------

// --- Moteurs DC (carte driver custom bleue) ---
#define PIN_MOTEUR_G_FWD   5   // Moteur gauche - sens avant  (PWM ~)
#define PIN_MOTEUR_G_BWD   4   // Moteur gauche - sens arrière
#define PIN_MOTEUR_D_FWD   6   // Moteur droit  - sens avant  (PWM ~)
#define PIN_MOTEUR_D_BWD   7   // Moteur droit  - sens arrière

// --- Capteurs de ligne (suiveur Grove) ---
#define PIN_LIGNE_G        8   // Capteur gauche
#define PIN_LIGNE_C        9   // Capteur centre
#define PIN_LIGNE_D        10  // Capteur droite

// --- Capteur ultrason Grove ---
#define PIN_ULTRASON       11  // Signal unique (Trig + Echo)

// --- Servo lanceur Grove ---
#define PIN_SERVO          3   // Broche PWM ~ du shield Grove

// --- Ruban LED RGB 30 LEDs ---
#define PIN_LED_STRIP      2
#define NB_LEDS            30

// --- LED de debug intégrée ---
#define PIN_LED_DEBUG      13

// Angles servo - À CALIBRER physiquement sur votre mécanisme
#define ANGLE_ARME         10   // Position balle retenue
#define ANGLE_TIR          120  // Position tir


// ------------------------------------------------------------
//  4. OBJETS GLOBAUX
// ------------------------------------------------------------
rgb_lcd           lcd;
Servo             servoLanceur;
Adafruit_NeoPixel strip(NB_LEDS, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);


// ------------------------------------------------------------
//  5. ÉTATS DE LA MACHINE FSM
// ------------------------------------------------------------
enum Etat {
  ETAT_TEST_MOTEURS,
  ETAT_SUIVI_LIGNE,
  ETAT_TUNNEL,
  ETAT_EVIT_OBS1,
  ETAT_EVIT_OBS2,
  ETAT_MONTEE_RAMPE,
  ETAT_DETECT_COULEUR,
  ETAT_DEMI_TOUR,
  ETAT_CHRONO,
  ETAT_PASSERELLE,
  ETAT_AFFICH_DIST,
  ETAT_LANCER,
  ETAT_FIN,
  ETAT_STOP_URGENCE
};

Etat etatCourant = ETAT_TEST_MOTEURS;


// ------------------------------------------------------------
//  6. VARIABLES GLOBALES
// ------------------------------------------------------------
int  lignG = 0, lignC = 0, lignD = 0;
long distanceCm = 0;
int  couleurDetectee = 0;

unsigned long tempsDebutChrono = 0;
unsigned long tempsChrono      = 0;
unsigned long tempsEtat        = 0;

#define VITESSE_NORMALE  160
#define VITESSE_LENTE    100
#define VITESSE_RAMPE    210
#define VITESSE_ROTATION 130
#define SEUIL_OBSTACLE_CM  20


// ------------------------------------------------------------
//  7. PROTOTYPES
// ------------------------------------------------------------
void lireCapteursLigne();
long lireUltrason();
int  lireCouleur();
void avancer(int vitG, int vitD);
void tournerDroite(int vit);
void tournerGauche(int vit);
void reculer(int vit);
void stopMoteurs();
void lancerBalle();
void armerLanceur();
void afficherLCD(String l1, String l2);
void allumerLEDs(uint32_t couleur);
void gererTestMoteurs();
void gererEtatSuiviLigne();
void gererEtatTunnel();
void gererEtatEvitObs(int numObs);
void gererEtatMonteeRampe();
void gererEtatDetectCouleur();
void gererEtatDemiTour();
void gererEtatChrono();
void gererEtatPasserelle();
void gererEtatAffiDist();
void gererEtatLancer();
void gererEtatFin();
void gererStopUrgence();


// ============================================================
//  8. SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  Serial.println("=============================");
  Serial.println("  ROBOT GROUPE 5 - DÉMARRAGE");
  Serial.println("=============================");

  pinMode(PIN_MOTEUR_G_FWD, OUTPUT);
  pinMode(PIN_MOTEUR_G_BWD, OUTPUT);
  pinMode(PIN_MOTEUR_D_FWD, OUTPUT);
  pinMode(PIN_MOTEUR_D_BWD, OUTPUT);
  stopMoteurs();

  pinMode(PIN_LIGNE_G, INPUT);
  pinMode(PIN_LIGNE_C, INPUT);
  pinMode(PIN_LIGNE_D, INPUT);
  pinMode(PIN_LED_DEBUG, OUTPUT);
  digitalWrite(PIN_LED_DEBUG, LOW);

  servoLanceur.attach(PIN_SERVO);
  armerLanceur();

  lcd.begin(16, 2);
  lcd.setRGB(0, 128, 255);
  afficherLCD("GROUPE 5", "Initialisation");
  delay(1000);

  strip.begin();
  strip.clear();
  strip.show();

  if (MODE_TEST_MOTEURS == 1) {
    etatCourant = ETAT_TEST_MOTEURS;
    afficherLCD("TEST MOTEURS", "3 secondes...");
    Serial.println("Mode TEST MOTEURS activé");
  } else {
    etatCourant = ETAT_SUIVI_LIGNE;
    afficherLCD("PARCOURS", "Section 1...");
    Serial.println("Démarrage direct parcours");
  }

  tempsEtat = millis();
  Serial.println("Setup terminé - GO !");
}


// ============================================================
//  9. LOOP
// ============================================================
void loop() {
  switch (etatCourant) {
    case ETAT_TEST_MOTEURS:   gererTestMoteurs();       break;
    case ETAT_SUIVI_LIGNE:    gererEtatSuiviLigne();    break;
    case ETAT_TUNNEL:         gererEtatTunnel();        break;
    case ETAT_EVIT_OBS1:      gererEtatEvitObs(1);      break;
    case ETAT_EVIT_OBS2:      gererEtatEvitObs(2);      break;
    case ETAT_MONTEE_RAMPE:   gererEtatMonteeRampe();   break;
    case ETAT_DETECT_COULEUR: gererEtatDetectCouleur(); break;
    case ETAT_DEMI_TOUR:      gererEtatDemiTour();      break;
    case ETAT_CHRONO:         gererEtatChrono();        break;
    case ETAT_PASSERELLE:     gererEtatPasserelle();    break;
    case ETAT_AFFICH_DIST:    gererEtatAffiDist();      break;
    case ETAT_LANCER:         gererEtatLancer();        break;
    case ETAT_FIN:            gererEtatFin();           break;
    case ETAT_STOP_URGENCE:   gererStopUrgence();       break;
    default: etatCourant = ETAT_STOP_URGENCE;           break;
  }
}


// ============================================================
//  10. TEST MOTEURS
// ============================================================
void gererTestMoteurs() {
  unsigned long t = millis() - tempsEtat;

  if (t < 1000) {
    afficherLCD("TEST: Mot.G", "Avant 1s");
    analogWrite(PIN_MOTEUR_G_FWD, VITESSE_NORMALE);
    digitalWrite(PIN_MOTEUR_G_BWD, LOW);
    digitalWrite(PIN_MOTEUR_D_FWD, LOW);
    digitalWrite(PIN_MOTEUR_D_BWD, LOW);
    Serial.println("TEST: Moteur GAUCHE avant");

  } else if (t < 2000) {
    afficherLCD("TEST: Mot.D", "Avant 1s");
    digitalWrite(PIN_MOTEUR_G_FWD, LOW);
    digitalWrite(PIN_MOTEUR_G_BWD, LOW);
    analogWrite(PIN_MOTEUR_D_FWD, VITESSE_NORMALE);
    digitalWrite(PIN_MOTEUR_D_BWD, LOW);
    Serial.println("TEST: Moteur DROIT avant");

  } else if (t < 3000) {
    afficherLCD("TEST: Les 2", "Avant 1s");
    analogWrite(PIN_MOTEUR_G_FWD, VITESSE_NORMALE);
    digitalWrite(PIN_MOTEUR_G_BWD, LOW);
    analogWrite(PIN_MOTEUR_D_FWD, VITESSE_NORMALE);
    digitalWrite(PIN_MOTEUR_D_BWD, LOW);
    Serial.println("TEST: Les DEUX moteurs");

  } else {
    stopMoteurs();
    afficherLCD("TEST OK!", "Parcours...");
    Serial.println("TEST terminé - Début parcours");
    delay(1000);
    tempsEtat = millis();
    etatCourant = ETAT_SUIVI_LIGNE;
  }
}


// ============================================================
//  11. ÉTATS DU PARCOURS
// ============================================================

void gererEtatSuiviLigne() {
  lireCapteursLigne();

  if (lignC == 1 && lignG == 0 && lignD == 0) {
    avancer(VITESSE_NORMALE, VITESSE_NORMALE);
  } else if (lignD == 1 && lignC == 0) {
    avancer(VITESSE_NORMALE, VITESSE_LENTE);
  } else if (lignG == 1 && lignC == 0) {
    avancer(VITESSE_LENTE, VITESSE_NORMALE);
  } else if (lignG == 1 && lignD == 1) {
    stopMoteurs();
    Serial.println("-> Croisement : TUNNEL");
    delay(200);
    tempsEtat = millis();
    etatCourant = ETAT_TUNNEL;
  } else {
    tournerDroite(VITESSE_LENTE);
  }
}

void gererEtatTunnel() {
  lireCapteursLigne();

  if (lignC == 1) {
    avancer(VITESSE_NORMALE, VITESSE_NORMALE);
  } else if (lignD == 1) {
    avancer(VITESSE_NORMALE, VITESSE_LENTE);
  } else if (lignG == 1) {
    avancer(VITESSE_LENTE, VITESSE_NORMALE);
  } else {
    avancer(VITESSE_LENTE, VITESSE_LENTE);
  }

  if (millis() - tempsEtat > 5000 && lignC == 1) {
    Serial.println("-> Sortie tunnel : EVIT_OBS1");
    tempsEtat = millis();
    etatCourant = ETAT_EVIT_OBS1;
  }
}

void gererEtatEvitObs(int numObs) {
  distanceCm = lireUltrason();
  lireCapteursLigne();

  if (distanceCm > 0 && distanceCm < SEUIL_OBSTACLE_CM) {
    Serial.print("Obstacle "); Serial.print(numObs);
    Serial.print(" à "); Serial.print(distanceCm); Serial.println(" cm");
    stopMoteurs();
    delay(100);
    tournerDroite(VITESSE_ROTATION);
    delay(600);
    avancer(VITESSE_NORMALE, VITESSE_NORMALE);
    delay(800);
    tournerGauche(VITESSE_ROTATION);
    delay(600);
  } else {
    if (lignC == 1)      avancer(VITESSE_NORMALE, VITESSE_NORMALE);
    else if (lignD == 1) avancer(VITESSE_NORMALE, VITESSE_LENTE);
    else if (lignG == 1) avancer(VITESSE_LENTE, VITESSE_NORMALE);
    else                 avancer(VITESSE_LENTE, VITESSE_LENTE);
  }

  if (millis() - tempsEtat > 6000) {
    stopMoteurs();
    tempsEtat = millis();
    if (numObs == 1) {
      Serial.println("-> Obstacle 1 passé : EVIT_OBS2");
      etatCourant = ETAT_EVIT_OBS2;
    } else {
      Serial.println("-> Obstacle 2 passé : MONTEE_RAMPE");
      etatCourant = ETAT_MONTEE_RAMPE;
    }
  }
}

void gererEtatMonteeRampe() {
  avancer(VITESSE_RAMPE, VITESSE_RAMPE);

  if (millis() - tempsEtat > 4000) {
    stopMoteurs();
    Serial.println("-> Rampe franchie : DETECT_COULEUR");
    tempsEtat = millis();
    etatCourant = ETAT_DETECT_COULEUR;
  }
}

void gererEtatDetectCouleur() {
  stopMoteurs();
  couleurDetectee = lireCouleur();

  if (couleurDetectee == 1) {
    allumerLEDs(strip.Color(255, 0, 0));
    afficherLCD("Couleur: ROUGE", "LEDs allumees");
    lcd.setRGB(255, 0, 0);
  } else if (couleurDetectee == 2) {
    allumerLEDs(strip.Color(0, 255, 0));
    afficherLCD("Couleur: VERT", "LEDs allumees");
    lcd.setRGB(0, 255, 0);
  } else if (couleurDetectee == 3) {
    allumerLEDs(strip.Color(0, 0, 255));
    afficherLCD("Couleur: BLEU", "LEDs allumees");
    lcd.setRGB(0, 0, 255);
  } else {
    allumerLEDs(strip.Color(255, 255, 255));
    afficherLCD("Couleur: ?", "Defaut blanc");
  }

  delay(2000);
  tempsEtat = millis();
  etatCourant = ETAT_DEMI_TOUR;
}

void gererEtatDemiTour() {
  tournerGauche(VITESSE_ROTATION);
  delay(1600);
  stopMoteurs();
  Serial.println("-> Demi-tour : CHRONO");
  delay(300);
  tempsDebutChrono = 0;
  tempsEtat = millis();
  etatCourant = ETAT_CHRONO;
}

void gererEtatChrono() {
  if (tempsDebutChrono == 0) {
    tempsDebutChrono = millis();
    afficherLCD("Chrono START", "En cours...");
    Serial.println("Chrono démarré");
  }

  lireCapteursLigne();
  if (lignC == 1)      avancer(VITESSE_NORMALE, VITESSE_NORMALE);
  else if (lignD == 1) avancer(VITESSE_NORMALE, VITESSE_LENTE);
  else if (lignG == 1) avancer(VITESSE_LENTE, VITESSE_NORMALE);
  else                 avancer(VITESSE_LENTE, VITESSE_LENTE);

  if (millis() - tempsEtat > 8000) {
    tempsChrono = millis() - tempsDebutChrono;
    stopMoteurs();
    afficherLCD("Chrono STOP", String(tempsChrono / 1000) + "s");
    Serial.print("Chrono : "); Serial.print(tempsChrono / 1000); Serial.println("s");
    delay(1000);
    tempsEtat = millis();
    etatCourant = ETAT_PASSERELLE;
  }
}

void gererEtatPasserelle() {
  avancer(VITESSE_LENTE, VITESSE_LENTE);
  afficherLCD("Passerelle", "En cours...");

  if (millis() - tempsEtat > 5000) {
    stopMoteurs();
    Serial.println("-> Passerelle : AFFICH_DIST");
    tempsEtat = millis();
    etatCourant = ETAT_AFFICH_DIST;
  }
}

void gererEtatAffiDist() {
  stopMoteurs();
  distanceCm = lireUltrason();
  afficherLCD("Dist. panier:", String(distanceCm) + " cm");
  lcd.setRGB(0, 128, 255);
  Serial.print("Distance panier : "); Serial.print(distanceCm); Serial.println(" cm");
  delay(2000);
  tempsEtat = millis();
  etatCourant = ETAT_LANCER;
}

void gererEtatLancer() {
  stopMoteurs();
  afficherLCD("LANCER !", "Alignement...");

  if (distanceCm > 30) {
    avancer(VITESSE_LENTE, VITESSE_LENTE);
    delay(500);
    stopMoteurs();
  }

  delay(500);
  lancerBalle();
  afficherLCD("BALLE LANCEE!", ":)");
  Serial.println("Balle lancée !");
  delay(1500);
  etatCourant = ETAT_FIN;
}

void gererEtatFin() {
  stopMoteurs();
  allumerLEDs(strip.Color(255, 215, 0));
  lcd.setRGB(0, 255, 0);
  afficherLCD("PARCOURS FINI!", "GROUPE 5 !!!");
  Serial.println("=== FIN DU PARCOURS ===");
  digitalWrite(PIN_LED_DEBUG, HIGH);
  delay(300);
  digitalWrite(PIN_LED_DEBUG, LOW);
  delay(300);
}

void gererStopUrgence() {
  stopMoteurs();
  allumerLEDs(strip.Color(255, 0, 0));
  lcd.setRGB(255, 0, 0);
  afficherLCD("STOP URGENCE", "Verifier robot");
  Serial.println("!!! STOP URGENCE !!!");
  digitalWrite(PIN_LED_DEBUG, HIGH);
  delay(300);
  digitalWrite(PIN_LED_DEBUG, LOW);
  delay(300);
}


// ============================================================
//  12. FONCTIONS UTILITAIRES
// ============================================================

void lireCapteursLigne() {
  lignG = digitalRead(PIN_LIGNE_G);
  lignC = digitalRead(PIN_LIGNE_C);
  lignD = digitalRead(PIN_LIGNE_D);
}

long lireUltrason() {
  pinMode(PIN_ULTRASON, OUTPUT);
  digitalWrite(PIN_ULTRASON, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASON, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASON, LOW);
  pinMode(PIN_ULTRASON, INPUT);
  long duree = pulseIn(PIN_ULTRASON, HIGH, 30000);
  if (duree == 0) return 999;
  return duree / 58;
}

int lireCouleur() {
  // TODO : à compléter par votre IT selon bibliothèque capteur couleur
  // Retourner 1=rouge, 2=vert, 3=bleu
  return 0;
}

void avancer(int vitG, int vitD) {
  vitG = constrain(vitG, 0, 255);
  vitD = constrain(vitD, 0, 255);
  analogWrite(PIN_MOTEUR_G_FWD, vitG);
  digitalWrite(PIN_MOTEUR_G_BWD, LOW);
  analogWrite(PIN_MOTEUR_D_FWD, vitD);
  digitalWrite(PIN_MOTEUR_D_BWD, LOW);
}

void reculer(int vit) {
  vit = constrain(vit, 0, 255);
  digitalWrite(PIN_MOTEUR_G_FWD, LOW);
  analogWrite(PIN_MOTEUR_G_BWD, vit);
  digitalWrite(PIN_MOTEUR_D_FWD, LOW);
  analogWrite(PIN_MOTEUR_D_BWD, vit);
}

void tournerDroite(int vit) {
  vit = constrain(vit, 0, 255);
  analogWrite(PIN_MOTEUR_G_FWD, vit);
  digitalWrite(PIN_MOTEUR_G_BWD, LOW);
  digitalWrite(PIN_MOTEUR_D_FWD, LOW);
  analogWrite(PIN_MOTEUR_D_BWD, vit);
}

void tournerGauche(int vit) {
  vit = constrain(vit, 0, 255);
  digitalWrite(PIN_MOTEUR_G_FWD, LOW);
  analogWrite(PIN_MOTEUR_G_BWD, vit);
  analogWrite(PIN_MOTEUR_D_FWD, vit);
  digitalWrite(PIN_MOTEUR_D_BWD, LOW);
}

void stopMoteurs() {
  digitalWrite(PIN_MOTEUR_G_FWD, LOW);
  digitalWrite(PIN_MOTEUR_G_BWD, LOW);
  digitalWrite(PIN_MOTEUR_D_FWD, LOW);
  digitalWrite(PIN_MOTEUR_D_BWD, LOW);
}

void armerLanceur() {
  servoLanceur.write(ANGLE_ARME);
  Serial.print("Servo armé à "); Serial.print(ANGLE_ARME); Serial.println("°");
  delay(300);
}

void lancerBalle() {
  servoLanceur.write(ANGLE_TIR);
  Serial.print("Servo tir à "); Serial.print(ANGLE_TIR); Serial.println("°");
  delay(600);
}

void afficherLCD(String l1, String l2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(l2.substring(0, 16));
}

void allumerLEDs(uint32_t couleur) {
  for (int i = 0; i < NB_LEDS; i++) {
    strip.setPixelColor(i, couleur);
  }
  strip.show();
}
