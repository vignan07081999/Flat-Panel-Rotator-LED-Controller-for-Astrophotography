#ifndef ROTATION_PANEL_H
#define ROTATION_PANEL_H

#include <libindi/indicom.h>
#include <libindi/indibase.h>
#include <libindi/basedevice.h> // Crucial:  Includes DefaultDevice definition
#include <string> // For std::string
#include <cstring> //for string manipulation functions


class RotationPanel : public INDI::DefaultDevice
{
public:
    RotationPanel(const char *port);
    ~RotationPanel();

    bool Connect() override;       // Added override
    bool Disconnect() override;    // Added override
    const char *getDefaultName() override;
    bool initProperties() override;
    void ISGetProperties(const char *dev) override;
    void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;

private:
    INDI::NumberVectorProperty *servoPositionNP;  // Use pointers
    INDI::NumberVectorProperty *ledBrightnessNP; // Use pointers

    INDI::Connection *connection; // Serial port connection
    char portName[64];
    bool isConnected;
    int BAUD_RATE = 115200; //Moved Baud Rate here

    bool sendCommand(const char *command);
    bool receiveResponse(char *buffer, size_t bufferSize, int timeout_ms = 500);
    void updatePropertyStates();
    void logError(const std::string& message); // Corrected parameter type
};

#endif // ROTATION_PANEL_H
