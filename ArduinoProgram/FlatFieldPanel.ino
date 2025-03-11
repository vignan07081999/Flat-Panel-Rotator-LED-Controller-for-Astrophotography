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
    inputString.reserve(20); // Reserve space for input string

    // --- DTR Reset Prevention (Optional but Recommended)---
    // On some Arduinos (like Uno), connecting via serial resets the board.
    // This delay gives the Arduino time to finish booting before the
    // Python script starts sending commands.  Adjust as needed.
    delay(2000); // Wait for serial connection to stabilize
}

void loop() {
    serialEvent();  // Call serialEvent in loop()

    if (stringComplete) {
        processCommand(inputString);
        inputString = ""; // Clear the string
        stringComplete = false;
    }
}


void processCommand(String command) {
    int separatorIndex = command.indexOf(':');
    if (separatorIndex == -1) {
        // Invalid command format.  Send an error response.
        Serial.println("ERR:Invalid command format");
        return;
    }

    String commandType = command.substring(0, separatorIndex);
    String commandValueStr = command.substring(separatorIndex + 1);

    if (commandType == "SERVO") {
        int commandValue = commandValueStr.toInt(); // Convert to int only for SERVO
        if (commandValue >= 0 && commandValue <= 180) {
            moveServo(servoPos, commandValue);  // Use moveServo for controlled movement
            servoPos = commandValue; // Update *after* movement
            EEPROM.write(servoPosAddress, servoPos);
            Serial.print("OK:SERVO:");
            Serial.println(servoPos);
        } else {
            Serial.println("ERR:Invalid servo position");
        }

    } else if (commandType == "LED") {
        int commandValue = commandValueStr.toInt(); // Convert to int only for LED
        if (commandValue >= 0 && commandValue <= ledBrightnessMax) {
            ledBrightness = commandValue;
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:LED:");
            Serial.println(ledBrightness);
        } else {
            Serial.println("ERR:Invalid LED brightness");
        }

    } else if (commandType == "PRESET") { // Handle PRESET commands
        if (commandValueStr == "OPEN_OFF") {
            moveServo(servoPos, 50); // Open to position 50
            servoPos = 50;
            ledBrightness = 0; // Turn LED off
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:1:SERVO:50:LED:0"); // Consistent feedback
            Serial.println();
        }
         else {
          applyPreset(commandValueStr.toInt()); //Handle other presets
        }
    }
      else if (commandType == "GET") {
        if (commandValueStr == "SERVO") {
            Serial.print("OK:SERVO:");
            Serial.println(servoPos);  // Send current servo position
        } else if (commandValueStr == "LED") {
            Serial.print("OK:LED:");
            Serial.println(ledBrightness);  // Send current LED brightness
        } else {
            Serial.println("ERR:Invalid GET parameter");
        }
    } else {
        Serial.print("ERR:Unknown command: ");
        Serial.println(command);
    }
}


void moveServo(int startPos, int endPos) {
    int step = (endPos > startPos) ? 1 : -1;
    int increment = abs(endPos - startPos) * (100 - servoSpeed) / 100;
    if (increment == 0) increment = 1; // Ensure at least one step

    for (int pos = startPos; ; pos += step) {
        myServo.write(pos);
        delay(servoDelay);

        if (abs(pos - startPos) >= increment || pos == endPos) {
            myServo.write(pos); // Ensure final position is reached
        }

        if (pos == endPos) {
            break; // Exit loop when target position is reached
        }
    }
}
void applyPreset(int presetNumber) {
    int newServoPos;
    switch (presetNumber) {
        case 1: // Not Used - Handled by OPEN_OFF
            return;
        case 2: // "Medium" (Optional - You can add functionality here)
            newServoPos = 90;
            ledBrightness = 128;
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
            newServoPos = 180; // Set to 180 degrees
            ledBrightness = 0; // Turn LED off
            moveServo(servoPos, newServoPos);
            servoPos = newServoPos;
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:3:SERVO:180:LED:0"); //Consistent feedback
            Serial.println();
            break;

        default:
            Serial.println("ERR:Invalid preset number");
            return;
    }
}
// serialEvent() is crucial for non-blocking serial reading
void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        inputString += inChar;
        if (inChar == '\n') {
            stringComplete = true;
        }
    }
}
