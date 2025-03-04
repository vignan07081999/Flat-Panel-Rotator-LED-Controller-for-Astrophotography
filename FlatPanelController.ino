#include <Servo.h>
#include <EEPROM.h>

// Define pins
#define SERVO_PIN 9
#define LED_PIN 5

// Create servo object
Servo panelServo;

// Variables to store current values
int servoPosition = 90; // Default middle position
int ledBrightness = 128; // Default brightness (out of 255)

void setup() {
    Serial.begin(9600);
    panelServo.attach(SERVO_PIN);
    pinMode(LED_PIN, OUTPUT);
    
    // Retrieve stored values from EEPROM
    servoPosition = EEPROM.read(0);
    ledBrightness = EEPROM.read(1);
    
    panelServo.write(servoPosition);
    analogWrite(LED_PIN, ledBrightness);
    
    Serial.println("Arduino Flat Panel Ready");
    Serial.print("Servo Position: "); Serial.println(servoPosition);
    Serial.print("LED Brightness: "); Serial.println(ledBrightness);
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.startsWith("SERVO")) {
            int pos = input.substring(6).toInt();
            if (pos >= 0 && pos <= 180) {
                servoPosition = pos;
                panelServo.write(servoPosition);
                EEPROM.write(0, servoPosition);
                Serial.print("Servo Moved To: "); Serial.println(servoPosition);
            }
        }
        
        else if (input.startsWith("LED")) {
            int brightness = input.substring(4).toInt();
            if (brightness >= 0 && brightness <= 255) {
                ledBrightness = brightness;
                analogWrite(LED_PIN, ledBrightness);
                EEPROM.write(1, ledBrightness);
                Serial.print("LED Brightness Set To: "); Serial.println(ledBrightness);
            }
        }
        
        else if (input == "STATUS") {
            Serial.print("Servo: "); Serial.println(servoPosition);
            Serial.print("LED: "); Serial.println(ledBrightness);
        }
    }
}
