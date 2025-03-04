#include "IndiFlatPanel.h"

bool IndiFlatPanel::initProperties()
{
    INDI::DefaultDevice::initProperties();

    IUFillNumber(&ServoPositionN[0], "SERVO_POSITION", "Servo Position", "%0.f", 0, 180, 1, 0);
    IUFillNumberVector(&ServoPositionNP, ServoPositionN, 1, getDeviceName(), "SERVO_CONTROL", "Servo Control", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    IUFillNumber(&LEDBrightnessN[0], "LED_BRIGHTNESS", "LED Brightness", "%0.f", 0, 255, 1, 0);
    IUFillNumberVector(&LEDBrightnessNP, LEDBrightnessN, 1, getDeviceName(), "LED_CONTROL", "LED Control", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    serialConnection = new Connection::Serial(this);
    registerConnection(serialConnection);

    return true;
}

bool IndiFlatPanel::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    if (isConnected())
    {
        defineNumber(&ServoPositionNP);
        defineNumber(&LEDBrightnessNP);
    }
    else
    {
        deleteProperty(ServoPositionNP.name);
        deleteProperty(LEDBrightnessNP.name);
    }
    return true;
}

bool IndiFlatPanel::Connect()
{
    if (!serialConnection->Connect())
        return false;
    return true;
}

bool IndiFlatPanel::Disconnect()
{
    return serialConnection->Disconnect();
}

bool IndiFlatPanel::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (!strcmp(dev, getDeviceName()))
    {
        if (!strcmp(name, ServoPositionNP.name))
        {
            ServoPositionN[0].value = values[0];
            char command[16];
            snprintf(command, sizeof(command), "SERVO %d", (int)values[0]);
            serialConnection->sendCommand(command);
            IDSetNumber(&ServoPositionNP, nullptr);
            return true;
        }
        else if (!strcmp(name, LEDBrightnessNP.name))
        {
            LEDBrightnessN[0].value = values[0];
            char command[16];
            snprintf(command, sizeof(command), "LED %d", (int)values[0]);
            serialConnection->sendCommand(command);
            IDSetNumber(&LEDBrightnessNP, nullptr);
            return true;
        }
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}
