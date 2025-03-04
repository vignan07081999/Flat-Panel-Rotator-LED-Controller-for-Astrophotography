#include <Servo.h>
#include <EEPROM.h>

// --- Configuration ---
const int SERVO_PIN = 9;
const int LED_PIN = 10;
const int EEPROM_SERVO_ADDR = 0;  // EEPROM address to store servo position
const int EEPROM_BRIGHTNESS_ADDR = 1; // EEPROM address to store LED brightness

// --- Variables ---
Servo myServo;
int targetServoPos = 90;  // Initial position (e.g., 90 degrees - adjust as needed)
int currentServoPos = 90;
int targetBrightness = 0;   // Initial brightness (0-255)
int currentBrightness = 0;
unsigned long lastCommandTime = 0; //to check inactivity
const unsigned long inactivityTimeout = 5000; // Timeout in milliseconds (5 seconds)

// --- Function Prototypes ---
void setServoPosition(int pos);
void setLEDBrightness(int brightness);
void sendFeedback();
void saveSettingsToEEPROM();
void loadSettingsFromEEPROM();
void processSerialCommand(String command);

void setup() {
  Serial.begin(115200); // Use a higher baud rate for faster communication
  myServo.attach(SERVO_PIN);
  pinMode(LED_PIN, OUTPUT);

  loadSettingsFromEEPROM();
  setServoPosition(targetServoPos);
  setLEDBrightness(targetBrightness);
  sendFeedback(); // Send initial feedback
}

void loop() {
  // --- Check for Serial Commands ---
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove leading/trailing whitespace
    processSerialCommand(command);
    lastCommandTime = millis();
  }
  // Inactivity check
  if (millis() - lastCommandTime > inactivityTimeout) {
    // Optionally, you could put the system into a low-power state here
    // or take some other action if no commands are received for a while.
    // For now, we'll just print a message.
    //Serial.println("System Idle");
    lastCommandTime = millis(); // Reset the timer even if it's our own message
  }

    // --- Update Servo (Smooth Movement) ---
  if (currentServoPos != targetServoPos) {
    if (currentServoPos < targetServoPos) {
      currentServoPos++;
    } else {
      currentServoPos--;
    }
    myServo.write(currentServoPos);
    delay(15); // Small delay for smooth movement
  }

  // --- Update LED Brightness (Smooth Transition) ---
    if (currentBrightness != targetBrightness) {
    if (currentBrightness < targetBrightness) {
      currentBrightness++;
    } else {
      currentBrightness--;
    }
    analogWrite(LED_PIN, currentBrightness);
    delay(5); // Small delay for smooth dimming
  }
}

void processSerialCommand(String command) {
  // --- Parse Commands ---
  // Expected commands:  "Sxxx" (servo), "Lxxx" (LED), "F" (feedback)
  // Example: "S90", "L128", "F"

  char commandType = command.charAt(0);
  String commandValue = command.substring(1);

    if (commandType == 'S') {
    int newServoPos = commandValue.toInt();
        if(newServoPos>=0 && newServoPos <=180) //servo position validity check
        {
            setServoPosition(newServoPos);
            saveSettingsToEEPROM();
        }
        else{
          Serial.println("Invalid Servo Position");
        }
  } else if (commandType == 'L') {
    int newBrightness = commandValue.toInt();
        if(newBrightness>=0 && newBrightness<=255)
        {
           setLEDBrightness(newBrightness);
           saveSettingsToEEPROM();
        }
         else{
          Serial.println("Invalid Brightness Value");
        }
  } else if (commandType == 'F') {
    sendFeedback();
  } else {
    Serial.println("Invalid Command");
  }
}

void setServoPosition(int pos) {
  targetServoPos = pos;
  // currentServoPos will be updated in the loop() for smooth movement
}

void setLEDBrightness(int brightness) {
  targetBrightness = brightness;
  // currentBrightness will be updated in the loop() for smooth transition
}

void sendFeedback() {
  Serial.print("SP:");
  Serial.print(currentServoPos);
  Serial.print(",LB:");
  Serial.println(currentBrightness);
}

void saveSettingsToEEPROM() {
  EEPROM.update(EEPROM_SERVO_ADDR, targetServoPos);
  EEPROM.update(EEPROM_BRIGHTNESS_ADDR, targetBrightness);
}

void loadSettingsFromEEPROM() {
  targetServoPos = EEPROM.read(EEPROM_SERVO_ADDR);
  targetBrightness = EEPROM.read(EEPROM_BRIGHTNESS_ADDR);
    //sanity checks in case of corrupted EEPROM
    if(targetServoPos<0 || targetServoPos>180)
    {
        targetServoPos = 90;
    }
    if(targetBrightness<0 || targetBrightness > 255)
    {
        targetBrightness = 0;
    }
}
