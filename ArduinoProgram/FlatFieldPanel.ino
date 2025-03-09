#include <Servo.h>
#include <EEPROM.h>

// --- Pin Definitions ---
const int servoPin = 9;      // Servo signal pin
const int ledPin = 10;       // LED strip

// --- Servo and LED Variables ---
Servo myServo;
int servoPos = 90;          // Initial position for servo
int ledBrightness = 128;
int ledBrightnessMax = 255;

// --- Servo Speed Control ---
const int servoSpeed = 40;  // Percentage of full speed (1-100)
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
  if (servoPos < 0 || servoPos > 180) servoPos = 90; // constrain
  myServo.write(servoPos);

  ledBrightness = EEPROM.read(ledBrightnessAddress);
  if (ledBrightness < 0 || ledBrightness > ledBrightnessMax) ledBrightness = 128; //constrain
  analogWrite(ledPin, ledBrightness);

  Serial.begin(baudRate);
  inputString.reserve(20);
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
    applyPreset(commandValue);

  } else if (commandType == "GET") {
       if(commandValueStr == "SERVO"){
            Serial.print("OK:SERVO:");
            Serial.println(servoPos);
          }else if(commandValueStr == "LED"){
            Serial.print("OK:LED:");
            Serial.println(ledBrightness);
          }
  }

  else {
    Serial.print("ERR:Unknown command: ");
    Serial.println(command);
  }
}

void applyPreset(int presetNumber) {
    int newServoPos;
    switch (presetNumber) {
        case 1: // "Open"
            newServoPos = 0;
            ledBrightness = 50;
            break;
        case 2:  // "Medium"
            newServoPos = 90;
            ledBrightness = 150;
            break;
        case 3: // "Close"
            newServoPos = 180;
            ledBrightness = 255;
            break;
        default:
            Serial.println("ERR:Invalid preset number");
            return;
    }

    moveServo(servoPos, newServoPos);
    servoPos = newServoPos;  // Update stored position

    analogWrite(ledPin, ledBrightness);
    EEPROM.write(servoPosAddress, servoPos);
    EEPROM.write(ledBrightnessAddress, ledBrightness);
    Serial.print("OK:PRESET:");
    Serial.print(presetNumber);
    Serial.print(":SERVO:");
    Serial.print(servoPos);
    Serial.print(":LED:");
    Serial.println(ledBrightness);
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
