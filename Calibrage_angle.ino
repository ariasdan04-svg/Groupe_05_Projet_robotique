#include <Wire.h>
#include <Servo.h>

#define PIN_SERVO    3
#define PIN_ULTRASON 4
#define SERVO_OFFSET 15

#define SERVO_AVANT   (90 - SERVO_OFFSET)
#define SERVO_GAUCHE  (180 - SERVO_OFFSET)
#define SERVO_DROITE  (0 + SERVO_OFFSET)

Servo servoUs;

long mesureUltrason() {
  pinMode(PIN_ULTRASON, OUTPUT);
  digitalWrite(PIN_ULTRASON, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASON, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_ULTRASON, LOW);
  pinMode(PIN_ULTRASON, INPUT);
  long d = pulseIn(PIN_ULTRASON, HIGH, 30000UL);
  return (d == 0) ? 999 : d / 58;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  servoUs.attach(PIN_SERVO);
  Serial.println("Commandes : G=gauche, D=droite, C=centre, ou tape un angle 0-180");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    int angle = -1;
    
    if (input == "G" || input == "g") {
      angle = SERVO_GAUCHE;
      Serial.print("→ GAUCHE ");
    } else if (input == "D" || input == "d") {
      angle = SERVO_DROITE;
      Serial.print("→ DROITE ");
    } else if (input == "C" || input == "c") {
      angle = SERVO_AVANT;
      Serial.print("→ CENTRE ");
    } else {
      angle = input.toInt();
      Serial.print("→ Angle brut ");
    }

    if (angle >= 0 && angle <= 180) {
      servoUs.write(angle);
      delay(500);
      long dist = mesureUltrason();
      Serial.print("("); Serial.print(angle);
      Serial.print("°) | Distance: "); Serial.print(dist);
      Serial.println(" cm");
    }
  }
}