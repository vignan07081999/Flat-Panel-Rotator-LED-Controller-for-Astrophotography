#include <Servo.h>
#include <EEPROM.h>

// --- Pin Definitions ---
const int servoPin = 9;      // Servo signal pin
const int ledPin = 3;       // LED strip (Use pin 3 for higher PWM freq)

// --- Servo and LED Variables ---
Servo myServo;
int servoPos = 90;          // Initial position for servo
int ledBrightness = 0;     //Initial LED Brightness
int ledBrightnessMax = 255;

// --- Servo Speed Control ---
const int servoSpeed = 40; // Percentage of full speed (1-100)
const int servoDelay = 10; // Delay in milliseconds between steps

// --- EEPROM Addresses ---
const int servoPosAddress = 0;
const int ledBrightnessAddress = 2;

// --- Serial Communication ---
const int baudRate = 9600;
String inputString = "";
bool stringComplete = false;

void setup() {
    myServo.attach(servoPin);
    pinMode(ledPin, OUTPUT);

    // --- Load settings from EEPROM ---
    servoPos = EEPROM.read(servoPosAddress);
    if (servoPos < 0 || servoPos > 180) servoPos = 90;
    myServo.write(servoPos);

    ledBrightness = EEPROM.read(ledBrightnessAddress);
    if (ledBrightness < 0 || ledBrightness > ledBrightnessMax) ledBrightness = 0;
    analogWrite(ledPin, ledBrightness);

    // --- Increase PWM Frequency (for pin 3 , Timer 2) ---
    TCCR2B = TCCR2B & B11111000 | B00000001; // Change PWM frequency on Pin 3

    Serial.begin(baudRate);
    inputString.reserve(20);

    // --- DTR Reset Prevention ---
    delay(2000); // Delay to allow for serial connection stabilization
}

void loop() {
    serialEvent();

    if (stringComplete) {
        processCommand(inputString);
        inputString = "";
        stringComplete = false;
    }
}

void processCommand(String command) {
    int separatorIndex = command.indexOf(':');
    if (separatorIndex == -1) return;

    String commandType = command.substring(0, separatorIndex);
    String commandValueStr = command.substring(separatorIndex + 1);
    int commandValue = commandValueStr.toInt();

    if (commandType == "SERVO") {
        if (commandValue >= 0 && commandValue <= 180) {
            moveServo(servoPos, commandValue);
            servoPos = commandValue; // Update after movement
            EEPROM.write(servoPosAddress, servoPos);
            Serial.print("OK:SERVO:");
            Serial.println(servoPos);
        } else {
            Serial.println("ERR:Invalid servo position");
        }

    } else if (commandType == "LED") {
        if (commandValue >= 0 && commandValue <= ledBrightnessMax) {
            ledBrightness = commandValue;
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:LED:");
            Serial.println(ledBrightness);
        } else {
            Serial.println("ERR:Invalid LED brightness");
        }

    } else if (commandType == "PRESET") {
          if (commandValueStr.startsWith("OPEN_ON")) { // New OPEN_ON command
            moveServo(servoPos, 50); //Open
            servoPos = 50;
            ledBrightness = commandValueStr.substring(8).toInt(); // Extract LED Brightness
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:1:SERVO:50:LED:");  // Consistent feedback
            Serial.println(ledBrightness);

        }
        else if (commandValueStr == "OPEN_OFF"){ //Open with led OFF
             moveServo(servoPos, 50); //Open
            servoPos = 50;
            ledBrightness = 0;
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:1:SERVO:50:LED:0");  // Consistent feedback
            Serial.println();
        }
        else {
          applyPreset(commandValue); //Handle other presets
        }

    } else if (commandType == "GET") {
        if (commandValueStr == "SERVO") {
            Serial.print("OK:SERVO:");
            Serial.println(servoPos);
        } else if (commandValueStr == "LED") {
            Serial.print("OK:LED:");
            Serial.println(ledBrightness);
        }
    } else {
        Serial.print("ERR:Unknown command: ");
        Serial.println(command);
    }
}

void applyPreset(int presetNumber) {
    int newServoPos;
    switch (presetNumber) {
        case 1: //  Not Used
            return; // Do nothing
        case 2:  // "Medium" (Not currently used, but kept for potential future use)
            newServoPos = 90;
            ledBrightness = 150;
             moveServo(servoPos, newServoPos);
            servoPos = newServoPos;
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:2:SERVO:");  // Give specific feedback
            Serial.print(servoPos);
            Serial.print(":LED:");
            Serial.println(ledBrightness);
            break;

        case 3: // "Close"
            newServoPos = 175; // Use 175, as requested
            ledBrightness = 0; // Keep LED off as requested
            moveServo(servoPos, newServoPos);
            servoPos = newServoPos;
            analogWrite(ledPin, ledBrightness); // Keep LED off
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:3:SERVO:175:LED:0"); //Consistent feedback
            Serial.println();
            break;
        default:
            Serial.println("ERR:Invalid preset number");
            return;
    }
}

void moveServo(int startPos, int endPos) {
    int step = (endPos > startPos) ? 1 : -1;
    int increment = abs(endPos - startPos) * (100 - servoSpeed) / 100;
    if (increment == 0) increment = 1;

    for (int pos = startPos; ; pos += step) {
        myServo.write(pos);
        delay(servoDelay);

        if (abs(pos - startPos) >= increment || pos == endPos) {
            myServo.write(pos);
        }

        if (pos == endPos) {
            break;
        }
    }
}

void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        inputString += inChar;
        if (inChar == '\n') {
            stringComplete = true;
        }
    }
}
