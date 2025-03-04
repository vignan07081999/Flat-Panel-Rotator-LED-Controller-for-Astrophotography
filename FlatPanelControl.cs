using System;
using System.IO.Ports;
using System.Windows.Forms;

namespace FlatPanelControl
{
    public partial class MainForm : Form
    {
        private SerialPort serialPort;

        public MainForm()
        {
            InitializeComponent();
            serialPort = new SerialPort("COM3", 9600);
            try 
            { 
                serialPort.Open();
                serialPort.DataReceived += SerialPort_DataReceived;
            } 
            catch (Exception ex) 
            { 
                MessageBox.Show("Failed to open COM port: " + ex.Message); 
            }
        }

        private void btnSetBrightness_Click(object sender, EventArgs e)
        {
            int brightness = (int)numBrightness.Value;
            SendCommand($"LED {brightness}");
        }

        private void btnMoveServo_Click(object sender, EventArgs e)
        {
            int position = (int)numServoPosition.Value;
            SendCommand($"SERVO {position}");
        }

        private void SendCommand(string command)
        {
            if (serialPort.IsOpen)
            {
                try
                {
                    serialPort.WriteLine(command);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error sending command: " + ex.Message);
                }
            }
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                string feedback = serialPort.ReadLine();
                this.Invoke(new Action(() => txtFeedback.AppendText(feedback + "\n")));
            }
            catch (Exception ex)
            {
                this.Invoke(new Action(() => txtFeedback.AppendText("Error reading data: " + ex.Message + "\n")));
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (serialPort.IsOpen) serialPort.Close();
        }
    }
}
