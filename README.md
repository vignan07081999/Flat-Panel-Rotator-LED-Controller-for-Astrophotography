# Flat-Panel-Rotator-LED-Controller-for-Astrophotography
This project is a custom rotating flat field panel designed for astrophotography. It can be used for taking flat frames and as a scope cover when not in use.

## Project Overview
This project is a **custom rotating flat field panel** designed for astrophotography. It can be used for taking flat frames and as a scope cover when not in use. The system consists of:
- **Arduino-controlled hardware** (servo motor, LED panel, EEPROM storage)
- **INDI driver** (for Linux-based astronomy software like EKOS/KStars on StellarMate)
- **ASCOM driver** (for Windows-based astronomy software)
- **Windows GUI Application** (for manual control on Windows)

## Features
✅ **Servo-controlled rotating panel** (acts as a cover & flat field source)  
✅ **Dimmable LED panel** (adjust brightness for flat calibration)  
✅ **Real-time feedback** (reads actual servo position & LED brightness)  
✅ **Persistent memory storage** (recovers last settings after power loss)  
✅ **Support for INDI & ASCOM drivers**  
✅ **Standalone Windows GUI app for manual control**  

---
## Hardware Requirements
- **Arduino Uno** (or compatible board)
- **Servo motor** (capable of rotating the panel, e.g., MG995)
- **Dimmable LED strips** (PWM controlled)
- **3D-printed panel enclosure** (to mount LED and servos)
- **External power source** (if needed for LEDs/servos)

### Circuit Diagram
The circuit includes connections for:
- Servo motor controlled via **Arduino PWM pin**
- LED brightness controlled via **PWM pin**
- EEPROM used for storing last position/brightness

---
## Software Components
### **1. Arduino Firmware** (`Arduino/FlatPanelController.ino`)
- Written in **C++**
- Controls servos and LED brightness via serial commands
- Saves last known state in EEPROM for recovery

### **2. INDI Driver** (`INDI/IndiFlatPanel.cpp`)
- Written in **C++**
- Compatible with **Linux, StellarMate, EKOS/KStars**
- Provides GUI controls in KStars/EKOS for servo & LED control

### **3. ASCOM Driver** (`ASCOM/ASCOMFlatPanel.cs`)
- Written in **C#**
- Compatible with **Windows-based astronomy software**
- Provides panel open/close and LED brightness control

### **4. Windows GUI Application** (`WindowsApp/FlatPanelControl.cs`)
- Written in **C#**
- Standalone application to control the panel on Windows

---
## Installation & Usage
### **1. Arduino Firmware**
1. Install **Arduino IDE**
2. Connect the Arduino via USB
3. Open `FlatPanelController.ino`, select the correct board & port
4. Upload the code

### **2. INDI Driver (Linux/StellarMate)**
1. Open a terminal & run:
   ```bash
   git clone https://github.com/yourusername/FlatPanelController.git
   cd FlatPanelController/INDI
   mkdir build && cd build
   cmake ..
   make
   sudo make install
   ```
2. Restart **INDI server**:
   ```bash
   indiserver -v indi_flatpanel
   ```
3. In **EKOS/KStars**, add a new device and select `Flat Panel Controller`

### **3. ASCOM Driver (Windows)**
1. Install **ASCOM Platform**
2. Clone the repo & build:
   ```powershell
   git clone https://github.com/yourusername/FlatPanelController.git
   cd FlatPanelController/ASCOM
   ```
3. Open the `.csproj` file in **Visual Studio**, build & register the driver
4. Select `Flat Panel Controller` in your ASCOM-compatible software

### **4. Windows GUI Application**
1. Clone the repo:
   ```powershell
   git clone https://github.com/yourusername/FlatPanelController.git
   ```
2. Open `FlatPanelControl.csproj` in **Visual Studio**
3. Build the project & run `FlatPanelControl.exe`

---
## Future Improvements
🔹 Add WiFi/Bluetooth support for wireless control  
🔹 Integrate into NINA & other astrophotography software  
🔹 Support more flat panel hardware types  

### **License & Contributions**
This project is **open-source**! Feel free to contribute or suggest improvements.

🚀 **Happy Imaging!**


