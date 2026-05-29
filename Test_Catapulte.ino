#include <Wire.h>

#define CATAPULTE 0x60

#define ARRET  0x00
#define AVANT  0x01
#define ARRIERE 0x02

void piloterMoteur(byte adresse, byte direction, byte vitesse) {
  if (vitesse > 63) vitesse = 63;
  byte commande = (vitesse << 2) | direction;
  Wire.beginTransmission(adresse);
  Wire.write(0x00);
  Wire.write(commande);
  Wire.endTransmission();
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  piloterMoteur(CATAPULTE, AVANT, 50);
  delay(2000);
  piloterMoteur(CATAPULTE, ARRET, 0);
  while(true);
}

void loop() {}