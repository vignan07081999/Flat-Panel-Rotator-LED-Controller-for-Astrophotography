#include <memory>
#include <indicom.h>
#include <indidevapi.h>
#include <fcntl.h>   // For file control options (open, etc.)
#include <termios.h> // For serial port settings
#include <unistd.h>  // For POSIX functions (read, write, close)
#include <errno.h>   // For error handling (errno)
#include <stdarg.h>  // For variable argument lists (logging)
#include <string.h>

class INDIFlatField : public INDI::DefaultDriver {
public:
    INDIFlatField();
    ~INDIFlatField();

    // --- INDI Driver Methods ---
    bool ISGetProperties(const char *dev);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *names[], int n);  // We probably won't use BLOBs
    bool ISSnoopDevice(XMLEle *root); //optional
    bool UpdateProperties();
    void TimerHit(); //for timer
    bool Connect();
    bool Disconnect();
    const char *getDefaultName()  { return "FlatField"; }; //device name

private:
    // --- Serial Communication ---
    int fd; // File descriptor for the serial port
    bool connected;

    // --- Properties ---
    // Connection
    ISwitchVectorProperty *connection_prop;

    // Servo Position
    INumberVectorProperty *servo_prop;
    double target_servo_pos;
    double current_servo_pos;

    // LED Brightness
    INumberVectorProperty *led_prop;
    double target_led_brightness;
    double current_led_brightness;

    // --- Helper Functions ---
    bool openSerialPort();
    void closeSerialPort();
    bool sendCommand(const char *command);
    bool readFeedback();
    void log(const char *format, ...);  // Logging function
};

// --- Constructor ---
INDIFlatField::INDIFlatField() {
    fd = -1; // Initialize file descriptor to invalid
    connected = false;
    target_servo_pos = 90;    // Default values
    current_servo_pos = 90;
    target_led_brightness = 0;
    current_led_brightness = 0;
    setVersion(1, 0); // Set driver version
}

// --- Destructor ---
INDIFlatField::~INDIFlatField() {}

// --- ISGetProperties ---
bool INDIFlatField::ISGetProperties(const char *dev) {
    // Call the base class implementation first
    INDI::DefaultDriver::ISGetProperties(dev);

    // Define Properties
    // Connection
    static ISwitch connection_switch[] = {
        {const_cast<char *>("CONNECT"),   "Connect",   ISS_OFF},
        {const_cast<char *>("DISCONNECT"), "Disconnect", ISS_ON}
    };
     connection_prop = newSwitch(&connection_switch,2, getDeviceName(), "CONNECTION",IP_RW, ISRule::ONE_OF_MANY, 60, IPS_IDLE);

    // Servo Position
    static INumber servo_numbers[] = {
        {const_cast<char *>("SERVO_POS"), "Position", "%.1f", 0, 180, 1, 90},
    };
     servo_prop = newNumber(&servo_numbers,1,getDeviceName(), "SERVO", IP_RW, 0, IPS_IDLE);

    // LED Brightness
    static INumber led_numbers[] = {
        {const_cast<char *>("LED_BRIGHT"), "Brightness", "%.0f", 0, 255, 1, 0},
    };
     led_prop = newNumber(&led_numbers,1,getDeviceName(), "LED", IP_RW, 0, IPS_IDLE);

    // Add properties to the driver
     defineSwitch(connection_prop);
     defineNumber(servo_prop);
     defineNumber(led_prop);

    return true;
}

// --- ISNewSwitch ---
bool INDIFlatField::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {
    // Call the base class implementation first
   if(strcmp(name, connection_prop->name) == 0)
   {
        IUFillSwitch(connection_prop, states, names, n);
        connection_prop->s = IPS_OK;

        if(connection_prop->sp[0].s == ISS_ON) //connect
        {
            if(Connect())
            {
                connection_prop->sp[0].s = ISS_ON; //connect
                connection_prop->sp[1].s = ISS_OFF; //disconnect
                IDSetSwitch(connection_prop, nullptr);
                return true;
            }
            else{
                connection_prop->sp[0].s = ISS_OFF; //connect
                connection_prop->sp[1].s = ISS_ON; //disconnect
                IDSetSwitch(connection_prop, nullptr);
                return false;
            }
        }
        else if (connection_prop->sp[1].s == ISS_ON) //Disconnect
        {
             if(Disconnect())
            {
                connection_prop->sp[0].s = ISS_OFF; //connect
                connection_prop->sp[1].s = ISS_ON; //disconnect
                IDSetSwitch(connection_prop, nullptr);
                return true;
            }
            else{
                connection_prop->sp[0].s = ISS_ON; //connect
                connection_prop->sp[1].s = ISS_OFF; //disconnect
                IDSetSwitch(connection_prop, nullptr);
                return false;
            }
        }
        else
        {
            IDSetSwitch(connection_prop, nullptr); //inform client of changes
        }
        return true;

   }
    return INDI::DefaultDriver::ISNewSwitch(dev, name, states, names, n);
}

// --- ISNewNumber ---
bool INDIFlatField::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {
   if (strcmp(name, servo_prop->name) == 0) {
        IUFillNumber(servo_prop, values, names, n);
        target_servo_pos = servo_prop->np[0].value;  // Update target servo position
        servo_prop->s = IPS_OK;
         if(connected)
         {
            char command[32];
            snprintf(command, sizeof(command), "S%.0f\n", target_servo_pos);
            if (!sendCommand(command)) {
               log("Failed to send servo command");
               servo_prop->s = IPS_ALERT;
            }
         }
        IDSetNumber(servo_prop, nullptr); //inform client of changes
        return true;
    } else if (strcmp(name, led_prop->name) == 0) {
        IUFillNumber(led_prop, values, names, n);
        target_led_brightness = led_prop->np[0].value;  // Update target LED brightness
        led_prop->s = IPS_OK;
        if(connected)
        {
            char command[32];
            snprintf(command, sizeof(command), "L%.0f\n", target_led_brightness);
                if (!sendCommand(command)) {
                    log("Failed to send LED command");
                    led_prop->s = IPS_ALERT; // Set property state to alert
                }
        }
        IDSetNumber(led_prop, nullptr); //inform client of changes
        return true;
    }
    return INDI::DefaultDriver::ISNewNumber(dev, name, values, names, n);
}

// --- ISNewText ---  (We don't have any text properties)
bool INDIFlatField::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) {
     return INDI::DefaultDriver::ISNewText(dev, name, texts, names, n);
}

// --- ISNewBLOB --- (We don't have any BLOB properties)
bool INDIFlatField::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *names[], int n) {
   return INDI::DefaultDriver::ISNewBLOB(dev,name,sizes,blobsizes,blobs,names,n);
}
bool INDIFlatField::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDriver::ISSnoopDevice(root);
}
// --- UpdateProperties ---
bool INDIFlatField::UpdateProperties() {
    INDI::DefaultDriver::UpdateProperties();
    return true;
}

// --- TimerHit ---
void INDIFlatField::TimerHit() {

    if (connected) {
        if (!readFeedback()) {
            log("Error reading feedback from device.");
             // Potentially set an error state on properties
        }
    }
    static struct timeval tv = {0, 100000}; // 100 ms, adjust as needed
    IDSetTimer(&tv); // reschedule timer
}

// --- Connect ---
bool INDIFlatField::Connect() {
    if (!openSerialPort()) {
        log("Failed to open serial port.");
        connected = false;
       return false;
    }

    connected = true;
    log("Device connected.");
     // Set timer for regular updates (e.g., every 100ms)
    static struct timeval tv = {0, 100000};  // 100ms
    IDSetTimer(&tv);

    //request initial values
    if (!sendCommand("F\n")) {
        log("Failed to send initial feedback request.");
    }
    return true;
}

// --- Disconnect ---
bool INDIFlatField::Disconnect() {
    closeSerialPort();
    connected = false;
    log("Device disconnected.");
    IDSetSwitch(connection_prop, nullptr); //inform client of changes
    return true;
}

// --- openSerialPort ---
bool INDIFlatField::openSerialPort() {
    // Try a common serial port.  "/dev/ttyACM0" is common for Arduinos
    // You might need to determine the correct port dynamically.
   const char *portName = "/dev/ttyACM0"; // Try ttyACM0 first

    fd = ::open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        portName = "/dev/ttyUSB0"; // If failed, try ttyUSB0
        fd = ::open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
        if(fd == -1)
        {
                return false;
        }
    }

    // Set serial port parameters (115200 baud, 8N1, etc.)
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
    options.c_cc[VMIN] = 0; // Non-blocking read
    options.c_cc[VTIME] = 0; // Non-blocking read
    tcsetattr(fd, TCSANOW, &options);

    // Give some time to arduino for initialization
    sleep(2);
    //flush
    tcflush(fd, TCIOFLUSH);
    return true;
}

// --- closeSerialPort ---
void INDIFlatField::closeSerialPort() {
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

// --- sendCommand ---
bool INDIFlatField::sendCommand(const char *command) {
    if (fd == -1) {
        return false; // Not connected
    }

    ssize_t bytes_written = write(fd, command, strlen(command));
    if (bytes_written != (ssize_t)strlen(command)) {
        log("Error writing to serial port.");
        return false;
    }
    return true;
}

// --- readFeedback ---
bool INDIFlatField::readFeedback() {
    if (fd == -1) {
        return false;  // Not connected
    }
    char buffer[256];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        //log("Raw feedback: %s", buffer);   // Debugging: Log raw data
        // Parse feedback: "SP:90,LB:128\n"
        char *sp_start = strstr(buffer, "SP:");
        char *lb_start = strstr(buffer, "LB:");

        if (sp_start && lb_start) {
           int servo, led;
           if (sscanf(sp_start, "SP:%d", &servo) == 1 && sscanf(lb_start, "LB:%d", &led) == 1) {
              current_servo_pos = servo;
              current_led_brightness = led;
              //Update the values in the properties
              servo_prop->np[0].value = current_servo_pos;
              led_prop->np[0].value = current_led_brightness;
              IDSetNumber(servo_prop,nullptr); //inform about update
              IDSetNumber(led_prop,nullptr);  //inform about update
           } else {
              log("Error parsing feedback: %s", buffer);
           }
        } else if(bytes_read>0) {
            // Print to log other messages from Arduino
             buffer[bytes_read] = '\0';
            log("%s", buffer);
        }
        return true;

    } else if(bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        //reading error
         log("Error reading from serial port: %s", strerror(errno));
         closeSerialPort(); // Try closing and reopening
         connected = false;
         if (Connect())
            log("Reconnected after the error");

         return false;
    }
    return true;
}

// --- Logging ---
void INDIFlatField::log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[1024]; //increase buffer size
    vsnprintf(buffer, sizeof(buffer), format, args);
    IDLog("%s\n", buffer);
    va_end(args);
}

// --- INDI Driver Registration ---

static INDIFlatField *driver = nullptr; //global var

extern "C" {
    INDI_DRIVER_MAIN
    {
        if(driver == nullptr) //check if driver object exists
        {
                driver = new INDIFlatField();
        }
        //driver = new INDIFlatField(); //removed singleton to allow multiple instances of the driver
        // Set driver properties
        driver->setVersion(1, 0);
        // Register the driver with INDI
        driver->registerDriver(argc, argv);

        return 0;
    }
}
