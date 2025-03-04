#include <Servo.h>
#include <EEPROM.h>

Servo panelServo;
const int servoPin = 9;
const int ledPin = 5;
int servoPosition = 0;
int ledBrightness = 0;

void setup() {
    Serial.begin(9600);
    panelServo.attach(servoPin);
    
    // Load last saved states from EEPROM
    servoPosition = EEPROM.read(0);
    ledBrightness = EEPROM.read(1);
    
    panelServo.write(servoPosition);
    analogWrite(ledPin, ledBrightness);
    Serial.println("Arduino Ready");
    Serial.println("SERVO " + String(servoPosition));
    Serial.println("LED " + String(ledBrightness));
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.startsWith("SERVO")) {
            servoPosition = command.substring(6).toInt();
            panelServo.write(servoPosition);
            EEPROM.write(0, servoPosition);
            Serial.println("SERVO " + String(servoPosition));
        }
        else if (command.startsWith("LED")) {
            ledBrightness = command.substring(4).toInt();
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(1, ledBrightness);
            Serial.println("LED " + String(ledBrightness));
        }
        else {
            Serial.println("INVALID COMMAND");
        }
    }
}
