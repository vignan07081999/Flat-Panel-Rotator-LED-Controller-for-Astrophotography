#include "RotationPanel.h" // VERY IMPORTANT: Include the header file!
#include <libindi/basedevice.h> // Include basedevice.h here too!
#include <libindi/indicom.h>    // Include indicom.h here!
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <cstring> // For string functions (already in .h, but good practice to include in .cpp too)


// --- Constructor ---
RotationPanel::RotationPanel(const char *port) : isConnected(false) {
    strncpy(portName, port, sizeof(portName) - 1);
    portName[sizeof(portName) - 1] = '\0'; // Ensure null termination
    connection = new INDI::Connection();
}

// --- Destructor ---
RotationPanel::~RotationPanel() {
    if (isConnected) {
        Disconnect(); // Ensure proper disconnection
    }
    delete connection;
}


// --- Connect ---
bool RotationPanel::Connect() {
    if (isConnected) {
        IDLog("Already connected.\n");
        return true;
    }

    if (connection->connect(portName, BAUD_RATE, 0) < 0) {
        IDLog("Error connecting to port %s\n", portName);
        logError("Could not connect to serial port.");
        return false;
    }

    isConnected = true;
    IDLog("Connected to %s\n", portName);

    // Wait for Arduino to send "Rotation Panel Ready"
    char buffer[256];
    if (!receiveResponse(buffer, sizeof(buffer), 2000)) { // Wait up to 2 seconds
        IDLog("Error: Did not receive handshake from Arduino.\n");
        logError("Did not receive handshake from Arduino.");
        Disconnect(); // Disconnect if handshake fails
        return false;
    }

    if (strstr(buffer, "Rotation Panel Ready") == nullptr) {
        IDLog("Error: Invalid handshake from Arduino.  Received: %s\n", buffer);
        logError("Invalid handshake from Arduino.");
        Disconnect();
        return false;
    }
    //Succesfully connected, lets read current values
    getValues();
    updatePropertyStates();
    return true;
}

// --- Disconnect ---
bool RotationPanel::Disconnect() {
    if (isConnected) {
        connection->disconnect();
        IDLog("Disconnected from %s\n", portName);
        isConnected = false;
    }
    updatePropertyStates();
    return true;
}

// --- get values
bool RotationPanel::getValues(){
    char buffer[256];
    if (sendCommand("GETS")){
        if (receiveResponse(buffer, sizeof(buffer))){
           if (strstr(buffer, "OK:Servo:") != nullptr) {
                int servoVal = atoi(buffer + 8); // Extract value after "OK:Servo:"
                servoPositionNP->value = servoVal; //Access using -> operator
           }
        }
    }
    if (sendCommand("GETL")){
        if (receiveResponse(buffer, sizeof(buffer))){
           if (strstr(buffer, "OK:LED:") != nullptr) {
                int ledVal = atoi(buffer + 6);  // Extract value after "OK:LED:"
                ledBrightnessNP->value = ledVal; //Acces using -> operator
           }
        }
    }
    return true;
}

// --- initProperties ---
bool RotationPanel::initProperties() {
    INDI::DefaultDevice::initProperties(); // Call superclass method

    // --- Servo Position ---
    servoPositionNP = new INDI::NumberVectorProperty("SERVO_POSITION", "Servo Position", "Main Control", IP_RW, IPS_IDLE);
    (*servoPositionNP)[0].name = "Position";
    (*servoPositionNP)[0].label = "Degrees";
    (*servoPositionNP)[0].format = "%.0f";
    (*servoPositionNP)[0].min = 0;
    (*servoPositionNP)[0].max = 180;
    (*servoPositionNP)[0].step = 1;
    (*servoPositionNP)[0].value = 90; // Initial value (match Arduino)
    addNumberVector(*servoPositionNP);


    // --- LED Brightness ---
    ledBrightnessNP = new INDI::NumberVectorProperty("LED_BRIGHTNESS", "LED Brightness", "Main Control", IP_RW, IPS_IDLE);
    (*ledBrightnessNP)[0].name = "Brightness";
    (*ledBrightnessNP)[0].label = "Level";
    (*ledBrightnessNP)[0].format = "%.0f";
    (*ledBrightnessNP)[0].min = 0;
    (*ledBrightnessNP)[0].max = 255;
    (*ledBrightnessNP)[0].step = 1;
    (*ledBrightnessNP)[0].value = 128;   // Initial value
    addNumberVector(*ledBrightnessNP);

    // --- Connection ---
    addStandardConnectionOption(CONNECTION_SERIAL); // Add standard serial connection option

    return true;
}

// --- ISGetProperties ---
void RotationPanel::ISGetProperties(const char *dev) {
    INDI::DefaultDevice::ISGetProperties(dev);
     if (isConnected) {
        // When properties are requested, update with current values
        getValues();
    }
    defineNumber(servoPositionNP);
    defineNumber(ledBrightnessNP);
