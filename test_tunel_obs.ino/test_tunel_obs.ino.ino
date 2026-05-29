#include <Wire.h>
#include <Servo.h>

#define PIN_SERVO     3
#define PIN_ULTRASON  4

#define MOTEUR_G      0x66
#define MOTEUR_D      0x68
#define CAPTEUR_LIGNE 0x20

#define ARRET         0x00
#define AVANT         0x01
#define ARRIERE       0x02
#define FREIN         0x03

#define SERVO_AVANT   90
#define SERVO_GAUCHE  180
#define SERVO_DROITE  0

#define VITESSE_BASE  30
#define KP            0.0015
#define ZONE_MORTE    5000

#define VITESSE_LENTE  35
#define VITESSE_TUNNEL 30

#define DIST_OBSTACLE      15
#define DIST_INFINIE       9999
#define DIST_MUR_FRONTAL   25
#define DIST_CORRECTION_TUNNEL 90
#define DIST_TRES_PROCHE_TUNNEL 8

#define TEMPS_LIGNE_PERDUE_MS 700
#define SEUIL_SORTIE_TUNNEL   3

#define DUREE_TOURNE_90             880 //  durée fonctionne 
#define DUREE_TOURNE_90_RETOUR_O1   999 // durée bien 
#define DUREE_TOURNE_90_RETOUR_O2   800
#define DUREE_PAR_CM       60
#define DIST_RECUL_INIT    8
#define DIST_ECART         28
#define DIST_APPROCHE      20
#define DIST_DEPASSEMENT   10

#define DIST_ECART_O2       70
#define DIST_DEPASSEMENT_O2 55

#define DIST_OBST_CIBLE    25
#define DIST_OBST_PROCHE   55
#define DUREE_MAX_LONGER   8000UL

#define TEMPS_STABILISATION_O1 900

Servo servoUs;

enum EtatRobot {
    SUIVI_LIGNE,
    TUNNEL,
    EVITEMENT_GAUCHE,
    EVITEMENT_DROITE
};

EtatRobot etatRobot = SUIVI_LIGNE;

bool tunnelDejaPasse    = false;
bool enPerteLigne       = false;
bool stabilisationO1    = false;

unsigned long debutPerteLigne      = 0;
unsigned long debutStabilisationO1 = 0;

int compteurLigneRetrouvee = 0;
int nbObstaclesEvites      = 0;

// ================= MOTEURS =================
void piloterMoteur(byte adresse, byte direction, byte vitesse) {
    vitesse = constrain(vitesse, 0, 63);
    Wire.beginTransmission(adresse);
    Wire.write(0x00);
    Wire.write((vitesse << 2) | direction);
    Wire.endTransmission();
}

void avancerDiff(int vitG, int vitD) {
    vitG = constrain(vitG, 0, 63);
    vitD = constrain(vitD, 0, 63);
    piloterMoteur(MOTEUR_G, AVANT,   vitG);
    piloterMoteur(MOTEUR_D, ARRIERE, vitD);
}

void arreter() {
    piloterMoteur(MOTEUR_G, FREIN, 0);
    piloterMoteur(MOTEUR_D, FREIN, 0);
}

void avancerDroit(int vitesse, unsigned long duree) {
    avancerDiff(vitesse, vitesse);
    delay(duree);
    arreter();
    delay(50);
}

void tournerGauche(unsigned long duree) {
    piloterMoteur(MOTEUR_G, AVANT, VITESSE_LENTE);
    piloterMoteur(MOTEUR_D, AVANT, VITESSE_LENTE);
    delay(duree);
    arreter();
    delay(50);
}

void tournerDroite(unsigned long duree) {
    piloterMoteur(MOTEUR_G, ARRIERE, VITESSE_LENTE);
    piloterMoteur(MOTEUR_D, ARRIERE, VITESSE_LENTE);
    delay(duree);
    arreter();
    delay(50);
}

void reculer(int vitesse, unsigned long duree) {
    piloterMoteur(MOTEUR_G, ARRIERE, vitesse);
    piloterMoteur(MOTEUR_D, AVANT,   vitesse);
    delay(duree);
    arreter();
    delay(50);
}

// ================= ULTRASON =================
long mesureUltrason() {
    pinMode(PIN_ULTRASON, OUTPUT);
    digitalWrite(PIN_ULTRASON, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRASON, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASON, LOW);
    pinMode(PIN_ULTRASON, INPUT);
    long duree = pulseIn(PIN_ULTRASON, HIGH, 30000UL);
    if (duree == 0) return DIST_INFINIE;
    return duree / 58;
}

// ================= LIGNE =================
uint8_t lireEtatLigne() {
    Wire.beginTransmission(CAPTEUR_LIGNE);
    Wire.write(0x07);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)CAPTEUR_LIGNE, (uint8_t)1);
    if (!Wire.available()) return 0xFF;
    uint8_t brut = Wire.read();
    return (~brut) & 0x0F;
}

bool ligneDetecteeTunnel(uint8_t etat) {
    if (etat == 0xFF)   return false;
    if (etat == 0b1111) return false;
    if (etat == 0b0000) return false;
    return true;
}

int convertirEtatEnPosition(uint8_t etat) {
    switch (etat) {
        case 0b1001:
        case 0b0110:  return 1000;
        case 0b1000:
        case 0b1100:  return -20000;
        case 0b0001:
        case 0b0011:  return 20000;
        case 0b1110:  return -30000;
        case 0b0111:  return 30000;
        case 0b1111:  return 0;
        case 0b0000:  return 1000;
        default:      return 1000;
    }
}

// ================= SUIVI LIGNE =================
void suivreLigne() {
    uint8_t etat = lireEtatLigne();

    if (stabilisationO1) {
        if (millis() - debutStabilisationO1 < TEMPS_STABILISATION_O1) {
            if (etat == 0b1111 || etat == 0xFF) {
                avancerDiff(VITESSE_BASE, VITESSE_BASE);
                delay(30);
                return;
            }
        } else {
            stabilisationO1 = false;
        }
    }

    if (etat == 0b1111 || etat == 0xFF) {
        if (!enPerteLigne) {
            debutPerteLigne = millis();
            enPerteLigne    = true;
        }

        avancerDiff(VITESSE_BASE, VITESSE_BASE);

        if (!tunnelDejaPasse && millis() - debutPerteLigne >= TEMPS_LIGNE_PERDUE_MS) {
            arreter();
            delay(100);
            etatRobot    = TUNNEL;
            enPerteLigne = false;
            return;
        }

        delay(30);
        return;
    }

    enPerteLigne = false;

    int position   = convertirEtatEnPosition(etat);
    int correction = (int)(KP * position);

    if (abs(position) < ZONE_MORTE) {
        avancerDiff(VITESSE_BASE, VITESSE_BASE);
    } else {
        int vitG = constrain(VITESSE_BASE - correction, 0, 63);
        int vitD = constrain(VITESSE_BASE + correction, 0, 63);
        avancerDiff(vitG, vitD);
    }

    delay(30);
}

// ================= TUNNEL =================
void gererTunnel() {
    int angleServo = SERVO_AVANT;
    int sensServo  = 1;

    long distG     = 50;
    long distD     = 50;
    long distAvant = 50;

    compteurLigneRetrouvee = 0;

    servoUs.write(SERVO_AVANT);
    delay(200);

    while (etatRobot == TUNNEL) {
        uint8_t etat = lireEtatLigne();

        if (ligneDetecteeTunnel(etat)) {
            compteurLigneRetrouvee++;
            avancerDiff(VITESSE_BASE, VITESSE_BASE);

            if (compteurLigneRetrouvee >= SEUIL_SORTIE_TUNNEL) {
                arreter();
                servoUs.write(SERVO_AVANT);
                delay(200);
                tunnelDejaPasse = true;
                etatRobot       = SUIVI_LIGNE;
                return;
            }

            delay(30);
            continue;
        } else {
            compteurLigneRetrouvee = 0;
        }

        servoUs.write(angleServo);
        delay(25);
        long distance = mesureUltrason();

        if (angleServo < 60)       distD     = distance;
        else if (angleServo > 120) distG     = distance;
        else                       distAvant = distance;

        angleServo += sensServo * 15;
        if (angleServo >= SERVO_GAUCHE) { angleServo = SERVO_GAUCHE; sensServo = -1; }
        if (angleServo <= SERVO_DROITE) { angleServo = SERVO_DROITE; sensServo =  1; }

        if (distAvant < DIST_MUR_FRONTAL) {
            arreter();
            delay(80);
            if (distG > distD) tournerGauche(550);
            else               tournerDroite(550);
            distAvant = 50;
            continue;
        }

        long erreur = distG - distD;

        if (distG <= DIST_TRES_PROCHE_TUNNEL) {
            avancerDiff(VITESSE_TUNNEL + 8, VITESSE_TUNNEL - 8);
        }
        else if (distD <= DIST_TRES_PROCHE_TUNNEL) {
            avancerDiff(VITESSE_TUNNEL - 8, VITESSE_TUNNEL + 8);
        }
        else if (distG < DIST_CORRECTION_TUNNEL || distD < DIST_CORRECTION_TUNNEL) {
            if      (erreur >  3) avancerDiff(VITESSE_TUNNEL - 10, VITESSE_TUNNEL + 10);
            else if (erreur < -3) avancerDiff(VITESSE_TUNNEL + 10, VITESSE_TUNNEL - 10);
            else                  avancerDiff(VITESSE_TUNNEL,       VITESSE_TUNNEL);
        }
        else {
            avancerDiff(VITESSE_TUNNEL, VITESSE_TUNNEL);
        }

        delay(30);
    }

    servoUs.write(SERVO_AVANT);
}

// ================= OBSTACLES =================
bool chercherLigne(unsigned long timeoutMs) {
    avancerDiff(VITESSE_LENTE, VITESSE_LENTE);
    unsigned long debut = millis();
    while (millis() - debut < timeoutMs) {
        uint8_t etat = lireEtatLigne();
        if (etat != 0b1111 && etat != 0xFF) return true;
        delay(15);
    }
    arreter();
    return false;
}

void longerObstacle(bool parDroite) {
    unsigned long debut      = millis();
    bool          obstacleVu = false;
    unsigned long tempsPerdu = 0;

    while (millis() - debut < DUREE_MAX_LONGER) {
        long dist = mesureUltrason();

        if (!obstacleVu) {
            if (dist < DIST_OBST_PROCHE && dist > 5) {
                obstacleVu = true;
            } else {
                avancerDiff(VITESSE_LENTE, VITESSE_LENTE);
                if (millis() - debut > 2000) return;
            }
            delay(50);
            continue;
        }

        if (dist > DIST_OBST_PROCHE) {
            if (tempsPerdu == 0) tempsPerdu = millis();
            if (millis() - tempsPerdu > 300) return;
            avancerDiff(VITESSE_LENTE, VITESSE_LENTE);
        } else {
            tempsPerdu = 0;
            long err   = dist - DIST_OBST_CIBLE;
            if (abs(err) < 5) {
                avancerDiff(VITESSE_LENTE, VITESSE_LENTE);
            } else if (parDroite) {
                if (err > 0) avancerDiff(VITESSE_LENTE + 3, VITESSE_LENTE - 3);
                else         avancerDiff(VITESSE_LENTE - 3, VITESSE_LENTE + 3);
            } else {
                if (err > 0) avancerDiff(VITESSE_LENTE - 3, VITESSE_LENTE + 3);
                else         avancerDiff(VITESSE_LENTE + 3, VITESSE_LENTE - 3);
            }
        }
        delay(50);
    }
    arreter();
}

// ================= EVITEMENT O1 — GAUCHE =================
void evitementGauche() {
    servoUs.write(SERVO_AVANT);
    delay(200);

    // 1. Reculer
    reculer(VITESSE_LENTE, DIST_RECUL_INIT * DUREE_PAR_CM);

    // 2. Tourner gauche
    tournerGauche(DUREE_TOURNE_90);

    // 3. Avancer à gauche de l'obstacle
    avancerDroit(VITESSE_LENTE, DIST_ECART * DUREE_PAR_CM);

    // 4. Tourner droite pour longer
    tournerDroite(DUREE_TOURNE_90);

    // 5. Avancer pour approcher
    avancerDroit(VITESSE_LENTE, DIST_APPROCHE * DUREE_PAR_CM);

    // 6. Longer obstacle capteur droite
    servoUs.write(SERVO_DROITE);
    delay(400);
    longerObstacle(true);

    // 7. Avancer pour dépasser
    servoUs.write(SERVO_AVANT);
    delay(200);
    avancerDroit(VITESSE_LENTE, DIST_DEPASSEMENT * DUREE_PAR_CM);

    // 8. Tourner droite retour
    tournerDroite(DUREE_TOURNE_90_RETOUR_O1);

    // 9. Chercher ligne
    chercherLigne(5000);

    stabilisationO1      = true;
    debutStabilisationO1 = millis();
    nbObstaclesEvites++;
    etatRobot = SUIVI_LIGNE;
    servoUs.write(SERVO_AVANT);
}

// ================= EVITEMENT O2 — DROITE =================
// ✅ Même logique triangulaire miroir de O1
void evitementDroite() {
    servoUs.write(SERVO_AVANT);
    delay(200);

    // 1. Reculer
    reculer(VITESSE_LENTE, DIST_RECUL_INIT * DUREE_PAR_CM);

    // 2. Tourner droite
    tournerDroite(DUREE_TOURNE_90);

    // 3. Avancer à droite de l'obstacle
    avancerDroit(VITESSE_LENTE, DIST_ECART_O2 * DUREE_PAR_CM);

    // 4. Tourner gauche pour longer
    tournerGauche(DUREE_TOURNE_90);

    // 5. Avancer pour approcher
    avancerDroit(VITESSE_LENTE, DIST_APPROCHE * DUREE_PAR_CM);

    // 6. Longer obstacle capteur gauche
    servoUs.write(SERVO_GAUCHE);
    delay(400);
    longerObstacle(false);

    // 7. Avancer pour dépasser
    servoUs.write(SERVO_AVANT);
    delay(200);
    avancerDroit(VITESSE_LENTE, DIST_DEPASSEMENT_O2 * DUREE_PAR_CM);

    // 8. Tourner gauche retour (grand angle)
    tournerGauche(DUREE_TOURNE_90_RETOUR_O2);

    // 9. Chercher ligne
    chercherLigne(6000);

    nbObstaclesEvites++;
    etatRobot = SUIVI_LIGNE;
    servoUs.write(SERVO_AVANT);
}

// ================= SETUP / LOOP =================
void setup() {
    Wire.begin();
    Serial.begin(9600);
    servoUs.attach(PIN_SERVO);
    servoUs.write(SERVO_AVANT);
    arreter();
    delay(500);
    Serial.println("GO");
}

void loop() {
    switch (etatRobot) {
        case SUIVI_LIGNE: {
            if (tunnelDejaPasse) {
                long distance = mesureUltrason();
                if (distance < DIST_OBSTACLE) {
                    arreter();
                    delay(200);
                    if (nbObstaclesEvites == 0) etatRobot = EVITEMENT_GAUCHE;
                    else                        etatRobot = EVITEMENT_DROITE;
                    return;
                }
            }
            suivreLigne();
            break;
        }
        case TUNNEL:
            gererTunnel();
            break;
        case EVITEMENT_GAUCHE:
            evitementGauche();
            break;
        case EVITEMENT_DROITE:
            evitementDroite();
            break;
    }
}