#include <Wire.h>
#include <Servo.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_NeoPixel.h>
#include "rgb_lcd.h"

#define PIN_SERVO     3
#define PIN_ULTRASON  4

#define MOTEUR_G     0x66
#define MOTEUR_D     0x68
#define MOTEUR_CA    0x60
#define ADDR_LF      0x20

#define AVANT    0x01
#define ARRIERE  0x02
#define FREIN    0x03
#define REG_DIGITAL 0x07

#define SERVO_AVANT   90    // centre physique
#define SERVO_GAUCHE  0     // gauche = 0°
#define SERVO_DROITE  180   // droite = 180°
#define SERVO_TUNNEL   30   // 30° droite pour détection tunnel

#define VITESSE_BASE   42
#define VITESSE_LENTE  42
#define VITESSE_PIVOT  44
#define VITESSE_TUNNEL 30
#define KP             0.0025
#define ZONE_MORTE     3000
#define TRIM_G         -5
#define TRIM_D         0

#define DIST_URGENCE       2
#define DIST_OBSTACLE      25
#define DIST_INFINIE       9999
#define SEUIL_LIGNE_PERDUE 20

#define SEUIL_ARRIVEE      30
#define DUREE_TOURNE_90    820
#define DUREE_TOURNE_90_G  920
#define DUREE_TOURNE_80    730
#define DUREE_TOURNE_70    650
#define DUREE_PAR_CM       35
#define DIST_RECUL_INIT    8
#define SEUIL_OBSTACLE_COTE  70
#define DIST_DEPASSEMENT     25
#define DIST_ECART           55
#define DIST_ECART2        3200
#define DIST_APPROCHE        35
#define TUNNEL_TIMEOUT     25000UL

#define PIN_LED          6
#define NUMPIXELS        30
#define DIST_MUR         5
#define RECUL_MS         1400
#define DEMI_TOUR_MS     2200
#define VITESSE_MONTEE   55
#define DELAI_TIR_MS     5000


enum Etat { SUIVI_LIGNE, TUNNEL, EVITEMENT_O1, EVITEMENT_O2, DETECTION_COULEUR, DEMI_TOUR, REPRENDRE_LIGNE, ARRIVE };

Etat     etatRobot           = SUIVI_LIGNE;
uint8_t  compteurArrivee     = 0;
uint16_t compteurLignePerdue = 0;
uint8_t  nbObstaclesEvites   = 0;
bool     apresObstacle       = false;
bool     tunnelTraverse      = false;

Servo    servoUs;
int      servoPos            = SERVO_AVANT;
unsigned long tunnelExitTime = 0;

Adafruit_NeoPixel strip(NUMPIXELS, PIN_LED, NEO_RGB + NEO_KHZ800);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
bool apresDetectionCouleur = false;

rgb_lcd lcd;
long          distTunnel  = 0;
int           posLigne    = 0;
char          nomCouleur[17] = "";
unsigned long tLcd        = 0;

void majLcd() {
  char buf[17];
  lcd.setCursor(0, 0);
  switch (etatRobot) {
    case SUIVI_LIGNE:       lcd.print("SUIVI LIGNE     "); break;
    case TUNNEL:            lcd.print("TUNNEL          "); break;
    case EVITEMENT_O1:      lcd.print("OBSTACLE 1      "); break;
    case EVITEMENT_O2:      lcd.print("OBSTACLE 2      "); break;
    case DETECTION_COULEUR: lcd.print("DETECTION       "); break;
    case DEMI_TOUR:         lcd.print("DEMI-TOUR       "); break;
    case REPRENDRE_LIGNE:   lcd.print("CHERCHE LIGNE   "); break;
    case ARRIVE:            lcd.print("*** ARRIVE! *** "); break;
  }
  lcd.setCursor(0, 1);
  switch (etatRobot) {
    case SUIVI_LIGNE:
      if      (posLigne >= 20000)  snprintf(buf, 17, "VIRAGE DROITE   ");
      else if (posLigne <= -20000) snprintf(buf, 17, "VIRAGE GAUCHE   ");
      else                         snprintf(buf, 17, "LIGNE DROITE    ");
      lcd.print(buf); break;
    case TUNNEL:
      snprintf(buf, 17, "Mur: %3dcm      ", (int)distTunnel);
      lcd.print(buf); break;
    case EVITEMENT_O1:
    case EVITEMENT_O2:
      lcd.print("Longement...    "); break;
    case DETECTION_COULEUR:
    case DEMI_TOUR:
      snprintf(buf, 17, "%-16s", nomCouleur);
      lcd.print(buf); break;
    default:
      lcd.print("                "); break;
  }
}

void piloterMoteur(byte adresse, byte direction, byte vitesse) {
  vitesse = constrain(vitesse, 0, 63);
  Wire.beginTransmission(adresse);
  Wire.write(0x00);
  Wire.write((vitesse << 2) | direction);
  Wire.endTransmission();
}

void avancer(int vG, int vD) {
  vG = constrain(vG + TRIM_G, 0, 63);
  vD = constrain(vD + TRIM_D, 0, 63);
  piloterMoteur(MOTEUR_G, AVANT,   vG);
  piloterMoteur(MOTEUR_D, ARRIERE, vD);
}

void arreter() {
  piloterMoteur(MOTEUR_G, FREIN, 0);
  piloterMoteur(MOTEUR_D, FREIN, 0);
}

void servoEcrire(int angle) {
  servoPos = angle;
  servoUs.writeMicroseconds(map(angle, 0, 180, 500, 2500));
}

void tournerGauche(unsigned long duree) {
  piloterMoteur(MOTEUR_G, ARRIERE, VITESSE_PIVOT);
  piloterMoteur(MOTEUR_D, ARRIERE, VITESSE_PIVOT);
  delay(duree);
  arreter(); delay(50);
}

void tournerDroite(unsigned long duree) {
  piloterMoteur(MOTEUR_G, AVANT,   VITESSE_PIVOT);
  piloterMoteur(MOTEUR_D, AVANT,   VITESSE_PIVOT);
  delay(duree);
  arreter(); delay(50);
}

void reculer(int vitesse, unsigned long duree) {
  piloterMoteur(MOTEUR_G, ARRIERE, vitesse);
  piloterMoteur(MOTEUR_D, AVANT,   vitesse);
  delay(duree);
  arreter(); delay(50);
}

void avancerDroit(int vitesse, unsigned long duree) {
  avancer(vitesse, vitesse);
  delay(duree);
  arreter(); delay(300);
}

long mesureUltrason() {
  pinMode(PIN_ULTRASON, OUTPUT);
  digitalWrite(PIN_ULTRASON, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASON, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_ULTRASON, LOW);
  pinMode(PIN_ULTRASON, INPUT);
  long d = pulseIn(PIN_ULTRASON, HIGH, 15000UL);
  return (d == 0) ? DIST_INFINIE : d / 58;
}

void verifierUltrason() {
  long dist = mesureUltrason();
  if (dist > 0 && dist <= DIST_URGENCE) {
    arreter();
    while (mesureUltrason() <= DIST_URGENCE) delay(50);
  }
}

int lirePosition() {
  Wire.beginTransmission(ADDR_LF);
  Wire.write(REG_DIGITAL);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)ADDR_LF, (uint8_t)1);
  if (!Wire.available()) return 0;
  uint8_t brut = Wire.read();
  uint8_t etat = (~brut) & 0x0F;

  switch (etat) {
    case 0b1001:
    case 0b0110: return 1000;
    case 0b1000:
    case 0b1100: return -20000;
    case 0b0001:
    case 0b0011: return 20000;
    case 0b1110: return -30000;
    case 0b0111: return 30000;
    case 0b1111: return 0;
    case 0b0000: return -1;
    default:     return 1000;
  }
}

bool chercherLigne(unsigned long timeoutMs) {
  avancer(VITESSE_LENTE, VITESSE_LENTE);
  unsigned long debut = millis();
  while (millis() - debut < timeoutMs) {
    int pos = lirePosition();
    if (pos != -1) { arreter(); delay(50); return true; }
    delay(15);
  }
  arreter();
  return false;
}

void suivreLigneImmediatement(unsigned long dureeMs) {
  uint8_t cNoir = 0;
  unsigned long t = millis();
  while (millis() - t < dureeMs) {
    int p = lirePosition();
    if (p == -1) {
      if (++cNoir >= 30) break;
      avancer(VITESSE_BASE, VITESSE_BASE);
    } else {
      cNoir = 0;
      int corr = (p != 0 && abs(p) >= ZONE_MORTE) ? (int)(KP * p) : 0;
      avancer(VITESSE_BASE - corr, VITESSE_BASE + corr);
    }
    delay(5);
  }
}
void setup() {
  Wire.begin();
  Serial.begin(9600);
  servoUs.attach(PIN_SERVO);
  servoEcrire(SERVO_AVANT);
  strip.begin();
  strip.show();
  tcs.begin();
  lcd.begin(16, 2);
  lcd.setRGB(0, 128, 255);
  lcd.setCursor(0, 0); lcd.print("  ROBOT PRET    ");
  lcd.setCursor(0, 1); lcd.print("      GO !      ");
  delay(500);
  arreter();
  Serial.println("GO");
}

void loop() {
  verifierUltrason();

  if (millis() - tLcd > 400) { tLcd = millis(); majLcd(); }

  switch (etatRobot) {
    case SUIVI_LIGNE:       gererSuiviLigne();   break;
    case TUNNEL:            gererTunnel();       break;
    case EVITEMENT_O1:      evitementDroite();   break;
    case EVITEMENT_O2:      evitementGauche();   break;
    case DETECTION_COULEUR: gererCouleur();      break;
    case DEMI_TOUR:         gererDemiTour();     break;
    case REPRENDRE_LIGNE:   gererRepriseLigne(); break;
    case ARRIVE:
      piloterMoteur(MOTEUR_CA, AVANT, 45);
      arreter();
      delay(500);
      break;
  }
}

void gererSuiviLigne() {
  int position = lirePosition();

  if (apresObstacle && position != -1) {
    apresObstacle = false;
    compteurArrivee = 0;
  }

  if (position != 0 && position != -1) {
    compteurLignePerdue = 0;
    if (servoPos != SERVO_AVANT) servoEcrire(SERVO_AVANT);
  } else {
    compteurLignePerdue++;
  }

  if (compteurLignePerdue > SEUIL_LIGNE_PERDUE && !tunnelTraverse && millis() - tunnelExitTime > 2000UL) {
    etatRobot = TUNNEL;
    compteurLignePerdue = 0;
    compteurArrivee = 0;
    Serial.println("-> TUNNEL");
    return;
  }

  if (position == -1) {
    if (apresObstacle) {
      apresObstacle = false;
      compteurArrivee = 0;
      return;
    }
    compteurArrivee++;
    if (compteurArrivee >= SEUIL_ARRIVEE && millis() - tunnelExitTime > 2000UL) {
      Serial.println("*** ARRIVEE ***");
      arreter();
      lcd.setCursor(0, 0); lcd.print("*** ARRIVEE ***  ");
      lcd.setCursor(0, 1); lcd.print("Chargez balle!  ");
      Serial.println("STOP ARRIVEE");
      delay(DELAI_TIR_MS);
      lcd.setCursor(0, 1); lcd.print("    TIR !       ");
      etatRobot = ARRIVE;
      return;
    }
    avancer(VITESSE_BASE, VITESSE_BASE);
    delay(5);
    return;
  }
  compteurArrivee = 0;

  if (servoPos == SERVO_AVANT) {
    long distAvant = mesureUltrason();
    if (nbObstaclesEvites >= 2 && !apresDetectionCouleur) {
      if (distAvant <= DIST_MUR && distAvant > 0) {
        arreter(); delay(200);
        etatRobot = DETECTION_COULEUR;
        return;
      }
    } else if (!apresDetectionCouleur && nbObstaclesEvites < 2) {
      if (distAvant < DIST_OBSTACLE && millis() - tunnelExitTime > 1500UL) {
        arreter(); delay(200);
        etatRobot = (nbObstaclesEvites == 0) ? EVITEMENT_O1 : EVITEMENT_O2;
        compteurArrivee = 0;
        return;
      }
    }
  }

  if (position == 0) {
    avancer(VITESSE_BASE, VITESSE_BASE);
    return;
  }

  posLigne = position;
  int correction = (int)(KP * position);
  if (abs(position) < ZONE_MORTE) {
    avancer(VITESSE_BASE, VITESSE_BASE);
  } else {
    avancer(VITESSE_BASE - correction, VITESSE_BASE + correction);
  }
}

void gererTunnel() {
  unsigned long debut = millis();
  const long CIBLE_G  = 30;
  const int  MAX_CORR = 12;

  servoEcrire(SERVO_TUNNEL);
  delay(150);

  while (millis() - debut < TUNNEL_TIMEOUT) {

    int pos = lirePosition();
    if (pos != -1) {
      servoEcrire(SERVO_AVANT);
      delay(150);
      suivreLigneImmediatement(1500UL);
      tunnelTraverse = true;
      tunnelExitTime = millis();
      etatRobot = SUIVI_LIGNE;
      compteurLignePerdue = 0;
      compteurArrivee = 0;
      return;
    }

    long distG = mesureUltrason();
    distTunnel = distG;
    if (millis() - tLcd > 400) { tLcd = millis(); majLcd(); }
    Serial.print("G:"); Serial.println(distG);

    if (distG > 0 && distG < 80) {
      int correction = constrain((int)((distG - CIBLE_G) * 4 / 5), -MAX_CORR, MAX_CORR);
      avancer(VITESSE_TUNNEL - correction, VITESSE_TUNNEL + correction);
    } else {
      avancer(VITESSE_TUNNEL, VITESSE_TUNNEL);
    }

    delay(30);
  }

  servoEcrire(SERVO_AVANT);
  tunnelExitTime = millis();
  etatRobot = SUIVI_LIGNE;
  compteurLignePerdue = 0;
  compteurArrivee = 0;
}

void longerObstacleParDroite() {
  unsigned long debut = millis();
  bool obstacleVu = false;
  unsigned long tempsObstaclePerdu = 0;

  while (millis() - debut < 20000UL) {
    long d = mesureUltrason();
    Serial.print(F("D:")); Serial.println(d);

    if (!obstacleVu) {
      if (d < 30 && d > 5) {
        obstacleVu = true;
      } else {
        avancer(VITESSE_LENTE, VITESSE_LENTE);
        if (millis() - debut > 3000UL) { arreter(); delay(50); return; }
      }
      delay(50); continue;
    }

    if (d > 30 || d == DIST_INFINIE) {
      if (tempsObstaclePerdu == 0) tempsObstaclePerdu = millis();
      if (millis() - tempsObstaclePerdu > 250) { arreter(); delay(50); return; }
      avancer(VITESSE_LENTE, VITESSE_LENTE);
    } else {
      tempsObstaclePerdu = 0;
      long err = d - 15;
      if (abs(err) < 5)   avancer(VITESSE_LENTE,     VITESSE_LENTE);
      else if (err > 0)   avancer(VITESSE_LENTE + 8, VITESSE_LENTE - 8);  // trop loin → virer droite vers obstacle
      else                avancer(VITESSE_LENTE - 8, VITESSE_LENTE + 8);  // trop proche → virer gauche
    }
    delay(50);
  }
  arreter(); delay(50);
}

void longerObstacleParGauche() {
  unsigned long debut = millis();
  bool obstacleVu = false;
  unsigned long tempsObstaclePerdu = 0;

  while (millis() - debut < 20000UL) {
    long d = mesureUltrason();
    Serial.print(F("G:")); Serial.println(d);

    if (!obstacleVu) {
      if (d < 60 && d > 5) {
        obstacleVu = true;
      } else {
        avancer(VITESSE_LENTE, VITESSE_LENTE);
        if (millis() - debut > 3000UL) { arreter(); delay(50); return; }
      }
      delay(50); continue;
    }

    if (d > 60 || d == DIST_INFINIE) {
      if (tempsObstaclePerdu == 0) tempsObstaclePerdu = millis();
      if (millis() - tempsObstaclePerdu > 100) { arreter(); delay(50); return; }
      avancer(VITESSE_LENTE, VITESSE_LENTE);
    } else {
      tempsObstaclePerdu = 0;
      long err = d - 30;
      if (abs(err) < 5)   avancer(VITESSE_LENTE,     VITESSE_LENTE);
      else if (err > 0)   avancer(VITESSE_LENTE - 8, VITESSE_LENTE + 8);
      else                avancer(VITESSE_LENTE + 8, VITESSE_LENTE - 8);
    }
    delay(50);
  }
  arreter(); delay(50);
}

void evitementGauche() {
  Serial.println(F("=== O1 GAUCHE ==="));
  majLcd();
  servoEcrire(SERVO_AVANT); delay(200);
  reculer(VITESSE_LENTE, DIST_RECUL_INIT * DUREE_PAR_CM);

  Serial.println(F("pivot G"));
  tournerGauche(DUREE_TOURNE_90_G);
  avancerDroit(VITESSE_LENTE, DIST_ECART2);

  Serial.println(F("pivot D + longer"));
  tournerDroite(DUREE_TOURNE_90);
  servoEcrire(SERVO_DROITE); delay(400);
  longerObstacleParDroite();

  Serial.println(F("retour"));
  servoEcrire(SERVO_AVANT); delay(200);
  tournerDroite(DUREE_TOURNE_80);

  Serial.println(F("cherche ligne"));
  tunnelExitTime = millis();
  chercherLigne(5000UL);
  suivreLigneImmediatement(1500UL);

  apresObstacle = true;
  nbObstaclesEvites++;
  etatRobot = SUIVI_LIGNE;
  compteurLignePerdue = 0;
  tunnelExitTime = millis();
}

void evitementDroite() {
  Serial.println(F("=== O1 DROITE ==="));
  majLcd();
  servoEcrire(SERVO_AVANT); delay(200);
  reculer(VITESSE_LENTE, DIST_RECUL_INIT * DUREE_PAR_CM);

  Serial.println(F("pivot D"));
  tournerDroite(DUREE_TOURNE_90);
  avancerDroit(VITESSE_LENTE, DIST_ECART * DUREE_PAR_CM);

  Serial.println(F("pivot G + longer"));
  tournerGauche(DUREE_TOURNE_90);
  servoEcrire(SERVO_GAUCHE); delay(400);
  longerObstacleParGauche();

  Serial.println(F("retour"));
  servoEcrire(SERVO_AVANT); delay(200);
  tournerGauche(DUREE_TOURNE_90_G);

  Serial.println(F("cherche ligne"));
  tunnelExitTime = millis();
  chercherLigne(5000UL);
  suivreLigneImmediatement(1500UL);

  apresObstacle = true;
  nbObstaclesEvites++;
  etatRobot = SUIVI_LIGNE;
  compteurLignePerdue = 0;
  tunnelExitTime = millis();
}

void allumerLED(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUMPIXELS; i++)
    strip.setPixelColor(i, strip.Color(r, g, b));
  strip.show();
}

void gererCouleur() {
  Serial.println(F("-> COULEUR"));
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  Serial.print("R:"); Serial.print(r);
  Serial.print(" G:"); Serial.print(g);
  Serial.print(" B:"); Serial.println(b);

  uint8_t ledR = 0, ledG = 0, ledB = 0;
  if      (r > g && r > b) { ledR = 255;                         strncpy(nomCouleur, "ROUGE           ", 17); Serial.println(F("ROUGE")); }
  else if (g > r && g > b) { ledG = 255;                         strncpy(nomCouleur, "VERT            ", 17); Serial.println(F("VERT"));  }
  else if (b > r && b > g) { ledB = 255;                         strncpy(nomCouleur, "BLEU            ", 17); Serial.println(F("BLEU"));  }
  else                     { ledR = 255; ledG = 255; ledB = 255; strncpy(nomCouleur, "BLANC           ", 17); Serial.println(F("BLANC")); }
  majLcd();

  for (int i = 0; i < 3; i++) {
    allumerLED(ledR, ledG, ledB); delay(500);
    allumerLED(0, 0, 0);          delay(500);
  }

  apresDetectionCouleur = true;
  etatRobot = DEMI_TOUR;
}

void gererDemiTour() {
  Serial.println(F("-> DEMI-TOUR"));
  majLcd();
  reculer(VITESSE_MONTEE, RECUL_MS);
  piloterMoteur(MOTEUR_G, ARRIERE, VITESSE_MONTEE);
  piloterMoteur(MOTEUR_D, ARRIERE, VITESSE_MONTEE);
  delay(DEMI_TOUR_MS);
  arreter(); delay(50);
  tunnelExitTime = millis();
  etatRobot = REPRENDRE_LIGNE;
}

void gererRepriseLigne() {
  int position = lirePosition();
  if (position != -1) {
    Serial.println(F("-> LIGNE"));
    arreter(); delay(100);
    suivreLigneImmediatement(1000UL);
    tunnelExitTime = millis();
    compteurLignePerdue = 0;
    compteurArrivee = 0;
    etatRobot = SUIVI_LIGNE;
    return;
  }
  avancer(VITESSE_MONTEE, VITESSE_MONTEE);
  delay(5);
}
