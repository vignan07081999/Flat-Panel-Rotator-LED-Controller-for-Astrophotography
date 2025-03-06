#include <Servo.h>
#include <EEPROM.h>

// --- Pin Definitions ---
const int servoPin = 9;       // Digital pin connected to the servo
const int ledPin = 10;       // Digital pin connected to the LED strip (must be PWM)

// --- Servo and LED Variables ---
Servo myServo;
int servoPos = 90;          // Initial servo position (degrees)
int ledBrightness = 128;    // Initial LED brightness (0-255)
int ledBrightnessMax = 255; //maximim led strip brightness

// --- EEPROM Addresses ---
const int servoPosAddress = 0;
const int ledBrightnessAddress = 2; // Use address 2, as int might take 2 bytes

// --- Serial Communication ---
const int baudRate = 9600;  // Match this to your Python code
String inputString = "";         // A String to hold incoming data
bool stringComplete = false;  // Whether the string is complete


void setup() {
  myServo.attach(servoPin);
  pinMode(ledPin, OUTPUT);

  // --- Load settings from EEPROM ---
  servoPos = EEPROM.read(servoPosAddress);
  if (servoPos < 0 || servoPos > 180) servoPos = 90;  // Sanity check
   myServo.write(servoPos);

    ledBrightness = EEPROM.read(ledBrightnessAddress);
    if (ledBrightness < 0 || ledBrightness > ledBrightnessMax) ledBrightness = 128; // Sanity check
    analogWrite(ledPin, ledBrightness);

  Serial.begin(baudRate);
  inputString.reserve(20); // Reserve space for incoming serial data
}

void loop() {
    //Serial Event Handling
    serialEvent();
    
    if (stringComplete) {
        processCommand(inputString);
        inputString = ""; // Clear the string
        stringComplete = false;
    }

}


void processCommand(String command) {
  // --- Command Parsing ---
  // Commands will be in the format: "COMMAND:VALUE"
  // Examples:  "SERVO:120", "LED:200", "PRESET:1"

    int separatorIndex = command.indexOf(':');
    if (separatorIndex == -1) return; // Invalid command

    String commandType = command.substring(0, separatorIndex);
    String commandValueStr = command.substring(separatorIndex + 1);
    int commandValue = commandValueStr.toInt();


  if (commandType == "SERVO") {
    if (commandValue >= 0 && commandValue <= 180) {
      servoPos = commandValue;
      myServo.write(servoPos);
      EEPROM.write(servoPosAddress, servoPos);
        Serial.print("OK:SERVO:");
        Serial.println(servoPos);  // Send feedback
    } else {
      Serial.println("ERR:Invalid servo position");
    }

  } else if (commandType == "LED") {
    if (commandValue >= 0 && commandValue <= ledBrightnessMax) {
      ledBrightness = commandValue;
      analogWrite(ledPin, ledBrightness);
      EEPROM.write(ledBrightnessAddress, ledBrightness);
        Serial.print("OK:LED:");
        Serial.println(ledBrightness); // Send feedback
    } else {
      Serial.println("ERR:Invalid LED brightness");
    }

  } else if (commandType == "PRESET") {
      applyPreset(commandValue);

  }  else if (commandType == "GET") { //Added to handle get property requests from INDI
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
    // Define your presets here.  Expand as needed.
    switch (presetNumber) {
        case 1:
            servoPos = 0;
            ledBrightness = 50;
            break;
        case 2:
            servoPos = 90;
            ledBrightness = 150;
            break;
        case 3:
            servoPos = 180;
            ledBrightness = 255;
            break;
        default:
            Serial.println("ERR:Invalid preset number");
            return;
    }

    myServo.write(servoPos);
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

//Serial Event for receiving data
void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        inputString += inChar;
        if (inChar == '\n') {
            stringComplete = true;
        }
    }
}
