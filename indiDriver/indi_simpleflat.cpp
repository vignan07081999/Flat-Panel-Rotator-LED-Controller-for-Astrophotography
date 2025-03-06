#include <libindi/indi.h>
#include <libindi/indidevapi.h>
#include <libindi/connectionserial.h> // Use INDI::Connection::Serial

#include <string>
#include <memory> // For std::unique_ptr
#include <iostream>
#include <sstream>
#include <cstring> // For strerror
#include <cerrno>  // For errno

class SimpleFlat : public INDI::DefaultDevice {
public:
    SimpleFlat();
    ~SimpleFlat();

    bool initProperties() override;
    void ISGetProperties(const char *dev) override;
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    bool UpdateProperties() override; //for updating properties
     void TimerHit() override;

    const char *getDefaultName() { return "SimpleFlat"; }

private:
    bool Connect();
    bool Disconnect();
    bool sendCommand(const std::string &command);
    bool readResponse(std::string &response, int timeout_ms = 1000); // Read with timeout
    bool getStatus(); //get status from arduino

    // Properties
    ISwitchVectorProperty *connection_prop;
    INumberVectorProperty *brightness_prop;
    ISwitchVectorProperty *cover_prop; // Use a switch for cover (open/close)

    // Serial connection
     std::unique_ptr<INDI::Serial> serialConnection; // Use Serial class
     bool connected = false;
     // Store values
    int currentBrightness = 0;
    int currentServoPos = 180;

};

// Constructor
SimpleFlat::SimpleFlat() {
     setVersion(1,0); //set driver version
}

// Destructor
SimpleFlat::~SimpleFlat() {}

// initProperties
bool SimpleFlat::initProperties() {
    INDI::DefaultDevice::initProperties();

    // --- Connection Property ---
    static ISwitch connection_switches[] = {
        {const_cast<char*>("CONNECT"), const_cast<char*>("Connect"), ISS_OFF},
        {const_cast<char*>("DISCONNECT"), const_cast<char*>("Disconnect"), ISS_ON}
    };
    connection_prop = newSwitch(&connection_switches, 2, getDeviceName(), "CONNECTION", IP_RW, ISRule::ONE_OF_MANY, 60, IPS_IDLE);
    defineSwitch(connection_prop);

    // --- Brightness Property ---
    static INumber brightness_numbers[] = {
        {const_cast<char*>("BRIGHTNESS"), const_cast<char*>("Brightness"), "%.0f", 0, 255, 1, 0},
    };
    brightness_prop = newNumber(&brightness_numbers, 1, getDeviceName(), "LIGHT", IP_RW, 60, IPS_IDLE);
    defineNumber(brightness_prop);


    // --- Cover Property (Switch) ---
    static ISwitch cover_switches[] = {
        {const_cast<char*>("OPEN"), const_cast<char*>("Open"), ISS_OFF},
        {const_cast<char*>("CLOSE"), const_cast<char*>("Close"), ISS_ON},
    };
    cover_prop = newSwitch(&cover_switches, 2, getDeviceName(), "COVER", IP_RW, ISRule::ONE_OF_MANY, 60, IPS_IDLE);
    defineSwitch(cover_prop);

    // --- Serial Connection ---
    serialConnection = std::make_unique<INDI::Serial>(this);
    serialConnection->setDevice(this); // Set device
    serialConnection->setDefaultBaudRate(INDI::Connection::Serial::B115200);  // 115200 baud
    serialConnection->setReadTimeout(5000); // 5 second timeout


     serialConnection->registerHandshake([&]() {
        // Attempt to connect and get initial status.
        if (serialConnection->connect()) {
            IDLog("Connected to serial port %s\n", serialConnection->getPortName());
            // Get initial status after short delay.
             SetTimer(1000);
            return true;
        } else {
             IDLog("Failed to connect to serial port %s\n", serialConnection->getPortName());
            return false;
        }
    });

    return true;
}

// ISGetProperties
void SimpleFlat::ISGetProperties(const char *dev) {
    INDI::DefaultDevice::ISGetProperties(dev);
     defineProperty(connection_prop);
     defineProperty(brightness_prop);
     defineProperty(cover_prop);

}

// ISNewSwitch (Connection and Cover)
bool SimpleFlat::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {

     if (strcmp(name, connection_prop->name) == 0) {
        IUFillSwitch(connection_prop, states, names, n);
        connection_prop->s = IPS_OK;

        if (connection_prop->sp[0].s == ISS_ON) {
            // Connect
            if (Connect()) {
                connection_prop->sp[0].s = ISS_ON;
                connection_prop->sp[1].s = ISS_OFF;
            } else {
                connection_prop->sp[0].s = ISS_OFF;
                connection_prop->sp[1].s = ISS_ON;
            }
        } else if (connection_prop->sp[1].s == ISS_ON) {
            // Disconnect
            if (Disconnect()) {
                connection_prop->sp[0].s = ISS_OFF;
                connection_prop->sp[1].s = ISS_ON;
            } else {
                connection_prop->sp[0].s = ISS_ON;
                connection_prop->sp[1].s = ISS_OFF;
            }
        }

        IDSetSwitch(connection_prop, nullptr);
        return true;
    }
    else if (strcmp(name, cover_prop->name) == 0) {
        IUFillSwitch(cover_prop, states, names, n);
        cover_prop->s = IPS_OK;

        if (cover_prop->sp[0].s == ISS_ON) {
            // Open
            if (!sendCommand("O\n")) { //send command
                IDLog("Failed to send open command.\n");
                cover_prop->s = IPS_ALERT;
            }
        } else if (cover_prop->sp[1].s == ISS_ON) {
            // Close
            if (!sendCommand("C\n")) {
                  IDLog("Failed to send close command.\n");
                 cover_prop->s = IPS_ALERT;
            }
        }

        IDSetSwitch(cover_prop, nullptr);
        return true;
    }
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

// ISNewNumber (Brightness)
bool SimpleFlat::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {

    if (strcmp(name, brightness_prop->name) == 0) {
        IUFillNumber(brightness_prop, values, names, n);
        brightness_prop->s = IPS_OK;
         if (isConnected()) {
            char command[32];
            snprintf(command, sizeof(command), "B%03d\n", (int)brightness_prop->np[0].value); //format
            if (!sendCommand(command)) {
                 IDLog("Failed to send brightness command.\n");
                brightness_prop->s = IPS_ALERT; // Set property state to alert.
            }
         }
        IDSetNumber(brightness_prop, nullptr);
        return true;
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);

}

// Connect
bool SimpleFlat::Connect() {
    if (!serialConnection->connect()) {
         IDLog("Failed to connect to serial port.\n");
        connected = false;
        return false;
    }

    connected = true;

     // Get initial status (after a short delay, handled in TimerHit)
     SetTimer(1000);  // 1 second delay


    return true;
}

// Disconnect
bool SimpleFlat::Disconnect() {
    if(serialConnection)
    {
        serialConnection->disconnect();
    }
    connected = false;
    IDSetSwitch(connection_prop, nullptr); // Inform clients
    return true;
}

// sendCommand
bool SimpleFlat::sendCommand(const std::string &command) {
  if (!serialConnection || !serialConnection->isConnected()) {
        return false;
    }
    // Send the command.
   int nbytes = serialConnection->write(command.c_str(), command.length());
    if (nbytes != (int)command.length()) {
        IDLog("Error writing to serial port.\n");
        return false;
    }

    return true;
}

// readResponse (with timeout)
bool SimpleFlat::readResponse(std::string &response, int timeout_ms) {
   if (!serialConnection || !serialConnection->isConnected()) {
        return false;
    }
    char buffer[256] = {0}; // Initialize the buffer
   int nbytes = serialConnection->read(buffer, sizeof(buffer) - 1, "\n", timeout_ms);
    if (nbytes > 0) {
        buffer[nbytes] = '\0'; // Null-terminate
        response = buffer; // Assign to the string
        return true;
    }
    return false;
}

//getStatus
bool SimpleFlat::getStatus()
{
    if (!sendCommand("S\n")) {
        IDLog("Failed to send status request.\n");
        return false;
    }

    std::string response;
    if (!readResponse(response)) {
        IDLog("Failed to get status response.\n");
        return false;
    }
    IDLog("getStatus response : %s\n",response.c_str());
     // Parse the response: "brightness,servoPosition"
    int newBrightness, newServoPos;
    if (sscanf(response.c_str(), "%d,%d", &newBrightness, &newServoPos) == 2) {
        // Update the properties.
        currentBrightness = newBrightness;
        currentServoPos = newServoPos;
        brightness_prop->np[0].value = currentBrightness;
        IDSetNumber(brightness_prop, nullptr); // Inform client

        // Update the cover state (Open/Close).
        cover_prop->sp[0].s = (currentServoPos == 0)   ? ISS_ON : ISS_OFF;   // Open
        cover_prop->sp[1].s = (currentServoPos == 180) ? ISS_ON : ISS_OFF; // Close
        IDSetSwitch(cover_prop, nullptr);    // Inform client
        return true;
    }
     else {
        IDLog("Error parsing status response: %s\n", response.c_str());
        return false;
    }

}

bool SimpleFlat::UpdateProperties()
{
    INDI::DefaultDevice::UpdateProperties(); //parent update

     if (isConnected())
    {
        getStatus();

    }
     return true;
}

// TimerHit (for periodic updates, if needed)
void SimpleFlat::TimerHit() {
   if(isConnected())
   {
    if (!getStatus()) {
        IDLog("Error getting status in TimerHit.\n");
        // Consider disconnecting if communication is consistently failing.
    }
   }

    static struct timeval tv = {1, 0}; // 1 second interval
    IDSetTimer(&tv);

}

// --- INDI Driver Registration ---
//static SimpleFlat *driver = nullptr; // No global pointer needed
extern "C" int indi_init(void) {
    static SimpleFlat driver;  // Create a static instance
    driver.registerDriver();      // Register the driver
    return 0;
}
