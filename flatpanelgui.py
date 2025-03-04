import tkinter as tk
import tkinter.ttk as ttk
import serial
import serial.tools.list_ports
import threading
import time

class FlatFieldApp:
    def __init__(self, master):
        self.master = master
        master.title("Flat Field Controller")

        self.serial_port = None
        self.serial_connected = False

        # --- GUI Elements ---

        # Port Selection
        self.port_label = tk.Label(master, text="Select Port:")
        self.port_label.grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)

        self.port_combo = ttk.Combobox(master, values=[], state="readonly")
        self.port_combo.grid(row=0, column=1, padx=5, pady=5)
        self.port_combo.bind("<<ComboboxSelected>>", self.connect_serial)

        self.refresh_button = tk.Button(master, text="Refresh Ports", command=self.refresh_ports)
        self.refresh_button.grid(row=0, column=2, padx=5, pady=5)

        # Servo Control
        self.servo_label = tk.Label(master, text="Servo Position (0-180):")
        self.servo_label.grid(row=1, column=0, sticky=tk.W, padx=5, pady=5)

        self.servo_slider = tk.Scale(master, from_=0, to=180, orient=tk.HORIZONTAL, command=self.update_servo_display)
        self.servo_slider.grid(row=1, column=1, padx=5, pady=5)

        self.servo_value_label = tk.Label(master, text="90")  # Initial value
        self.servo_value_label.grid(row=1, column=2, sticky=tk.W, padx=5, pady=5)

        self.servo_button = tk.Button(master, text="Set Servo", command=self.send_servo_command)
        self.servo_button.grid(row=1, column=3, padx=5, pady=5)

        # LED Brightness Control
        self.led_label = tk.Label(master, text="LED Brightness (0-255):")
        self.led_label.grid(row=2, column=0, sticky=tk.W, padx=5, pady=5)

        self.led_slider = tk.Scale(master, from_=0, to=255, orient=tk.HORIZONTAL, command=self.update_led_display)
        self.led_slider.grid(row=2, column=1, padx=5, pady=5)

        self.led_value_label = tk.Label(master, text="0")  # Initial value
        self.led_value_label.grid(row=2, column=2, sticky=tk.W, padx=5, pady=5)

        self.led_button = tk.Button(master, text="Set LED", command=self.send_led_command)
        self.led_button.grid(row=2, column=3, padx=5, pady=5)

        # Feedback Display
        self.feedback_label = tk.Label(master, text="Feedback:")
        self.feedback_label.grid(row=3, column=0, sticky=tk.W, padx=5, pady=5)

        self.feedback_text = tk.Text(master, height=2, width=30)
        self.feedback_text.grid(row=3, column=1, columnspan=3, padx=5, pady=5)
        self.feedback_text.config(state=tk.DISABLED)  # Make read-only

        # Log Display
        self.log_label = tk.Label(master, text="Log:")
        self.log_label.grid(row=4, column=0, sticky=tk.W, padx=5, pady=5)

        self.log_text = tk.Text(master, height=5, width=50)
        self.log_text.grid(row=4, column=1, columnspan=3, padx=5, pady=5)
        self.log_text.config(state=tk.DISABLED)

        # --- Initialize ---
        self.refresh_ports()
        self.servo_slider.set(90)  # Set initial servo position
        self.led_slider.set(0)    # Set initial LED brightness

        # Start the serial read thread
        self.read_thread = threading.Thread(target=self.read_serial, daemon=True)
        self.read_thread.start()


    def refresh_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.current(0)  # Select the first port by default
            self.connect_serial() #attempt autoconnect
        else:
            self.log_message("No serial ports found.")

    def connect_serial(self, event=None):
            selected_port = self.port_combo.get()
            if self.serial_connected:
                self.serial_port.close()
                self.serial_connected = False

            try:
                self.serial_port = serial.Serial(selected_port, 115200, timeout=1)
                self.serial_connected = True
                self.log_message(f"Connected to {selected_port}")
                # Request initial feedback after connecting
                self.send_feedback_command()
            except serial.SerialException as e:
                self.log_message(f"Error connecting to {selected_port}: {e}")
                self.serial_connected = False

    def send_servo_command(self):
        if self.serial_connected:
            value = int(self.servo_slider.get())
            command = f"S{value}\n"
            self.serial_port.write(command.encode())
            self.log_message(f"Sent servo command: {command.strip()}")
        else:
            self.log_message("Not connected to serial port.")

    def send_led_command(self):
        if self.serial_connected:
            value = int(self.led_slider.get())
            command = f"L{value}\n"
            self.serial_port.write(command.encode())
            self.log_message(f"Sent LED command: {command.strip()}")
        else:
            self.log_message("Not connected to serial port.")

    def send_feedback_command(self):
        if self.serial_connected:
            command = "F\n"
            self.serial_port.write(command.encode())
            self.log_message("Sent feedback request")
        else:
            self.log_message("Not connected to serial port.")

    def update_servo_display(self, value):
        self.servo_value_label.config(text=str(int(float(value))))

    def update_led_display(self, value):
        self.led_value_label.config(text=str(int(float(value))))


    def read_serial(self):
        while True:
            if self.serial_connected and self.serial_port.in_waiting > 0:
                try:
                    line = self.serial_port.readline().decode('utf-8').strip()
                    if line.startswith("SP:") and "LB:" in line:
                        # Parse feedback: "SP:90,LB:128"
                        parts = line.split(',')
                        servo_part = parts[0].split(':')
                        led_part = parts[1].split(':')
                        if len(servo_part) == 2 and len(led_part) == 2:
                          servo_pos = int(servo_part[1])
                          led_brightness = int(led_part[1])
                          self.update_feedback_display(servo_pos, led_brightness)
                    elif line == "System Idle":
                        #self.log_message("Arduino: System Idle") #removed because of feedback spam
                        pass
                    elif line == "Invalid Command" or line == "Invalid Brightness Value" or line == "Invalid Servo Position":
                        self.log_message(f"Arduino: {line}")
                    else:
                        self.log_message(f"Arduino: {line}")

                except Exception as e:
                    self.log_message(f"Error reading serial data: {e}")
            time.sleep(0.01)  # Short delay to prevent busy-waiting


    def update_feedback_display(self, servo_pos, led_brightness):
        self.feedback_text.config(state=tk.NORMAL)  # Enable editing
        self.feedback_text.delete(1.0, tk.END)
        self.feedback_text.insert(tk.END, f"Servo: {servo_pos}\nLED: {led_brightness}")
        self.feedback_text.config(state=tk.DISABLED)  # Make read-only


    def log_message(self, message):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.config(state=tk.DISABLED)
        self.log_text.see(tk.END)  # Scroll to the end

root = tk.Tk()
app = FlatFieldApp(root)
root.mainloop()
