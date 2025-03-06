#!/usr/bin/env python3

from astropy.io import fits
import PyIndi
import time
import sys
import threading

class FlatPanelDriver(PyIndi.BaseClient):
    def __init__(self):
        super(FlatPanelDriver, self).__init__()
        self.device = None
        self.port = "/dev/ttyACM0"  #  CHANGE THIS TO YOUR ARDUINO'S PORT!
        self.baud = 9600
        self.connected = False
        self.servo_pos = 90  # Initial values
        self.led_brightness = 128
        self.is_moving = False # Flag to track if servo is moving.


    def ISNewNumber(self, dev, name, values, names):
        if dev != self.device_name:
            return

        if name == "SERVO_CONTROL":
            for value, value_name in zip(values, names):
                if value_name == "SERVO_POSITION":
                    self.servo_pos = int(value)
                    self.send_servo_command()
                elif value_name == "SERVO_ABORT": #Added for Abort
                    self.send_command("SERVO:90") #Default position for Abort
                    self.is_moving = False

        elif name == "LED_CONTROL":
            for value, value_name in zip(values, names):
                if value_name == "LED_BRIGHTNESS":
                    self.led_brightness = int(value)
                    self.send_led_command()


    def ISNewSwitch(self, dev, name, states, names):
        if dev != self.device_name:
            return
        if name == "CONNECTION":
            for state, state_name in zip(states, names):
                if state_name == "CONNECT":
                    if state == PyIndi.ISS_ON:
                        self.connect_device()
                    else:
                        self.disconnect_device()
        if name == "SERVO_PRESETS":
            for state, state_name in zip(states, names):
                if state == PyIndi.ISS_ON:
                    if state_name == "SERVO_OPEN":
                        self.servo_pos = 0
                        self.send_servo_command()
                    elif state_name == "SERVO_CLOSE":
                        self.servo_pos = 180
                        self.send_servo_command()
        if name == "LED_PRESETS":
             for state, state_name in zip(states, names):
                if state == PyIndi.ISS_ON:
                    if state_name == "LED_FULL":
                        self.led_brightness = 255
                        self.send_led_command()
                    elif state_name == "LED_HALF":
                        self.led_brightness = 128
                        self.send_led_command()
                    elif state_name == "LED_OFF":
                        self.led_brightness = 0
                        self.send_led_command()

    def connect_device(self):
        try:
            import serial
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            self.connected = True
            self.update_connection_status()
            print("Device connected.")
            # Get initial values after connecting
            self.get_initial_values()
        except serial.SerialException as e:
            print(f"Error connecting to device: {e}")
            self.connected = False
            self.update_connection_status()

    def disconnect_device(self):
        if self.connected:
            self.ser.close()
            self.connected = False
            self.update_connection_status()
            print("Device disconnected.")
        
    def update_connection_status(self):
       isConnected = PyIndi.ISS_ON if self.connected else PyIndi.ISS_OFF
       isDisconnected = PyIndi.ISS_OFF if self.connected else PyIndi.ISS_ON
       self.sendSwitch(self.device_name, "CONNECTION", [isDisconnected, isConnected], ["DISCONNECT", "CONNECT"])


    def send_command(self, command):
        if self.connected:
            try:
                self.ser.write((command + '\n').encode('utf-8'))
                response = self.ser.readline().decode('utf-8').strip()
                print(f"Sent: {command}, Received: {response}")  # Debugging
                return response
            except Exception as e:
                print(f"Error sending command: {e}")
                self.connected = False
                self.update_connection_status() #On error disconnect
                return None

    def send_servo_command(self):
        if not self.is_moving: #Only send servo if not already moving
            self.is_moving = True
            self.send_command(f"SERVO:{self.servo_pos}")
            self.sendNumber(self.device_name, "SERVO_CONTROL", [self.servo_pos], ["SERVO_POSITION"])
            self.is_moving = False #Reset the flag

    def send_led_command(self):
        self.send_command(f"LED:{self.led_brightness}")
        self.sendNumber(self.device_name, "LED_CONTROL", [self.led_brightness], ["LED_BRIGHTNESS"])

    def get_initial_values(self):
        """Gets initial servo and LED values from the Arduino."""
        servo_response = self.send_command("GET:SERVO")
        if servo_response and servo_response.startswith("OK:SERVO:"):
            try:
                self.servo_pos = int(servo_response.split(":")[2])
                self.sendNumber(self.device_name, "SERVO_CONTROL", [self.servo_pos], ["SERVO_POSITION"])
            except ValueError:
                print("Error parsing initial servo value.")

        led_response = self.send_command("GET:LED")
        if led_response and led_response.startswith("OK:LED:"):
            try:
                self.led_brightness = int(led_response.split(":")[2])
                self.sendNumber(self.device_name, "LED_CONTROL", [self.led_brightness], ["LED_BRIGHTNESS"])
            except ValueError:
                print("Error parsing initial LED value.")

    def ISGetProperties(self, dev):
        self.define_properties()

    def define_properties(self):
        # --- Connection ---
        self.device_name = "Flat Panel"  # The name that will appear in EKOS
        connection_property = self.defSwitch(self.device_name, "CONNECTION", "Connect/Disconnect",
                                            PyIndi.ISR_1OFMANY, PyIndi.IP_RW, ["DISCONNECT", "CONNECT"])
        # --- Servo Control ---
        servo_control_property = self.defNumber(self.device_name, "SERVO_CONTROL", "Servo Control",
                                            PyIndi.IP_RW, [0], ["SERVO_POSITION"])
        servo_control_property[0].min = 0
        servo_control_property[0].max = 180
        servo_control_property[0].step = 1
        servo_control_property[0].value = self.servo_pos

        servo_abort_property = self.defNumber(self.device_name, "SERVO_CONTROL", "Servo Control",
                                          PyIndi.IP_WO, [0], ["SERVO_ABORT"]) #WO - Write Only
        servo_abort_property[0].min = 0
        servo_abort_property[0].max = 0
        servo_abort_property[0].step = 0
        servo_abort_property[0].value = 0


        # --- Servo Presets ---
        servo_presets_property = self.defSwitch(self.device_name, "SERVO_PRESETS", "Servo Presets",
                                                 PyIndi.ISR_1OFMANY, PyIndi.IP_RW, ["SERVO_OPEN", "SERVO_CLOSE"])

        # --- LED Control ---
        led_control_property = self.defNumber(self.device_name, "LED_CONTROL", "LED Control",
                                             PyIndi.IP_RW, [0], ["LED_BRIGHTNESS"])
        led_control_property[0].min = 0
        led_control_property[0].max = 255
        led_control_property[0].step = 1
        led_control_property[0].value = self.led_brightness

        # --- LED Presets ---
        led_presets_property = self.defSwitch(self.device_name, "LED_PRESETS", "LED Presets",
                                                PyIndi.ISR_1OFMANY, PyIndi.IP_RW, ["LED_OFF", "LED_HALF", "LED_FULL"])
        self.update_connection_status() #Initial connection status


if __name__ == "__main__":
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = "/dev/ttyACM0"  # Default port.  CHANGE THIS IF NEEDED

    flat_panel = FlatPanelDriver()
    flat_panel.setServer("localhost", 7624)
    flat_panel.port = port  # Set the port from command line or default
    if not flat_panel.connectServer():
        print(f"Error: Could not connect to INDI server.  Is indiserver running?")
        sys.exit(1)

    print(f"INDI Flat Panel driver running.  Connect in EKOS to 'Flat Panel'.")
    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        flat_panel.disconnectServer()
        print("Driver exiting.")
        sys.exit(0)
