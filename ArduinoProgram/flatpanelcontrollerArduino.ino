#include <Servo.h>
#include <EEPROM.h>

// --- Pin Definitions ---
const int SERVO_PIN = 9;
const int LED_PIN = 10; // PWM pin

// --- EEPROM Addresses ---
const int EEPROM_BRIGHTNESS_ADDR = 0;
const int EEPROM_SERVO_ADDR = 1;

// --- States ---
const int STATE_CLOSED = 0;
const int STATE_OPEN = 1;
const int STATE_MOVING = 2;

// --- Variables ---
Servo myServo;
int brightness = 0;       // 0-255
int servoPosition = 180; // Closed initially
bool coverMoving = false;

// --- Commands ---
const char CMD_SET_BRIGHTNESS = 'B'; // Bxxx (xxx = 000-255)
const char CMD_OPEN = 'O';
const char CMD_CLOSE = 'C';
const char CMD_HALT = 'H';
const char CMD_GET_STATUS = 'S';



void setup() {
    Serial.begin(115200); // Use a consistent baud rate
    myServo.attach(SERVO_PIN);
    pinMode(LED_PIN, OUTPUT);
    loadSettings();
    setServoPosition(servoPosition); //move to saved position
    setBrightness(brightness); //set saved brightness
    applyLightState(); //apply the light
    sendStatus(); //initial status

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
                delay(5); // Small delay
            } else {
                // Timeout
                coverMoving = false;
                moveStartTime = 0; // Reset timer
            }
        } else {
            coverMoving = false;
            moveStartTime = 0;
        }
    }
}

void processCommand(String commandLine) {
    if (commandLine.length() == 0) { // Corrected emptiness check
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
void openCover()
{
    setServoPosition(0); //open
    coverMoving = true;
}

void closeCover()
{
     setServoPosition(180); //close
    coverMoving = true;
}
void haltCover()
{
    coverMoving = false;
}
void applyLightState() {
  analogWrite(LED_PIN, brightness); //set the led
}

void sendStatus() {
    // Format: "brightness,servoPosition\n"  (NO leading zeros, NO carriage return)
    Serial.print(brightness);
    Serial.print(",");
    Serial.print(myServo.read()); //send current position
    Serial.println(); // Newline is important
    Serial.flush(); // Wait for sending to complete

}

void setBrightness(int newBrightness) {
    brightness = constrain(newBrightness, 0, 255);
    applyLightState(); //update led
}

void setServoPosition(int newPosition) {
    servoPosition = constrain(newPosition, 0, 180);
    //myServo.write(servoPosition); //move servo in the loop
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
