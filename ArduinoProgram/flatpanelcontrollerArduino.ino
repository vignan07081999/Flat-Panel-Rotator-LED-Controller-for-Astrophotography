#include <Servo.h>
#include <EEPROM.h>

// --- Pin Definitions ---
const int SERVO_PIN = 9;
const int LED_PIN = 10;

// --- Alnitak Commands ---
const char CMD_GET_STATUS = '?';       // Get status
const char CMD_SET_BRIGHTNESS = 'B'; // Set brightness
const char CMD_OPEN_COVER = 'O';      // Open cover
const char CMD_CLOSE_COVER = 'C';     // Close cover
const char CMD_HALT_COVER = 'H';      // Halt cover (not really supported, but we'll handle it)
const char CMD_INIT = 'I';          // initialise device

// --- Variables ---
Servo myServo;
int brightness = 0;       // Current LED brightness (0-255)
int servoPosition = 180;    // Current servo position (0-180, 180=closed, 0=open)
bool coverMoving = false;  // Track if the cover is currently moving

//eeprom address
const int EEPROM_BRIGHTNESS_ADDR = 0;
const int EEPROM_SERVO_ADDR = 1;

void setup() {
  Serial.begin(115200); // Match the Alnitak baud rate
  myServo.attach(SERVO_PIN);
  pinMode(LED_PIN, OUTPUT);
  loadSettings();
  setServoPosition(servoPosition);
  setBrightness(brightness);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    processCommand(command);
  }

    // Smooth Servo Movement (if needed) - keep this outside the serial check
    if (coverMoving) {
      int currentServoPos = myServo.read(); // Read *actual* current position
      if (currentServoPos != servoPosition) {
          if (currentServoPos < servoPosition) {
              myServo.write(currentServoPos + 1);
          } else {
              myServo.write(currentServoPos - 1);
          }
          delay(15); // Adjust for speed - IMPORTANT: Keep this short!
      }
      else
      {
        coverMoving = false; //stop moving
      }
    }
}
void processCommand(char command) {
  switch (command) {
    case CMD_GET_STATUS:
      sendStatus();
      break;
    case CMD_SET_BRIGHTNESS:
      // Expect a brightness value (0-255) as subsequent characters.
      // Read up to 4 characters, which is enough for "255\r".
      // Alnitak uses '\r' (carriage return) as the terminator.
      {
          String brightnessStr = Serial.readStringUntil('\r'); //read string until carriage return
          int newBrightness = brightnessStr.toInt();
          if(newBrightness >=0 && newBrightness <=255)
          {
              setBrightness(newBrightness);
              saveSettings();

          }
          else
          {
            Serial.println("x"); //invalid command
          }
      }

      break;
    case CMD_OPEN_COVER:
        setServoPosition(0);   // 0 = Open
        saveSettings();
        break;
    case CMD_CLOSE_COVER:
        setServoPosition(180); // 180 = Closed
        saveSettings();
        break;
    case CMD_HALT_COVER:
        // Stop any current servo movement. The simplest way is to just
        // set the target position to the current position.
         coverMoving = false;
        break;
      case CMD_INIT:
        //initialize
        loadSettings();
        setServoPosition(servoPosition);
        setBrightness(brightness);
        sendStatus(); // Send status after initialization
        break;
    default:
        Serial.println("x"); //invalid command
      break;
  }
}

void sendStatus() {
  // Format:  "bbb,ppp\r"  (brightness, position, carriage return)
  //brightness
  if(brightness<10)
  {
    Serial.print("00");
    Serial.print(brightness);
  }
  else if(brightness<100)
  {
    Serial.print("0");
    Serial.print(brightness);
  }
  else
  {
     Serial.print(brightness);
  }
  Serial.print(",");

  //servo position
  int currentServoPos = myServo.read();
  if(currentServoPos<10)
  {
    Serial.print("00");
    Serial.print(currentServoPos);
  }
  else if(currentServoPos<100)
  {
    Serial.print("0");
    Serial.print(currentServoPos);
  }
  else
  {
     Serial.print(currentServoPos);
  }

  Serial.print("\r"); // Carriage return is CRITICAL
}

void setBrightness(int newBrightness) {
  brightness = constrain(newBrightness, 0, 255);
  analogWrite(LED_PIN, brightness);
}

void setServoPosition(int newPosition) {
  servoPosition = constrain(newPosition, 0, 180);
  coverMoving = true; //start moving
  //myServo.write(servoPosition); //move to the position. This is handled in the loop for smooth movement
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
