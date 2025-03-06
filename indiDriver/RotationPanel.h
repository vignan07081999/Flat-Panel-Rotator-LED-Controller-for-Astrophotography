#ifndef ROTATION_PANEL_H
#define ROTATION_PANEL_H

#include "indicom.h"  // For serial communication

class RotationPanel : public INDI::DefaultDevice
{
public:
    RotationPanel(const char *port);
    ~RotationPanel();

    bool Connect();
    bool Disconnect();
    const char *getDefaultName() override { return "Rotation Panel"; }
    bool initProperties() override;
    void ISGetProperties(const char *dev) override;
    void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;

private:
    INDI::NumberVectorProperty servoPositionNP;
    INDI::NumberVectorProperty ledBrightnessNP;
    
    INDI::Connection *connection; // Serial port connection
    char portName[64];
    bool isConnected;

    bool sendCommand(const char *command);
    bool receiveResponse(char *buffer, size_t bufferSize, int timeout_ms = 500);
    void updatePropertyStates(); // Update INDI property states (IU_IDLE, IU_BUSY, etc)
};

#endif // ROTATION_PANEL_H
