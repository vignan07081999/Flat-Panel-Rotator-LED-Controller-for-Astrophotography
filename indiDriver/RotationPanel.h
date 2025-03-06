#ifndef ROTATION_PANEL_H
#define ROTATION_PANEL_H

// THESE ARE ALL REQUIRED IN THE HEADER NOW:
#include <libindi/indicom.h>
#include <libindi/indibase.h>
#include <libindi/basedevice.h>
#include <string>
#include <cstring>  // For string manipulation functions

class RotationPanel : public INDI::DefaultDevice
{
public:
    RotationPanel(const char *port);
    ~RotationPanel();

    // Virtual function overrides.  These MUST match the base class signatures.
    bool Connect() override;
    bool Disconnect() override;
    const char *getDefaultName() override;
    bool initProperties() override;
    void ISGetProperties(const char *dev) override;
    void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;


private:
    INDI::NumberVectorProperty *servoPositionNP;
    INDI::NumberVectorProperty *ledBrightnessNP;

    INDI::Connection *connection; // Serial port connection
    char portName[64];
    bool isConnected;
    int  BAUD_RATE = 115200;

    bool sendCommand(const char *command);
    bool receiveResponse(char *buffer, size_t bufferSize, int timeout_ms = 500);
    void updatePropertyStates();
    void logError(const std::string& message);
};

#endif // ROTATION_PANEL_H
