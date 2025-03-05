import tkinter as tk
import tkinter.ttk as ttk
import serial
import serial.tools.list_ports
import threading
import time

class FlatFieldApp:
    def __init__(self, master):
        self.master = master
        master.title("Flat Field Controller (Alnitak Emulation)")

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

        # Brightness Control (0-255)
        self.brightness_label = tk.Label(master, text="Brightness (0-255):")
        self.brightness_label.grid(row=1, column=0, sticky=tk.W, padx=5, pady=5)

        self.brightness_slider = tk.Scale(master, from_=0, to=255, orient=tk.HORIZONTAL, command=self.update_brightness_display)
        self.brightness_slider.grid(row=1, column=1, padx=5, pady=5)

        self.brightness_value_label = tk.Label(master, text="0")
        self.brightness_value_label.grid(row=1, column=2, sticky=tk.W, padx=5, pady=5)

        self.brightness_button = tk.Button(master, text="Set Brightness", command=self.send_brightness_command)
        self.brightness_button.grid(row=1, column=3, padx=5, pady=5)

        # Cover Control (Open/Close/Halt)
        self.open_button = tk.Button(master, text="Open Cover", command=self.open_cover)
        self.open_button.grid(row=2, column=0, padx=5, pady=5)

        self.close_button = tk.Button(master, text="Close Cover", command=self.close_cover)
        self.close_button.grid(row=2, column=1, padx=5, pady=5)

        self.halt_button = tk.Button(master, text="Halt Cover", command=self.halt_cover)
        self.halt_button.grid(row=2, column=2, padx=5, pady=5)

        # Status Display
        self.status_label = tk.Label(master, text="Status:")
        self.status_label.grid(row=3, column=0, sticky=tk.W, padx=5, pady=5)

        self.status_text = tk.Text(master, height=2, width=30)
        self.status_text.grid(row=3, column=1, columnspan=3, padx=5, pady=5)
        self.status_text.config(state=tk.DISABLED)

        # Log Display
        self.log_label = tk.Label(master, text="Log:")
        self.log_label.grid(row=4, column=0, sticky=tk.W, padx=5, pady=5)

        self.log_text = tk.Text(master, height=5, width=50)
        self.log_text.grid(row=4, column=1, columnspan=3, padx=5, pady=5)
        self.log_text.config(state=tk.DISABLED)

        # --- Initialize ---
        self.refresh_ports()
        self.brightness_slider.set(0)  # Set initial brightness

        # Start the serial read thread
        self.read_thread = threading.Thread(target=self.read_serial, daemon=True)
        self.read_thread.start()


    def refresh_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.current(0)
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
            # Request initial status after connecting
            self.send_command("I\r")  #initialize
        except serial.SerialException as e:
            self.log_message(f"Error connecting to {selected_port}: {e}")
            self.serial_connected = False


    def send_brightness_command(self):
        if self.serial_connected:
            value = int(self.brightness_slider.get())
            command = f"B{value}\r"  # Note the carriage return!
            self.send_command(command)
            self.log_message(f"Sent brightness command: {command.strip()}")
        else:
            self.log_message("Not connected to serial port.")


    def open_cover(self):
        if self.serial_connected:
            self.send_command("O\r")  # Note the carriage return!
            self.log_message("Sent open cover command")
        else:
            self.log_message("Not connected to serial port.")

    def close_cover(self):
        if self.serial_connected:
            self.send_command("C\r")  # Note the carriage return!
            self.log_message("Sent close cover command")
        else:
            self.log_message("Not connected to serial port.")

    def halt_cover(self):
        if self.serial_connected:
            self.send_command("H\r")  # Note the carriage return!
            self.log_message("Sent halt cover command")
        else:
            self.log_message("Not connected to serial port.")

    def send_command(self, command):
         if self.serial_connected:
            try:
                self.serial_port.write(command.encode())
            except Exception as e:
                self.log_message("Failed to send command. Error: "+str(e))
         else:
            self.log_message("Not connected to serial port.")

    def update_brightness_display(self, value):
        self.brightness_value_label.config(text=str(int(float(value))))

    def read_serial(self):
        buffer = ""
        while True:
            if self.serial_connected:
                try:
                    data = self.serial_port.read(self.serial_port.in_waiting or 1).decode('ascii', errors='ignore')
                    if data:
                        buffer += data
                        if '\r' in buffer:
                            line, buffer = buffer.split('\r', 1)  # Split at the FIRST carriage return
                            self.process_response(line)


                except Exception as e:
                    self.log_message(f"Error reading serial data: {e}")
                    self.serial_connected = False  # Disconnect on error.
                    self.serial_port.close()

            time.sleep(0.01)

    def process_response(self, response):
        self.log_message(f"Received: {response}")  # Log the raw response
        if "," in response:
            try:
                brightness_str, position_str = response.split(",")
                brightness = int(brightness_str)
                position = int(position_str)
                self.update_status_display(brightness, position)
            except ValueError:
                self.log_message(f"Error parsing response: {response}")
        elif response =="x":
            self.log_message("Invalid Command sent")

    def update_status_display(self, brightness, position):
        self.status_text.config(state=tk.NORMAL)
        self.status_text.delete(1.0, tk.END)
        self.status_text.insert(tk.END, f"Brightness: {brightness}\nPosition: {position}")
        self.status_text.config(state=tk.DISABLED)

    def log_message(self, message):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.config(state=tk.DISABLED)
        self.log_text.see(tk.END)

root = tk.Tk()
app = FlatFieldApp(root)
root.mainloop()
