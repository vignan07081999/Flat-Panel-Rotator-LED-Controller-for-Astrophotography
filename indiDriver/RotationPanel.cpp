#include "RotationPanel.h" // ONLY include the header!
// DO NOT include <libindi/...> headers here again!

#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
// #include <cstring> // No longer needed here, RotationPanel.h includes it

// --- Constructor ---
RotationPanel::RotationPanel(const char *port) : isConnected(false) {
    strncpy(portName, port, sizeof(portName) - 1);
    portName[sizeof(portName) - 1] = '\0';
    connection = new INDI::Connection();
}

// --- Destructor ---
RotationPanel::~RotationPanel() {
    if (isConnected) {
        Disconnect();
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

    char buffer[256];
    if (!receiveResponse(buffer, sizeof(buffer), 2000)) {
        IDLog("Error: Did not receive handshake from Arduino.\n");
        logError("Did not receive handshake from Arduino.");
        Disconnect();
        return false;
    }

    if (strstr(buffer, "Rotation Panel Ready") == nullptr) {
        IDLog("Error: Invalid handshake from Arduino.  Received: %s\n", buffer);
        logError("Invalid handshake from Arduino.");
        Disconnect();
        return false;
    }

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
                int servoVal = atoi(buffer + 8);
                servoPositionNP->value = servoVal;
           }
        }
    }
    if (sendCommand("GETL")){
        if (receiveResponse(buffer, sizeof(buffer))){
           if (strstr(buffer, "OK:LED:") != nullptr) {
                int ledVal = atoi(buffer + 6);
                ledBrightnessNP->value = ledVal;
           }
        }
    }
    return true;
}

// --- initProperties ---
bool RotationPanel::initProperties() {
    INDI::DefaultDevice::initProperties();

    servoPositionNP = new INDI::NumberVectorProperty("SERVO_POSITION", "Servo Position", "Main Control", IP_RW, IPS_IDLE);
    (*servoPositionNP)[0].name = "Position";
    (*servoPositionNP)[0].label = "Degrees";
    (*servoPositionNP)[0].format = "%.0f";
    (*servoPositionNP)[0].min = 0;
    (*servoPositionNP)[0].max = 180;
    (*servoPositionNP)[0].step = 1;
    (*servoPositionNP)[0].value = 90;
    addNumberVector(*servoPositionNP);

    ledBrightnessNP = new INDI::NumberVectorProperty("LED_BRIGHTNESS", "LED Brightness", "Main Control", IP_RW, IPS_IDLE);
    (*ledBrightnessNP)[0].name = "Brightness";
    (*ledBrightnessNP)[0].label = "Level";
    (*ledBrightnessNP)[0].format = "%.0f";
    (*ledBrightnessNP)[0].min = 0;
    (*ledBrightnessNP)[0].max = 255;
    (*ledBrightnessNP)[0].step = 1;
    (*ledBrightnessNP)[0].value = 128;
    addNumberVector(*ledBrightnessNP);

    addStandardConnectionOption(CONNECTION_SERIAL);

    return true;
}

// --- ISGetProperties ---
void RotationPanel::ISGetProperties(const char *dev) {
    INDI::DefaultDevice::ISGetProperties(dev);
     if (isConnected) {
        getValues();
    }
    defineNumber(servoPositionNP);
    defineNumber(ledBrightnessNP);

}

// --- ISNewNumber ---
void RotationPanel::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {

    if (strcmp(name, "SERVO_POSITION") == 0) {
        servoPositionNP->update(values, names, n);
        IDSetNumber(servoPositionNP, nullptr);

        if (servoPositionNP->s == IPS_OK) {
            char command[32];
            snprintf(command, sizeof(command), "S%.0f", (*servoPositionNP)[0].value);
            if (!sendCommand(command)) {
              servoPositionNP->s = IPS_ALERT;
              logError("Failed to send servo command.");
            } else {
                char buffer[256];
                if (receiveResponse(buffer, sizeof(buffer))){
                    if (strstr(buffer, "OK:Servo:") != nullptr) {
                        int servoVal = atoi(buffer + 8);
                        if (servoVal == (int)(*servoPositionNP)[0].value){
                             servoPositionNP->s = IPS_OK;
                        }
                        else {
                            servoPositionNP->s = IPS_ALERT;
                            logError("Servo position mismatch.");
                        }
                    }
                }
                else{
                    servoPositionNP->s = IPS_ALERT;
                    logError("No response from servo command.");
                }
            }
        }
    }
    else if (strcmp(name, "LED_BRIGHTNESS") == 0) {
        ledBrightnessNP->update(values, names, n);
        IDSetNumber(ledBrightnessNP, nullptr);

      if (ledBrightnessNP->s == IPS_OK) {
        char command[32];
        snprintf(command, sizeof(command), "L%.0f", (*ledBrightnessNP)[0].value);
        if (!sendCommand(command)) {
             ledBrightnessNP->s = IPS_ALERT;
             logError("Failed to send LED command.");
        } else {
            char buffer[256];
                if (receiveResponse(buffer, sizeof(buffer))){
                    if (strstr(buffer, "OK:LED:") != nullptr) {
                        int ledVal = atoi(buffer + 6);
                        if (ledVal == (int)(*ledBrightnessNP)[0].value){
                             ledBrightnessNP->s = IPS_OK;
                        }
                        else {
                            ledBrightnessNP->s = IPS_ALERT;
                            logError("LED brightness mismatch.");
                        }
                    }
                }
                else
