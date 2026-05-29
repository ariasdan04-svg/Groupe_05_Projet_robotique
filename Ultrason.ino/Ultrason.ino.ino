#include "Ultrasonic.h"

Ultrasonic capteurSon(3);

long distance;

void setup() {
    Serial.begin(9600);
    Serial.println("Test capteur ultrasons...");
}

void loop() {
    distance = capteurSon.MeasureInCentimeters();
    
    Serial.print("Distance : ");
    Serial.print(distance);
    Serial.println(" cm");
    
    delay(500);
}
