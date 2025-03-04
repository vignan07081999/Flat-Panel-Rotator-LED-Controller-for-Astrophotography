#include "flatfield_driver.h"
#include <iostream>
#include <string.h>
#include <sstream>
#include <memory>
#include <algorithm>

FlatFieldDriver::FlatFieldDriver() : isConnected(false), currentServoPos(90), currentLEDBrightness(0) {
    setVersion(1, 0); // Set driver version
     timer = 0;
    timer_flag = false;
    count = 0;
}

FlatFieldDriver::~FlatFieldDriver() {}

const char *FlatFieldDriver::getDefaultPort() {
   return "/dev/ttyACM0";  // Default serial port (Linux).  Adjust as needed.
}
//ISGetProperties is called only once when the driver first loads
bool FlatFieldDriver::ISGetProperties(const char *dev)
{
    // Call the parent class first.
    INDI::DefaultDriver::ISGetProperties(dev);
    // Call initProperties which will define the device properties.
    initProperties();
    return true;
}

bool FlatFieldDriver::initProperties() {
    // Initialize INDI properties
    INDI::DefaultDriver::initProperties();

    // --- Servo Property ---
    servoProperty = new INDI::NumberVectorProperty();
    servoProperty->name = "SERVO_POSITION";
    servoProperty->label = "Servo Position";
    servoProperty->group = "Main Control";
    servoProperty->state = IPS_IDLE;
    servoProperty->perm = IP_RW; // Read-Write permission

    INumber servoValue;
    servoValue.name = "position";
    servoValue.label = "Position (deg)";
    servoValue.format = "%.0f";
    servoValue.min = 0;
    servoValue.max = 180;
    servoValue.step = 1;
    servoValue.value = currentServoPos; // Initial value
    servoProperty->np = &servoValue;
    servoProperty->nnp = 1; // Number of number elements

    defineNumber(servoProperty);


    // --- LED Brightness Property ---
    ledProperty = new INDI::NumberVectorProperty();
    ledProperty->name = "LED_BRIGHTNESS";
    ledProperty->label = "LED Brightness";
    ledProperty->group = "Main Control";
    ledProperty->state = IPS_IDLE;
    ledProperty->perm = IP_RW;

    INumber ledValue;
    ledValue.name = "brightness";
    ledValue.label = "Brightness";
    ledValue.format = "%.0f";
    ledValue.min = 0;
    ledValue.max = 255;
    ledValue.step = 1;
    ledValue.value = currentLEDBrightness;
    ledProperty->np = &ledValue;
    ledProperty->nnp = 1;

    defineNumber(ledProperty);

     // --- Log Property ---
    logProperty = new INDI::TextVectorProperty();
    logProperty->name = "LOG_MESSAGE";
    logProperty->label = "Log";
    logProperty->group = "Logs";
    logProperty->state = IPS_IDLE;
    logProperty->perm = IP_RO;  // Read Only

    IText logText;
    logText.name = "log";
    logText.text = ""; // Initialize with empty string
    logProperty->tp = &logText;
    logProperty->ntp = 1;

    defineText(logProperty);

    //add a timer to the driver.
    addTimer(500); //update and send the feedback for every 500ms

    return true;
}

//timerEvent method is called every time the timer set by addTimer method in initProperties.
void FlatFieldDriver::timerEvent()
{
    // Set the timer flag to allow processing in the main loop
    timer_flag = true;

    // Clear the timer by calling clearTimer.  If you do not call clearTimer,
    // the timer task will be called only once.
    clearTimer(timer);

    // After clearing the timer, you must add it again if you want it to repeat.
    addTimer(500);
     if (isConnected) {
        // Request feedback every time the timer fires
        sendCommand("F");
    }
}

bool FlatFieldDriver::ConnectDevice(const char *port) {

     if (isConnected) {
        log("Device already connected.");
        return true; // Already connected
    }
    LOGF_INFO("Connecting to device on port: %s", port);
    serialPort = new INDI::BaseDevice::SerialPort(port, 0); // 0 for default flags

  if (serialPort->Connect() != 0) {
        LOG_ERROR("Failed to connect to serial port.");
        delete serialPort;
        serialPort = nullptr;
        isConnected = false;
        return false;
    }

    serialPort->setBaudRate(INDI::BaseDevice::SerialPort::B_115200);
    serialPort->setParity(INDI::BaseDevice::SerialPort::P_NONE);
    serialPort->setDataBits(INDI::BaseDevice::SerialPort::D_8);
    serialPort->setStopBits(INDI::BaseDevice::SerialPort::S_1);
    if (!serialPort->applySettings())
     {
        LOG_ERROR("Failed to set Serial settings");
        delete serialPort;
        serialPort = nullptr;
        isConnected = false;
        return false;
     }
    isConnected = true;
    log("Device connected successfully.");
    //initial values and feedback command
    char command[20];
    sprintf(command, "S%d", currentServoPos);
    sendCommand(command);
    sprintf(command, "L%d", currentLEDBrightness);
    sendCommand(command);
    sendCommand("F");
    return true;
}

bool FlatFieldDriver::DisconnectDevice(const char *port) {

    if (!isConnected || serialPort == nullptr) {
        return true; // Already disconnected
    }

    LOGF_INFO("Disconnecting from device on port: %s", port);

    if (serialPort->Disconnect() != 0) {
        LOG_ERROR("Failed to disconnect from serial port.");
        return false;
    }

    delete serialPort;
    serialPort = nullptr;
    isConnected = false;
    log("Device disconnected.");
     IDSetSwitch (getSwitch("CONNECTION"), "DISCONNECT", nullptr, getDeviceName());
    return true;
}

bool FlatFieldDriver::sendCommand(const char *command) {
    if (!isConnected || serialPort == nullptr) {
        log("Not connected. Cannot send command.");
        return false;
    }

    LOGF_DEBUG("Sending command: %s", command);
    int len = strlen(command);
    int n = serialPort->write(command, len); //send with out newline
    serialPort->write("\n", 1);//send newline
    if (n != len+1) {
        LOG_ERROR("Failed to send command.");
        return false;
    }
     // Read the response from the Arduino
    char buffer[256] = {0};
    int bytesRead = serialPort->read(buffer, sizeof(buffer) - 1, 1000); // 1-second timeout
     if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate
        processFeedback(buffer); // Process the received data

    }
    else if(bytesRead<0)
    {
         LOG_ERROR("Failed to get response from arduino.");
    }
    return true;
}


void FlatFieldDriver::processFeedback(const char *feedback) {
     LOGF_DEBUG("Raw feedback: %s", feedback); //added for raw feedback from arduino

    // Parse feedback: "SP:90,LB:128"
    if (strncmp(feedback, "SP:", 3) == 0) {
        char *commaPos = strchr(feedback, ',');
        if (commaPos != nullptr) {
            *commaPos = '\0';  // Split the string at the comma
            char *servoStr = feedback + 3; // Skip "SP:"
            char *ledStr = commaPos + 4;   // Skip ",LB:"

            int servoPos = atoi(servoStr);
            int ledBrightness = atoi(ledStr);

            // Update internal variables
            currentServoPos = servoPos;
            currentLEDBrightness = ledBrightness;

            // Update INDI properties
            servoProperty->np[0].value = servoPos;
            ledProperty->np[0].value = ledBrightness;

            servoProperty->state = IPS_OK;
            ledProperty->state = IPS_OK;

            IDSetNumber(servoProperty, nullptr); // Send update to INDI server
            IDSetNumber(ledProperty, nullptr);

        }
    }
      else if (strstr(feedback, "Invalid Command") != NULL ||
               strstr(feedback, "Invalid Brightness Value") != NULL ||
               strstr(feedback, "Invalid Servo Position") != NULL)
               {
                log(feedback);
               }
}

void FlatFieldDriver::log(const char *message) {
    LOG_INFO(message); //log using INDI logger
    // Update the log property
    if (logProperty && logProperty->tp) { // Check for null pointers
        strncpy(logProperty->tp[0].text, message, MAXINDISTRLEN - 1);
        logProperty->tp[0].text[MAXINDISTRLEN - 1] = '\0'; // Ensure null termination
        logProperty->state = IPS_OK;
        IDSetText(logProperty, nullptr);
    }
}


void FlatFieldDriver::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {

     // Check which property changed
    if (strcmp(name, "SERVO_POSITION") == 0) {
        // Update servo position
        int newServoPos = (int)values[0];
        if (newServoPos != currentServoPos) {
          char command[20];
          sprintf(command, "S%d", newServoPos);
          if (sendCommand(command))
          {
            servoProperty->np[0].value = newServoPos; //update the indi property
            servoProperty->state = IPS_OK;
          }
          else{
            servoProperty->state = IPS_ALERT;
          }
        }
    } else if (strcmp(name, "LED_BRIGHTNESS") == 0) {
        // Update LED brightness
        int newLEDBrightness = (int)values[0];
        if (newLEDBrightness != currentLEDBrightness)
        {
          char command[20];
          sprintf(command, "L%d", newLEDBrightness);
          if (sendCommand(command))
          {
            ledProperty->np[0].value = newLEDBrightness;
            ledProperty->state = IPS_OK;
          }
          else{
             ledProperty->state = IPS_ALERT;
          }
        }
    }
    IDSetNumber(getNumber(name),nullptr);
}

// --- Main Function (for INDI Server) ---
int main(int argc, char **argv) {
    FlatFieldDriver *driver = new FlatFieldDriver();
    driver->setIndiVersion(B_1_7); // Set INDI version
    driver->start(); // This starts the INDI server loop
    delete driver;
    return 0;
}
