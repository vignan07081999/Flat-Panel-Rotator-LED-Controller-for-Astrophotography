using System;
using System.IO.Ports;
using ASCOM.DeviceInterface;
using ASCOM.Utilities;
using System.Runtime.InteropServices;

[ComVisible(true)]
[Guid("YOUR-GUID-HERE")]
[ClassInterface(ClassInterfaceType.None)]
public class ASCOMFlatPanel : IFlatPanel
{
    private SerialPort serialPort;
    private int servoPosition = 0;
    private int ledBrightness = 0;
    private TraceLogger logger;

    public ASCOMFlatPanel()
    {
        logger = new TraceLogger("ASCOMFlatPanel", "Logs");
        logger.LogMessage("Constructor", "Initialized");
    }

    public void Connect()
    {
        serialPort = new SerialPort("COM3", 9600);
        serialPort.Open();
        logger.LogMessage("Connect", "Connected to Arduino");
    }

    public void Disconnect()
    {
        if (serialPort != null && serialPort.IsOpen)
        {
            serialPort.Close();
            logger.LogMessage("Disconnect", "Disconnected from Arduino");
        }
    }

    public bool Connected => serialPort != null && serialPort.IsOpen;

    public int Brightness
    {
        get { return ledBrightness; }
        set
        {
            ledBrightness = value;
            SendCommand($"LED {ledBrightness}");
            logger.LogMessage("Brightness", $"Set to {ledBrightness}");
        }
    }

    public int CoverState => servoPosition == 0 ? 0 : 1;

    public void OpenCover()
    {
        servoPosition = 180;
        SendCommand("SERVO 180");
        logger.LogMessage("OpenCover", "Panel opened");
    }

    public void CloseCover()
    {
        servoPosition = 0;
        SendCommand("SERVO 0");
        logger.LogMessage("CloseCover", "Panel closed");
    }

    private void SendCommand(string command)
    {
        if (Connected)
        {
            serialPort.WriteLine(command);
            string response = serialPort.ReadLine();
            logger.LogMessage("Arduino Response", response);
        }
    }
}
