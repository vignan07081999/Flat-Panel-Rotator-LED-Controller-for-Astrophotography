#include "RotationPanel.h" // VERY IMPORTANT: Include the header file!
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

}

// --- ISNewNumber ---
void RotationPanel::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {

    if (strcmp(name, "SERVO_POSITION") == 0) {
        servoPositionNP->update(values, names, n);
         //Indi expects the driver to set the value after.
        IDSetNumber(servoPositionNP, nullptr);

        // Check if the state is OK before sending the command
        if (servoPositionNP->s == IPS_OK) {
            char command[32];
            snprintf(command, sizeof(command), "S%.0f", (*servoPositionNP)[0].value);
            if (!sendCommand(command)) {
              servoPositionNP->s = IPS_ALERT; // Set state to Alert if command fails
              logError("Failed to send servo command.");
            } else {
                //Wait for response from Arduino and update the values.
                char buffer[256];
                if (receiveResponse(buffer, sizeof(buffer))){
                    if (strstr(buffer, "OK:Servo:") != nullptr) {
                        int servoVal = atoi(buffer + 8); // Extract value after "OK:Servo:"
                        if (servoVal == (int)(*servoPositionNP)[0].value){
                             servoPositionNP->s = IPS_OK;
                        }
                        else {
                            servoPositionNP->s = IPS_ALERT; // Set state to Alert if command fails
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
        //Indi expects the driver to set the value after.
        IDSetNumber(ledBrightnessNP, nullptr);

      if (ledBrightnessNP->s == IPS_OK) {
        char command[32];
        snprintf(command, sizeof(command), "L%.0f", (*ledBrightnessNP)[0].value);
        if (!sendCommand(command)) {
             ledBrightnessNP->s = IPS_ALERT; // Set state to Alert
             logError("Failed to send LED command.");
        } else {
            char buffer[256];
                if (receiveResponse(buffer, sizeof(buffer))){
                    if (strstr(buffer, "OK:LED:") != nullptr) {
                        int ledVal = atoi(buffer + 6); // Extract value after "OK:Servo:"
                        if (ledVal == (int)(*ledBrightnessNP)[0].value){
                             ledBrightnessNP->s = IPS_OK;
                        }
                        else {
                            ledBrightnessNP->s = IPS_ALERT; // Set state to Alert if command fails
                            logError("LED brightness mismatch.");
                        }
                    }
                }
                else{
                    ledBrightnessNP->s = IPS_ALERT;
                    logError("No response from LED command.");
                }
        }
      }

    }
    else {
        INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
    }
    updatePropertyStates();
}

// --- sendCommand ---
bool RotationPanel::sendCommand(const char *command) {
   if (!isConnected) {
      IDLog("Error: Not connected. Cannot send command.\n");
      logError("Not connected. Cannot send command.");
      return false;
    }
    IDLog("Sending command: %s\n", command);

    if (connection->send(command, strlen(command)) < 0) {
        IDLog("Error sending command: %s\n", command);
        logError("Failed to send command.");
        return false;
    }
     if (connection->send("\n", 1) < 0) { // Send newline
        IDLog("Error sending newline after command.\n");
        logError("Error sending newline after command.");
        return false;
    }
    return true;
}


// --- receiveResponse ---
bool RotationPanel::receiveResponse(char *buffer, size_t bufferSize, int timeout_ms) {
    if (!isConnected) {
        return false; // Not connected
    }
    buffer[0] = '\0'; // Initialize buffer
    int len = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (true) {
        int bytes_read = connection->receive(buffer + len, 1); //Read one byte a time.
        if (bytes_read > 0) {
            len += bytes_read;
            if (buffer[len - 1] == '\n') {
                 buffer[len-1] = '\0'; //terminate string and remove newline
                IDLog("Received: %s\n", buffer);
                return true; // Got a complete line
            }
             if (len >= bufferSize - 1) {
                buffer[bufferSize - 1] = '\0'; // Ensure null-termination
                IDLog("Warning: receive buffer full.\n");
                 logError("Receive buffer full.");
                return false;
            }
        }
        else if (bytes_read == 0){
            //No data
        }
        else{ // bytes_read < 0
             if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available yet, check timeout
             }
             else{
                IDLog("Error on Serial Read %s\n", strerror(errno));
                logError("Error on Serial Read: " + std::string(strerror(errno)));
                 return false;
             }
        }

        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
        if (elapsed_time.count() > timeout_ms) {
            IDLog("Timeout waiting for response.\n");
            logError("Timeout waiting for response.");
            return false; // Timeout
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Don't busy-wait

    }
}

// --- updatePropertyStates ---
void RotationPanel::updatePropertyStates() {
    if (isConnected) {
        servoPositionNP->s = IPS_OK;
        ledBrightnessNP->s = IPS_OK;
    } else {
        servoPositionNP->s = IPS_IDLE;
        ledBrightnessNP->s = IPS_IDLE;
    }

}

void RotationPanel::logError(const std::string& message) {
    std::ofstream logFile("rotation_panel_error_log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
        logFile << ss.str() << " - Error: " << message << "\n";
        logFile.close();
    }
}
