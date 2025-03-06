# Arduino-Controlled Rotating Flat Field Panel with Python GUI

This project provides a system for controlling a rotating flat field panel for astrophotography, using an Arduino Uno, a servo motor, an LED strip, and a Python-based graphical user interface (GUI). The system allows for precise control of the panel's rotation and LED brightness, with preset options and EEPROM storage for persistent settings.  It is designed to be used standalone, with communication between the Raspberry Pi (running the GUI) and the Arduino occurring over a serial connection.  

**Note:** This project does *not* include KStars/EKOS integration via INDI.  While INDI is the recommended approach for full integration with astrophotography software, this project focuses on providing a standalone, GUI-driven solution.

## Project Overview

The system consists of two main components:

1.  **Arduino Firmware (`FlatFieldPanel.ino`):**
    *   Controls the servo motor for panel rotation.
    *   Controls the LED strip brightness using PWM (Pulse Width Modulation).
    *   Reads and writes settings (servo position, LED brightness) to the Arduino's EEPROM to preserve them across power cycles.
    *   Communicates with the Raspberry Pi via serial communication, receiving commands and sending feedback.
    *   Implements a simple command protocol (e.g., "SERVO:120", "LED:200").

2.  **Python GUI (`gui.py`):**
    *   Runs on a Raspberry Pi (or any computer with Python and the required libraries).
    *   Provides a user-friendly interface with:
        *   A dropdown menu for selecting the serial port connected to the Arduino.
        *   A slider for controlling the servo position (0-180 degrees).
        *   A slider for controlling the LED brightness (0-255).
        *   Preset buttons for common servo positions (Open, Close) and LED brightness levels (Full, Half, Off).
        *   "Send" buttons to explicitly send commands to the Arduino based on the slider values.
        *   Feedback labels displaying the last known servo position and LED brightness.
        *   A real-time log window showing sent commands and received responses from the Arduino.
        *   A connect/disconnect button to establish or terminate the serial connection.
    *   Uses the `tkinter` library for the GUI and the `pyserial` library for serial communication.
    *   Implements a separate thread for handling incoming serial data from the Arduino, ensuring the GUI remains responsive.

## Hardware Requirements

*   Arduino Uno (or compatible)
*   Servo motor (e.g., SG90, MG996R) - Choose one with sufficient torque for your flat panel.
*   2-wire LED strip (addressable LED strips are *not* supported by this code; use a simple, single-color strip that can be controlled with PWM)
*   Resistor (value depends on your LED strip - typically 220-470 ohms) - Protects the Arduino's output pin.
*   Power supply for the LED strip (voltage and current rating depend on your strip)
*   Power supply for the Arduino (can be powered via USB from the Raspberry Pi)
*   Jumper wires
*   Raspberry Pi (any model; tested with Raspberry Pi 3 and 4) - or another computer to run the GUI.
*   (Optional) Breadboard for prototyping.

## Wiring Diagram



 **Servo:** Connect the servo's VCC (usually red) to the Arduino's 5V pin, GND (usually brown) to Arduino's GND, and the signal wire (usually orange or yellow) to Arduino digital pin 9.
*   **LED Strip:** Connect the LED strip's positive (+) wire to the positive terminal of your *external* LED power supply.  Connect the LED strip's negative (-) wire to the negative terminal of the power supply *AND* to the Arduino's GND.  Connect a resistor (220-470 ohms is a good starting point) between Arduino digital pin 10 and the LED strip's data/control wire (if it has one; for simple 2-wire strips, this is often just the positive wire).  **Do not power the LED strip directly from the Arduino's 5V pin, especially if it's a long or high-power strip.**
*   **Arduino Power:** The Arduino can be powered via its USB port, connected to the Raspberry Pi.

## Software Dependencies

*   **Arduino IDE:** To upload the firmware (`FlatFieldPanel.ino`) to the Arduino Uno.
*   **Python 3:**  The GUI is written in Python 3.
*   **pyserial:**  For serial communication. Install with: `pip install pyserial`
*   **tkinter:**  For the GUI.  Usually included with Python 3 installations, but you may need to install it separately on some systems (e.g., `sudo apt-get install python3-tk` on Debian/Raspberry Pi OS).

## Installation Steps

1.  **Arduino Setup:**
    *   Download the `FlatFieldPanel.ino` file.
    *   Open the file in the Arduino IDE.
    *   Connect your Arduino Uno to your computer via USB.
    *   Select the correct board (Arduino Uno) and port in the Arduino IDE (Tools menu).
    *   Upload the sketch to the Arduino.

2.  **Raspberry Pi (or other computer) Setup:**
    *   Ensure you have Python 3 and `pip` installed.
    *   Install `pyserial`:
        ```bash
        pip install pyserial
        ```
    *   If `tkinter` is not already installed (you'll get an error when running the GUI if it's missing), install it.  The specific command depends on your operating system.  On Debian/Raspberry Pi OS:
        ```bash
        sudo apt-get update
        sudo apt-get install python3-tk
        ```
    *   Download the `gui.py` file.

3.  **Running the GUI:**
    *   Connect the Arduino to the Raspberry Pi via USB.
    *   Open a terminal on the Raspberry Pi.
    *   Navigate to the directory where you saved `gui.py`.
    *   Run the GUI:
        ```bash
        python3 gui.py
        ```
    *   Select the correct serial port from the dropdown menu in the GUI.
    *   Click "Connect".
    *   Use the sliders and preset buttons to control the flat panel.

## Usage

*   **Serial Port Selection:**  Select the correct serial port from the dropdown that corresponds to your Arduino.
*   **Connect/Disconnect:** Use the "Connect" button to establish a serial connection.  The button will change to "Disconnect" when connected. Click "Disconnect" to close the connection.
*   **Servo Control:**  Use the servo slider to set the servo position (0-180 degrees). Click "Send Servo" to apply the setting.  Use the "Open (0)" and "Close (180)" preset buttons for quick positioning.
*   **LED Control:** Use the LED slider to set the brightness (0-255). Click "Send LED" to apply.  Use the "Full," "Half," and "Off" preset buttons.
*   **Feedback:** The "Last Servo Pos" and "Last LED Brightness" labels show the last values sent to and acknowledged by the Arduino.
*   **Log Window:**  The log window displays all commands sent to the Arduino and all responses received. This is useful for debugging and understanding the communication.
*   **Presets**: Are available for quick selection of servo angle and led brightness.

## Troubleshooting

*   **GUI Doesn't Start:**  Make sure you have Python 3 and `tkinter` installed.
*   **Serial Port Error:** Ensure you've selected the correct serial port. Try unplugging and replugging the Arduino. Check the output of `ls /dev/tty*` (on Linux) to see if the port changes.
*   **No Response from Arduino:** Verify the wiring. Check that the Arduino is powered. Make sure the baud rate in `gui.py` (9600) matches the baud rate in `FlatFieldPanel.ino`.  Use the Arduino IDE's Serial Monitor to test if the Arduino is receiving commands.
* **GUI Freezes**: There might be an issue with serial communication. The program is built in such a way that the GUI does not freeze. If it freezes, try restarting the arduino and/or reconnecting.

## Files

*   **`FlatFieldPanel.ino`:** The Arduino firmware.
*   **`gui.py`:** The Python GUI script.

## Contributing

Contributions are welcome!  Please submit pull requests or open issues on GitHub.

