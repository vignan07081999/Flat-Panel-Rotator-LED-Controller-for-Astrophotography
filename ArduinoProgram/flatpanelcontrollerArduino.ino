#include <Servo.h>
#include <EEPROM.h>

// --- Pin Definitions ---
const int SERVO_PIN = 9;
const int LED_PIN = 10;

// --- EEPROM Addresses ---
const int EEPROM_BRIGHTNESS_ADDR = 0;
const int EEPROM_SERVO_ADDR = 1;

// --- Variables ---
Servo myServo;
int brightness = 0;
int servoPosition = 180; // Closed initially
bool coverMoving = false; //flag to check servo movement

void setup() {
    Serial.begin(115200); // Consistent baud rate
    myServo.attach(SERVO_PIN);
    pinMode(LED_PIN, OUTPUT);
    loadSettings();
    setServoPosition(servoPosition); //move to saved position
    setBrightness(brightness); //set saved brightness
    sendStatus(); //send the initial status
}

void loop() {
    if (Serial.available() > 0) {
        String commandLine = Serial.readStringUntil('\n');
        commandLine.trim();
        processCommand(commandLine);
    }
     // Smooth Servo Movement with Timeout
    if (coverMoving) {
        static unsigned long moveStartTime = 0;
        int currentServoPos = myServo.read();

        if (moveStartTime == 0) {
            moveStartTime = millis();
        }

        if (currentServoPos != servoPosition) {
            if (millis() - moveStartTime < 2000) { // 2-second timeout
                if (currentServoPos < servoPosition) {
                    myServo.write(currentServoPos + 1);
                } else {
                    myServo.write(currentServoPos - 1);
                }
                delay(5);  // Small delay
            } else {
                // Timeout
                coverMoving = false;
                moveStartTime = 0;
            }
        } else {
            coverMoving = false;
            moveStartTime = 0;
        }
    }
}

void processCommand(String commandLine) {
    if (commandLine.isEmpty()) { // Use .isEmpty() for String objects
        return; // Ignore empty commands
    }

    int colonIndex = commandLine.indexOf(':');
    if (colonIndex == -1 && commandLine != "Q") { //added Q as it does not contains :
        Serial.println("ERROR"); // Invalid format
        return;
    }

    String command;
    String valueStr;

    if(commandLine == "Q")
    {
      command = "Q"; //no values
    }
    else{
        command = commandLine.substring(0, colonIndex);
        valueStr = commandLine.substring(colonIndex + 1);
    }
    if (command == "S") {
        int newPosition = valueStr.toInt();
        setServoPosition(newPosition);
        Serial.println("OK");
        saveSettings(); //save settings

    } else if (command == "B") {
        int newBrightness = valueStr.toInt();
        setBrightness(newBrightness);
        Serial.println("OK");
         saveSettings();//save settings

    } else if (command == "Q") {
        sendStatus();

    }
     else {
        Serial.println("ERROR"); // Unknown command
    }
}

void sendStatus() {
    Serial.print("STATUS:");
    Serial.print(myServo.read()); //send current position
    Serial.print(",");
    Serial.print(brightness);
    Serial.println();
    Serial.flush(); // Wait for sending to complete
}

void setBrightness(int newBrightness) {
    brightness = constrain(newBrightness, 0, 255);
    analogWrite(LED_PIN, brightness);
}
void setServoPosition(int newPosition) {
    servoPosition = constrain(newPosition, 0, 180);
    //myServo.write(servoPosition); //move in the loop
    coverMoving = true; //set flag for smooth movement

}
void saveSettings()
{
  EEPROM.update(EEPROM_BRIGHTNESS_ADDR,brightness);
  EEPROM.update(EEPROM_SERVO_ADDR,servoPosition);
}
void loadSettings()
{
    brightness = EEPROM.read(EEPROM_BRIGHTNESS_ADDR);
    servoPosition = EEPROM.read(EEPROM_SERVO_ADDR);
    //sanity checks in case of corrupted EEPROM
    if(servoPosition<0 || servoPosition>180)
    {
        servoPosition = 180;
    }
    if(brightness<0 || brightness > 255)
    {
        brightness = 0;
    }
}
