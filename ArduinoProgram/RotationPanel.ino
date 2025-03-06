#include <Servo.h>
#include <EEPROM.h>

// --- Configuration ---
#define SERVO_PIN 9
#define LED_PIN 10      // Use a PWM pin
#define EEPROM_SERVO_ADDR 0
#define EEPROM_LED_ADDR 2
#define BAUD_RATE 115200

// --- Variables ---
Servo myServo;
int servoPos = 90;  // Initial position (degrees)
int ledBrightness = 128; // Initial brightness (0-255)
bool newServoData = false;
bool newLedData = false;

void setup() {
    Serial.begin(BAUD_RATE);
    myServo.attach(SERVO_PIN);
    pinMode(LED_PIN, OUTPUT);

    // Load last known values from EEPROM
    servoPos = EEPROM.read(EEPROM_SERVO_ADDR);
    ledBrightness = EEPROM.read(EEPROM_LED_ADDR);
    //check for valid eeprom values:
    if (servoPos<0 || servoPos>180) { servoPos = 90;}
    if (ledBrightness<0 || ledBrightness>255) {ledBrightness = 128;}
    
    myServo.write(servoPos);
    analogWrite(LED_PIN, ledBrightness);

    Serial.println("Rotation Panel Ready"); // Initial handshake
}

void loop() {
    if (Serial.available() > 0) {
        processIncomingCommand();
    }

    if (newServoData) {
        myServo.write(servoPos);
        EEPROM.write(EEPROM_SERVO_ADDR, servoPos);
        newServoData = false;
        Serial.print("OK:Servo:");
        Serial.println(servoPos);  // Send feedback
    }

    if (newLedData) {
        analogWrite(LED_PIN, ledBrightness);
        EEPROM.write(EEPROM_LED_ADDR, ledBrightness);
        newLedData = false;
        Serial.print("OK:LED:");
        Serial.println(ledBrightness); // Send feedback
    }
    delay(10); // Small delay
}


void processIncomingCommand() {
    String command = Serial.readStringUntil('\n');
    command.trim();  // Remove leading/trailing whitespace

    if (command.startsWith("S")) {  // Servo command (e.g., "S120")
        int newPos = command.substring(1).toInt();

        if (newPos >= 0 && newPos <= 180) {
            servoPos = newPos;
            newServoData = true;
        } else {
            Serial.println("ERR:Invalid Servo Position");
        }

    } else if (command.startsWith("L")) {  // LED command (e.g., "L200")
        int newBrightness = command.substring(1).toInt();

        if (newBrightness >= 0 && newBrightness <= 255) {
            ledBrightness = newBrightness;
            newLedData = true;
        } else {
            Serial.println("ERR:Invalid LED Brightness");
        }
    }
     else if (command.startsWith("GET")) {
        if (command.substring(3).startsWith("S")) {  
            Serial.print("OK:Servo:");
            Serial.println(servoPos); 
        }
        else if (command.substring(3).startsWith("L")) {  
            Serial.print("OK:LED:");
            Serial.println(ledBrightness);
        }
    }
    else {
        Serial.println("ERR:Unknown Command");
    }
}
