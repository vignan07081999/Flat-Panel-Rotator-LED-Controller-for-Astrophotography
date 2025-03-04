#ifndef FLATFIELD_DRIVER_H
#define FLATFIELD_DRIVER_H

#include <basedevice.h>
#include <indicom.h>  // For serial communication

class FlatFieldDriver : public INDI::DefaultDriver {
public:
    FlatFieldDriver();
    ~FlatFieldDriver();

    // INDI Driver Methods
    bool ISGetProperties(const char *dev);
    bool initProperties();
    bool ConnectDevice(const char *port);
    bool DisconnectDevice(const char *port);
    const char *getDefaultPort();
    bool UpdateProperties();
    void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    // Add ISNewText if you add text properties later

private:
    // Serial Communication
    INDI::BaseDevice::SerialPort *serialPort;
    bool isConnected;

    // INDI Properties
    INDI::NumberVectorProperty *servoProperty;
    INDI::NumberVectorProperty *ledProperty;
    INDI::TextVectorProperty *logProperty; //for logs

    // Internal Variables
    int currentServoPos;
    int currentLEDBrightness;

    // Helper Functions
    bool sendCommand(const char *command);
    void processFeedback(const char *feedback);
    void log(const char *message);

    //timer variables
     double timer;
    bool timer_flag;
    int count;
};

#endif // FLATFIELD_DRIVER_H
