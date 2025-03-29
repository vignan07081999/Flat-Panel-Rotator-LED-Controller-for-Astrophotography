/*
 * Arduino_Firmware_Enhanced_Limited_Pins_StdPWM.ino
 * Based on code Copyright (C) 2022 - Present, Julien Lecomte
 * Modifications for LED control, arbitrary position, Python GUI feedback,
 * limited close angle (min 20 degrees), specified pins (Servo:6, LED:9),
 * using STANDARD LED PWM frequency due to Timer1/Servo conflict.
 * Licensed under the MIT License.
 */

#include <Servo.h>

// --- Configuration ---
#define SERVO_PIN 6      // Servo pin
#define LED_PIN 9        // <-- CHANGED LED pin back to 9
#define MIN_ANGLE 20     // Minimum allowed angle (Closed Position)
#define MAX_ANGLE 180    // Maximum allowed angle (Open Position)
#define BAUD_RATE 57600
#define MOVEMENT_DELAY 20 // Milliseconds delay between servo steps

// --- Communication Protocol (Commands, Results, Errors remain the same) ---
constexpr auto DEVICE_GUID = "b45ba2c9-f554-4b4e-a43c-10605ca3b84d";

constexpr auto COMMAND_PING = "COMMAND:PING";
constexpr auto COMMAND_INFO = "COMMAND:INFO";
constexpr auto COMMAND_GETSTATE = "COMMAND:GETSTATE";
constexpr auto COMMAND_OPEN = "COMMAND:OPEN";
constexpr auto COMMAND_CLOSE = "COMMAND:CLOSE";
constexpr auto COMMAND_SETPOS_PREFIX = "COMMAND:SETPOS:";
constexpr auto COMMAND_SETLED_PREFIX = "COMMAND:SETLED:";
constexpr auto COMMAND_GETSTATUS = "COMMAND:GETSTATUS";

constexpr auto RESULT_PING = "RESULT:PING:OK:";
constexpr auto RESULT_INFO = "RESULT:DarkSkyGeek's Telescope Cover Firmware v1.4-Pin9-StdPWM"; // Updated version
constexpr auto RESULT_STATE_UNKNOWN = "RESULT:STATE:UNKNOWN";
constexpr auto RESULT_STATE_OPEN = "RESULT:STATE:OPEN";
constexpr auto RESULT_STATE_CLOSED = "RESULT:STATE:CLOSED";
constexpr auto RESULT_STATE_MOVING = "RESULT:STATE:MOVING";
constexpr auto RESULT_STATUS_PREFIX = "RESULT:STATUS:";
constexpr auto RESULT_OK = "RESULT:OK";

constexpr auto ERROR_INVALID_COMMAND = "ERROR:INVALID_COMMAND";
constexpr auto ERROR_INVALID_ARGUMENT = "ERROR:INVALID_ARGUMENT";
constexpr auto ERROR_OUT_OF_RANGE = "ERROR:OUT_OF_RANGE";

// --- Global Variables ---
Servo servo;
int currentAngle = MIN_ANGLE;
int targetAngle = MIN_ANGLE;
int currentLedBrightness = 0; // Stores the desired brightness (0-255) set by user
bool isMoving = false;

// --- Setup ---
void setup() {
    // Initialize serial port
    Serial.begin(BAUD_RATE);
    while (!Serial) { ; }
    Serial.flush();

    // --- Standard PWM Frequency on Pin 9 ---
    // Pin 9 uses Timer1 for PWM. Changing Timer1 frequency conflicts
    // with the standard Servo library. Therefore, we do NOT modify
    // timer registers here and use the default PWM frequency (~490Hz).

    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    setLed(0); // Start with LED off

    // Initialize servo on the correct pin
    servo.attach(SERVO_PIN);
    servo.write(currentAngle); // Move to initial MIN_ANGLE position explicitly
    delay(500);

    sendStatus(); // Send initial status
}

// --- Main Loop ---
void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == COMMAND_PING) handlePing();
        else if (command == COMMAND_INFO) sendFirmwareInfo();
        else if (command == COMMAND_GETSTATE) sendAscomState();
        else if (command == COMMAND_GETSTATUS) sendStatus();
        else if (command == COMMAND_OPEN) moveToPosition(MAX_ANGLE);
        else if (command == COMMAND_CLOSE) moveToPosition(MIN_ANGLE);
        else if (command.startsWith(COMMAND_SETPOS_PREFIX)) handleSetPosition(command);
        else if (command.startsWith(COMMAND_SETLED_PREFIX)) handleSetLed(command);
        else if (command.length() > 0) handleInvalidCommand(command);
    }
}

// --- Command Handlers (handlePing, sendFirmwareInfo, sendAscomState, sendStatus are same) ---
void handlePing() {
    Serial.print(RESULT_PING);
    Serial.println(DEVICE_GUID);
}

void sendFirmwareInfo() {
    Serial.println(RESULT_INFO);
}

void sendAscomState() {
    int tolerance = 5;
    if (isMoving) Serial.println(RESULT_STATE_MOVING);
    else if (abs(currentAngle - MIN_ANGLE) < tolerance) Serial.println(RESULT_STATE_CLOSED);
    else if (abs(currentAngle - MAX_ANGLE) < tolerance) Serial.println(RESULT_STATE_OPEN);
    else Serial.println(RESULT_STATE_OPEN);
}

void sendStatus() {
    Serial.print(RESULT_STATUS_PREFIX);
    Serial.print(currentAngle);
    Serial.print(":");
    Serial.println(currentLedBrightness); // Report the stored brightness
}

void handleSetPosition(String command) {
    String arg = command.substring(strlen(COMMAND_SETPOS_PREFIX));
    bool argOk = false;
    for (int i = 0; i < arg.length(); i++) { if (isDigit(arg.charAt(i))) { argOk = true; break; }}

    if (argOk) {
        int requested_angle = arg.toInt();
        moveToPosition(requested_angle);
    } else {
        Serial.println(ERROR_INVALID_ARGUMENT);
    }
}

void handleSetLed(String command) {
    String arg = command.substring(strlen(COMMAND_SETLED_PREFIX));
    bool argOk = false;
    for (int i = 0; i < arg.length(); i++) { if (isDigit(arg.charAt(i))) { argOk = true; break; }}

    if (argOk) {
        int brightness = arg.toInt();
        brightness = constrain(brightness, 0, 255);

        currentLedBrightness = brightness;
        // Serial.print("Stored LED Brightness: "); Serial.println(currentLedBrightness); // Debug

        // Apply brightness physically only if closed and not moving
        if (!isMoving && abs(currentAngle - MIN_ANGLE) < 5) {
             // Serial.print("Applying LED Brightness: "); Serial.println(currentLedBrightness); // Debug
             setLed(currentLedBrightness);
        } else {
             // Serial.println("Cover not closed or is moving, ensuring LED is off."); // Debug
             setLed(0);
        }
        Serial.println(RESULT_OK);
        sendStatus();
    } else {
        Serial.println(ERROR_INVALID_ARGUMENT);
    }
}

void handleInvalidCommand(String command) {
    Serial.print(ERROR_INVALID_COMMAND);
    Serial.print(":");
    Serial.println(command);
}


// --- Core Functions ---

// Sets the physical LED brightness using analogWrite on the specified LED_PIN
void setLed(int brightness) {
    analogWrite(LED_PIN, brightness);
}

// Moves the servo to the target position step-by-step
void moveToPosition(int target) {
    int constrained_target = constrain(target, MIN_ANGLE, MAX_ANGLE);

    if (target != constrained_target && (target < MIN_ANGLE || target > MAX_ANGLE)) {
        Serial.print(ERROR_OUT_OF_RANGE);
        Serial.print(": Requested="); Serial.print(target);
        Serial.print(", Actual="); Serial.println(constrained_target);
    }

    targetAngle = constrained_target;

    if (targetAngle == currentAngle) {
        if (abs(currentAngle - MIN_ANGLE) < 5) { setLed(currentLedBrightness); }
        else { setLed(0); }
        sendStatus();
        return;
    }

    isMoving = true;
    setLed(0); // Turn LED OFF during movement
    Serial.println(RESULT_STATE_MOVING);

    // Gradual movement loop
    if (targetAngle > currentAngle) {
        for (int pos = currentAngle + 1; pos <= targetAngle; pos++) {
            servo.write(pos);
            currentAngle = pos;
            delay(MOVEMENT_DELAY);
        }
    } else {
        for (int pos = currentAngle - 1; pos >= targetAngle; pos--) {
            servo.write(pos);
            currentAngle = pos;
            delay(MOVEMENT_DELAY);
        }
    }

    isMoving = false;

    // Set final LED state based on position
    if (abs(currentAngle - MIN_ANGLE) < 5) {
        // Serial.print("Movement finished at closed pos. Applying brightness: "); Serial.println(currentLedBrightness); // Debug
        setLed(currentLedBrightness);
    } else {
        // Serial.println("Movement finished at open pos. Ensuring LED is off."); // Debug
        setLed(0);
    }

    sendStatus();
}
