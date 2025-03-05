#include <memory>
#include "indidriver.h"

class FlatFieldPanel : public INDI::DefaultDevice
{
public:
    FlatFieldPanel() {}
    virtual ~FlatFieldPanel() {}

protected:
    bool initProperties() override;
    bool updateProperties() override;
    void ISGetProperties(const char *dev) override;
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

private:
    void sendCommand(const std::string &command);
    bool processResponse();
    int fd;
    
    INumberVectorProperty ServoPosNP;
    INumber ServoPosN[1];
    
    INumberVectorProperty LEDBrightnessNP;
    INumber LEDBrightnessN[1];
};

bool FlatFieldPanel::initProperties()
{
    INDI::DefaultDevice::initProperties();

    ServoPosN[0].min = 0;
    ServoPosN[0].max = 180;
    ServoPosN[0].value = 90;
    ServoPosN[0].step = 1;
    IUFillNumberVector(&ServoPosNP, ServoPosN, 1, getDeviceName(), "Servo Position", "Servo", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    LEDBrightnessN[0].min = 0;
    LEDBrightnessN[0].max = 255;
    LEDBrightnessN[0].value = 0;
    LEDBrightnessN[0].step = 1;
    IUFillNumberVector(&LEDBrightnessNP, LEDBrightnessN, 1, getDeviceName(), "LED Brightness", "Brightness", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    return true;
}

bool FlatFieldPanel::updateProperties()
{
    if (isConnected())
    {
        defineNumber(&ServoPosNP);
        defineNumber(&LEDBrightnessNP);
    }
    else
    {
        deleteProperty(ServoPosNP.name);
        deleteProperty(LEDBrightnessNP.name);
    }
    return true;
}

bool FlatFieldPanel::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (!strcmp(name, ServoPosNP.name))
    {
        ServoPosN[0].value = values[0];
        sendCommand("S" + std::to_string((int)ServoPosN[0].value));
        IDSetNumber(&ServoPosNP, nullptr);
        return true;
    }
    else if (!strcmp(name, LEDBrightnessNP.name))
    {
        LEDBrightnessN[0].value = values[0];
        sendCommand("L" + std::to_string((int)LEDBrightnessN[0].value));
        IDSetNumber(&LEDBrightnessNP, nullptr);
        return true;
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

void FlatFieldPanel::sendCommand(const std::string &command)
{
    if (fd > 0)
    {
        write(fd, command.c_str(), command.size());
    }
}

bool FlatFieldPanel::processResponse()
{
    char response[32];
    if (fd > 0 && read(fd, response, sizeof(response)) > 0)
    {
        LOGF_INFO("Flat Panel Response: %s", response);
        return true;
    }
    return false;
}

void ISGetProperties(const char *dev)
{
    FlatFieldPanel panel;
    panel.ISGetProperties(dev);
}

bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    FlatFieldPanel panel;
    return panel.ISNewNumber(dev, name, values, names, n);
}

bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    FlatFieldPanel panel;
    return panel.ISNewSwitch(dev, name, states, names, n);
}

extern "C" void ISInit()
{
    static std::unique_ptr<FlatFieldPanel> panel = std::make_unique<FlatFieldPanel>();
    panel->initProperties();
}
