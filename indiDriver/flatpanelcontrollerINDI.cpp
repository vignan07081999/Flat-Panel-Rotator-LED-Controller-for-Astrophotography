#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <cstdlib> // For atexit()

// Serial Port Headers
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// INDI Headers
#include <libindi/indicom.h>
#include <libindi/indidevapi.h>

class INDIFlatField : public INDI::DefaultDriver {
public:
    INDIFlatField();
    ~INDIFlatField();

    bool ISGetProperties(const char *dev);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *names[], int n);
    bool ISSnoopDevice(XMLEle *root);
    bool UpdateProperties();
    void TimerHit();
    bool Connect();
    bool Disconnect();
    const char *getDefaultName() { return "FlatField"; };

private:
    int fd = -1; // Serial port file descriptor
    bool connected = false;

    ISwitchVectorProperty *connection_prop;
    INumberVectorProperty *servo_prop;
    double target_servo_pos = 90;
    double current_servo_pos = 90;
    INumberVectorProperty *led_prop;
    double target_led_brightness = 0;
    double current_led_brightness = 0;

    ITextVectorProperty *port_prop;  //Port selection
    std::string selectedPort;     // Store the selected port


    bool openSerialPort();
    void closeSerialPort();
    bool sendCommand(const char *command);
    bool readFeedback();
    void log(const char *format, ...);
    std::string findSerialPort();
};

INDIFlatField::INDIFlatField() {
    setVersion(1, 0);
    selectedPort = "";
    // Register a cleanup function to close the serial port on exit
     atexit([]() {
        if (driver) {
            driver->Disconnect(); // Cleanly disconnect if possible
        }
    });
}
INDIFlatField::~INDIFlatField() {}

bool INDIFlatField::ISGetProperties(const char *dev) {
    INDI::DefaultDriver::ISGetProperties(dev);

    static ISwitch connection_switch[] = {
        {const_cast<char *>("CONNECT"),   "Connect",   ISS_OFF},
        {const_cast<char *>("DISCONNECT"), "Disconnect", ISS_ON}
    };
    connection_prop = newSwitch(&connection_switch, 2, getDeviceName(), "CONNECTION", IP_RW, ISRule::ONE_OF_MANY, 60, IPS_IDLE);

    static INumber servo_numbers[] = {
        {const_cast<char *>("SERVO_POS"), "Position", "%.1f", 0, 180, 1, 90},
    };
    servo_prop = newNumber(&servo_numbers, 1, getDeviceName(), "SERVO", IP_RW, 0, IPS_IDLE);

    static INumber led_numbers[] = {
        {const_cast<char *>("LED_BRIGHT"), "Brightness", "%.0f", 0, 255, 1, 0},
    };
    led_prop = newNumber(&led_numbers, 1, getDeviceName(), "LED", IP_RW, 0, IPS_IDLE);

     // Serial Port Selection Property
    static IText port_texts[] = {
        {const_cast<char *>("PORT"), "Port", const_cast<char *>(""),} // Will be filled dynamically
    };
    port_prop = newText(&port_texts, 1, getDeviceName(), "SETTINGS", IP_RW, 0, IPS_IDLE);

    defineSwitch(connection_prop);
    defineNumber(servo_prop);
    defineNumber(led_prop);
    defineText(port_prop);
    return true;
}

bool INDIFlatField::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {
    if (strcmp(name, connection_prop->name) == 0) {
        IUFillSwitch(connection_prop, states, names, n);
        connection_prop->s = IPS_OK;
        if (connection_prop->sp[0].s == ISS_ON) {
            if (Connect()) {
                connection_prop->sp[0].s = ISS_ON;
                connection_prop->sp[1].s = ISS_OFF;
            } else {
                connection_prop->sp[0].s = ISS_OFF;
                connection_prop->sp[1].s = ISS_ON;
            }
        } else if (connection_prop->sp[1].s == ISS_ON) {
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
    return INDI::DefaultDriver::ISNewSwitch(dev, name, states, names, n);
}

bool INDIFlatField::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {
    if (strcmp(name, servo_prop->name) == 0) {
        IUFillNumber(servo_prop, values, names, n);
        target_servo_pos = servo_prop->np[0].value;
        servo_prop->s = IPS_OK;
        if (connected) {
            char command[32];
            snprintf(command, sizeof(command), "S%.0f\n", target_servo_pos);
            if (!sendCommand(command)) {
                log("Failed to send servo command");
                servo_prop->s = IPS_ALERT;
            }
        }
        IDSetNumber(servo_prop, nullptr);
        return true;
    } else if (strcmp(name, led_prop->name) == 0) {
        IUFillNumber(led_prop, values, names, n);
        target_led_brightness = led_prop->np[0].value;
        led_prop->s = IPS_OK;
        if (connected) {
            char command[32];
            snprintf(command, sizeof(command), "L%.0f\n", target_led_brightness);
            if (!sendCommand(command)) {
                log("Failed to send LED command");
                led_prop->s = IPS_ALERT;
            }
        }
        IDSetNumber(led_prop, nullptr);
        return true;
    }
    return INDI::DefaultDriver::ISNewNumber(dev, name, values, names, n);
}

bool INDIFlatField::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(name, port_prop->name) == 0) {
        IUFillText(port_prop, texts, names, n);
        selectedPort = port_prop->tp[0].text; // Store the selected port
        port_prop->s = IPS_OK;
        IDSetText(port_prop, nullptr); //inform client
        return true;
    }
    return INDI::DefaultDriver::ISNewText(dev, name, texts, names, n);
}

bool INDIFlatField::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *names[], int n) {
    return INDI::DefaultDriver::ISNewBLOB(dev, name, sizes, blobsizes, blobs, names, n);
}
bool INDIFlatField::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDriver::ISSnoopDevice(root);
}

bool INDIFlatField::UpdateProperties() {
    INDI::DefaultDriver::UpdateProperties();
    // Add dynamic port list update *before* connecting
    if (isConnected() == false) {
        std::string foundPort = findSerialPort();
        if (!foundPort.empty()) {
            port_prop->tp[0].text = strdup(foundPort.c_str()); // Update the text property
            IDSetText(port_prop, nullptr); // Inform clients of the change
        }
    }
    return true;
}

void INDIFlatField::TimerHit() {
    if (connected) {
        if (!readFeedback()) {
            log("Error reading feedback from device.");
        }
    }
    static struct timeval tv = {0, 100000};
    IDSetTimer(&tv);
}

bool INDIFlatField::Connect() {
   if (selectedPort.empty()) { // Use the selected port
        log("No serial port selected.");
        return false;
    }

    if (!openSerialPort()) {
      log("Failed to open serial port: %s", selectedPort.c_str());
        connected = false;
        return false;
    }

    connected = true;
    log("Device connected to %s.", selectedPort.c_str());
    static struct timeval tv = {0, 100000};
    IDSetTimer(&tv);
    if (!sendCommand("F\n")) {
        log("Failed to send initial feedback request.");
    }
    return true;
}

bool INDIFlatField::Disconnect() {
    closeSerialPort();
    connected = false;
    log("Device disconnected.");
     IDSetSwitch(connection_prop, nullptr); //inform client of changes
    return true;
}

bool INDIFlatField::openSerialPort() {
    fd = ::open(selectedPort.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        return false;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &options);
    sleep(2); //wait for initialization
    tcflush(fd, TCIOFLUSH); //flush
    return true;
}

void INDIFlatField::closeSerialPort() {
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

bool INDIFlatField::sendCommand(const char *command) {
    if (fd == -1) return false;
    ssize_t bytes_written = write(fd, command, strlen(command));
    if (bytes_written != (ssize_t)strlen(command)) {
        log("Error writing to serial port.");
        return false;
    }
    return true;
}

bool INDIFlatField::readFeedback() {
    if (fd == -1) return false;
    char buffer[256];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        char *sp_start = strstr(buffer, "SP:");
        char *lb_start = strstr(buffer, "LB:");
        if (sp_start && lb_start) {
            int servo, led;
            if (sscanf(sp_start, "SP:%d", &servo) == 1 && sscanf(lb_start, "LB:%d", &led) == 1) {
                current_servo_pos = servo;
                current_led_brightness = led;
                servo_prop->np[0].value = current_servo_pos;
                led_prop->np[0].value = current_led_brightness;
                IDSetNumber(servo_prop, nullptr);
                IDSetNumber(led_prop, nullptr);
            } else {
                log("Error parsing feedback: %s", buffer);
            }
        } else if (bytes_read > 0) {
            log("%s", buffer);
        }
        return true;
    } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        log("Error reading from serial port: %s", strerror(errno));
        closeSerialPort();
        connected = false;
         if (Connect())
            log("Reconnected after the error");
        return false;
    }
    return true;
}

void INDIFlatField::log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    IDLog("%s\n", buffer);
    va_end(args);
}

// --- Dynamic Serial Port Detection (using libudev) ---
std::string INDIFlatField::findSerialPort() {
    struct udev *udev = udev_new();
    if (!udev) {
        log("Failed to create udev context.");
        return "";
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty"); // Look for tty devices
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *dev_list_entry;

    std::string foundPort = ""; // Store the found port

    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);

        // Check if it's an Arduino (by Vendor and Product ID)
        const char *vendor = udev_device_get_property_value(dev, "ID_VENDOR_ID");
        const char *product = udev_device_get_property_value(dev, "ID_MODEL_ID");

        //  Arduino Uno Vendor and Product IDs (adjust if yours is different!)
        if (vendor && product && (strcmp(vendor, "2341") == 0 || strcmp(vendor, "2a03")==0) && (strcmp(product, "0043") == 0 || strcmp(product, "0001") == 0 )) {
              const char *devnode = udev_device_get_devnode(dev); // Get device node like /dev/ttyACM0
               if (devnode) {
                  log("Found Arduino on port: %s", devnode);
                  foundPort = devnode; // Store the found port
                  udev_device_unref(dev);
                  break; // Stop after finding the first matching device
               }
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return foundPort;
}
// --- INDI Driver Registration ---
static INDIFlatField *driver = nullptr;
extern "C" {
    INDI_DRIVER_MAIN
    {
        if(driver == nullptr) //check if object exist
        {
            driver = new INDIFlatField();
        }
        driver->setVersion(1, 0);
        driver->registerDriver(argc, argv);
        return 0;
    }
}
