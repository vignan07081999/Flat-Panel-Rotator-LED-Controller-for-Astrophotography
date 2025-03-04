using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;
using System.IO.Ports; // For SerialPort

using ASCOM;
using ASCOM.Astrometry;
using ASCOM.Astrometry.AstroUtils;
using ASCOM.Utilities;
using ASCOM.DeviceInterface;

namespace ASCOM.FlatField
{
    [Guid("your-unique-guid-here")] // REPLACE WITH A UNIQUE GUID!
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    public class FlatField : IFlatFieldV2
    {
        // --- Constants ---
        private const string DRIVER_ID = "ASCOM.FlatField.FlatField"; // Consistent driver ID
        private const string DRIVER_NAME = "ASCOM FlatField Driver";
        private const string DRIVER_DESCRIPTION = "ASCOM Driver for Arduino Flat Field Panel";

        // --- Serial Port Settings ---
        private string comPort = "COM1"; // Default COM port - will be configurable
        private const int baudRate = 115200;

        // --- Device State ---
        private bool connected = false;
        private SerialPort serialPort;
        private int maxBrightness = 255; // Maximum brightness value from Arduino code
        private int currentBrightness = 0; // Store current LED brightness
        private int currentServoPosition = 90; // Store current Servo Position

        // --- Utility & Trace Logger ---
        private Util utilities;
        private TraceLogger traceLogger;

        // --- Constructor ---
        public FlatField()
        {
            traceLogger = new TraceLogger("", "FlatField"); // Create TraceLogger
            traceLogger.Enabled = true; // Enable logging (can be controlled via ASCOM Profile)
            traceLogger.LogMessage("FlatField", "Starting initialization");

            utilities = new Util(); // ASCOM utilities

            // Initialize serial port (but don't connect yet)
            serialPort = new SerialPort();
            serialPort.BaudRate = baudRate;
            serialPort.DataBits = 8;
            serialPort.Parity = Parity.None;
            serialPort.StopBits = StopBits.One;
            serialPort.Handshake = Handshake.None;
            serialPort.ReadTimeout = 200; // in milliseconds
            serialPort.WriteTimeout = 200;
             serialPort.NewLine = "\n"; // Set the newline character
            serialPort.DataReceived += SerialPort_DataReceived; // Add data received event handler

            traceLogger.LogMessage("FlatField", "Initialization complete");
        }

        // --- ASCOM Driver Methods ---

        public string Action(string actionName, string actionParameters)
        {
           LogMessage("Action", $"Action {actionName} is not implemented by this driver");
           throw new ASCOM.ActionNotImplementedException($"Action {actionName} is not implemented by this driver");
        }

        public void CommandBlind(string command, bool raw)
        {
            CheckConnected("CommandBlind");
            SendCommand(command);
        }

        public bool CommandBool(string command, bool raw)
        {
            CheckConnected("CommandBool");
            throw new ASCOM.MethodNotImplementedException("CommandBool is not implemented");
        }

        public string CommandString(string command, bool raw)
        {
            CheckConnected("CommandString");
             throw new ASCOM.MethodNotImplementedException("CommandString is not implemented");
        }

        public void Dispose()
        {
            // Clean up resources
            traceLogger.Enabled = false;
            traceLogger.Dispose();
            traceLogger = null;
            utilities.Dispose();
            utilities = null;
            if (serialPort != null)
            {
                if (serialPort.IsOpen)
                {
                    serialPort.Close();
                }
                serialPort.Dispose();
                serialPort = null;
            }
        }
        public bool Connected
        {
            get
            {
                LogMessage("Connected Get", connected.ToString());
                return connected;
            }
            set
            {
                traceLogger.LogMessage("Connected Set", value.ToString());
                if (value == connected)
                    return;

                if (value) // Connect
                {
                    try
                    {
                        serialPort.PortName = comPort;  // Set the COM port (from profile)
                        serialPort.Open();
                        connected = true;
                        LogMessage("Connected Set", "Connected to " + comPort);
                         // Request initial feedback
                        SendCommand("F");

                    }
                    catch (Exception ex)
                    {
                        LogMessage("Connected Set - Error", "Failed to connect: " + ex.Message);
                        connected = false;
                        throw new ASCOM.NotConnectedException("Failed to connect", ex); // More specific exception
                    }
                }
                else // Disconnect
                {
                    if (serialPort.IsOpen)
                    {
                        serialPort.Close();
                    }
                    connected = false;
                    LogMessage("Connected Set", "Disconnected");
                }
            }
        }

        public string Description
        {
            get
            {
                traceLogger.LogMessage("Description Get", DRIVER_DESCRIPTION);
                return DRIVER_DESCRIPTION;
            }
        }

        public string DriverInfo
        {
            get
            {
                // Include version information
                Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
                string driverInfo = $"{DRIVER_DESCRIPTION} Version: {version.Major}.{version.Minor}";
                traceLogger.LogMessage("DriverInfo Get", driverInfo);
                return driverInfo;
            }
        }

        public string DriverVersion
        {
            get
            {
                Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
                string driverVersion = $"{version.Major}.{version.Minor}";
                traceLogger.LogMessage("DriverVersion Get", driverVersion);
                return driverVersion;
            }
        }

        public short InterfaceVersion
        {
            get
            {
                LogMessage("InterfaceVersion Get", "2");
                return 2; // IFlatFieldV2
            }
        }

        public string Name
        {
            get
            {
                traceLogger.LogMessage("Name Get", DRIVER_NAME);
                return DRIVER_NAME;
            }
        }

        public void SetupDialog()
        {
           // Consider implementing your own setup dialog
            using (SetupDialogForm F = new SetupDialogForm(this))
            {
                var result = F.ShowDialog();
                if (result == System.Windows.Forms.DialogResult.OK)
                {
                    //SaveComPort(); // Persist COM port changes
                }
            }
        }

        public ArrayList SupportedActions
        {
            get
            {
                traceLogger.LogMessage("SupportedActions Get", "Returning empty arraylist");
                return new ArrayList(); // We don't support any custom actions
            }
        }


        // --- IFlatFieldV2 Members ---
        public int Brightness
        {
             get
            {
                CheckConnected("Brightness");
                return currentBrightness;
            }
        }

		public int MaxBrightness
		{
			get
            {
                traceLogger.LogMessage("MaxBrightness Get", maxBrightness.ToString());
                return maxBrightness;
            }
		}
        public void StartCoverCalibration(int Brightness)
        {
            throw new MethodNotImplementedException("StartCoverCalibration");
        }

        public CoverStatus CoverStatus
		{
			get
            {
                CheckConnected("CoverStatus");
                // Map servo position to CoverStatus.  This is an EXAMPLE, adjust as needed.
                if (currentServoPosition == 0)
                {
                    traceLogger.LogMessage("CoverStatus Get", "CoverStatus: Open");
                    return CoverStatus.CoverOpen;
                }
                else if (currentServoPosition == 180)
                {
                    traceLogger.LogMessage("CoverStatus Get", "CoverStatus: Closed");
                    return CoverStatus.CoverClosed;
                }
                else
                {
                    traceLogger.LogMessage("CoverStatus Get", "CoverStatus: Moving");
                    return CoverStatus.CoverMoving; // Or Unknown, if appropriate
                }
            }
		}

		public void CalibratorOn(int Brightness)
		{
            CheckConnected("CalibratorOn");
            if (Brightness < 0 || Brightness > maxBrightness)
            {
                throw new InvalidValueException($"Brightness must be between 0 and {maxBrightness}");
            }
            currentBrightness = Brightness;
             SendCommand("L" + Brightness);

            traceLogger.LogMessage("CalibratorOn", $"LED brightness set to {Brightness}");
		}

		public void CalibratorOff()
		{
			 CheckConnected("CalibratorOff");
             SendCommand("L0");
             currentBrightness = 0;
             traceLogger.LogMessage("CalibratorOff", "LEDs turned off");
		}

		public void CloseCover()
		{
            CheckConnected("CloseCover");
             SendCommand("S180"); // Example: 180 degrees for closed
             currentServoPosition = 180;
             traceLogger.LogMessage("CloseCover", "Cover closing...");
		}

		public void HaltCover()
		{
             CheckConnected("HaltCover");
            // There's no specific "halt" command for our simple Arduino setup.
            // The best we can do is probably to re-send the *current* position,
            // which should stop any ongoing movement (if the Arduino code is well-behaved).
             SendCommand("S" + currentServoPosition);
              traceLogger.LogMessage("HaltCover", "Cover halt attempted.");
		}

		public void OpenCover()
		{
			 CheckConnected("OpenCover");
             SendCommand("S0");  // Example: 0 degrees for open
             currentServoPosition = 0;
             traceLogger.LogMessage("OpenCover", "Cover opening...");
		}

        // --- Helper Methods ---

        private void SendCommand(string command)
        {
            if (serialPort.IsOpen)
            {
                try
                {
                    traceLogger.LogMessage("SendCommand", $"Sending: {command}");
                    serialPort.Write(command + "\n");
                }
                catch (Exception ex)
                {
                    traceLogger.LogMessage("SendCommand - Error", ex.Message);
                      throw new ASCOM.DriverException("Error sending command: " + ex.Message, ex); // More general exception
                }
            }
            else
            {
                traceLogger.LogMessage("SendCommand", "Serial port not open!");
                throw new ASCOM.NotConnectedException("Serial port not open");
            }
        }
        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                string data = serialPort.ReadExisting();  // Read all available data
                traceLogger.LogMessage("DataReceived", $"Received: {data}");

                // Process the received data (look for feedback)
                string[] lines = data.Split(new string[] { "\r\n", "\n" }, StringSplitOptions.RemoveEmptyEntries);
                 foreach (string line in lines)
                {
                    if (line.StartsWith("SP:") && line.Contains("LB:"))
                    {
                        // Parse feedback: "SP:90,LB:128"
                        string[] parts = line.Split(',');
                        if (parts.Length == 2)
                        {
                            string servoPart = parts[0].Substring(3); // Remove "SP:"
                            string ledPart = parts[1].Substring(3);   // Remove "LB:"

                            if (int.TryParse(servoPart, out int servoPos) && int.TryParse(ledPart, out int ledBrightness))
                            {
                                currentServoPosition = servoPos;
                                currentBrightness = ledBrightness;
                                 traceLogger.LogMessage("DataReceived", $"Parsed Feedback - Servo: {currentServoPosition}, LED: {currentBrightness}");
                            }
                        }
                    }
                    else
                    {
                         traceLogger.LogMessage("DataReceived-Other", $"Arduino:{line}");
                    }
                }

            }
            catch (Exception ex)
            {
                traceLogger.LogMessage("DataReceived - Error", ex.Message);
                // Don't throw exceptions from event handlers; log instead.
            }
        }

        private void CheckConnected(string methodName)
        {
            if (!connected)
            {
                throw new ASCOM.NotConnectedException(methodName + " requires a connection.");
            }
        }
        private void LogMessage(string identifier, string message)
        {
            if (traceLogger != null)
            {
                traceLogger.LogMessage(identifier, message);
            }
        }

         // --- Profile Handling (COM Port, etc.) ---
          internal string ComPort  // Made internal so SetupDialogForm can access it
        {
            get { return comPort; }
            set { comPort = value; }
        }

        internal void LoadProfile()
        {
            using (Profile profile = new Profile())
            {
                profile.DeviceType = "FlatField";
                comPort = profile.GetValue(DRIVER_ID, "ComPort", string.Empty, "COM1"); //default port is COM1
                traceLogger.Enabled = bool.Parse(profile.GetValue(DRIVER_ID, "Trace Level", string.Empty, "true")); //logging is on by default
            }
           
        }

        internal void SaveProfile()
        {
            using (Profile profile = new Profile())
            {
                profile.DeviceType = "FlatField";
                profile.WriteValue(DRIVER_ID, "ComPort", comPort);
                profile.WriteValue(DRIVER_ID, "Trace Level", traceLogger.Enabled.ToString());
            }
        }

    }

    // --- Helper Classes (Driver ID, etc.) ---

    [Guid("your-unique-guid-here")] // REPLACE WITH A UNIQUE GUID! -  Must be a different GUID than the Class
    [ComVisible(true)]
    public class DriverID
    {
        public string Value { get { return FlatField.DRIVER_ID; } }  // Use the constant from the main class
    }
}
