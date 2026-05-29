#include <Wire.h>

#define CAPTEUR_LIGNE 0x20

void setup() {
    Wire.begin();
    Serial.begin(9600);
    Serial.println("Test détection ligne...");
}

void loop() {
    Wire.requestFrom(CAPTEUR_LIGNE, 2);
    if (Wire.available() >= 2) {
        int high = Wire.read();
        int low  = Wire.read();
        int position = (high << 8) | low;

        Serial.print("Valeur : ");
        Serial.print(position);

        if (position != 0) {
            Serial.println(" → LIGNE DETECTEE ✓");
        } else {
            Serial.println(" → RIEN ✗");
        }
    }
    delay(300);
}