import tkinter as tk
import tkinter.ttk as ttk
import serial
import serial.tools.list_ports
import threading
import time

class FlatFieldGUI:
    def __init__(self, master):
        self.master = master
        master.title("Flat Field Panel Control")

        # --- Serial Port Configuration ---
        self.port = None
        self.serialInst = serial.Serial()

        # --- Serial Port Selection ---
        self.port_label = tk.Label(master, text="Select Serial Port:")
        self.port_label.grid(row=0, column=0, sticky=tk.W)

        self.port_combobox = ttk.Combobox(master, values=self.get_serial_ports())
        self.port_combobox.grid(row=0, column=1, sticky=tk.W+tk.E)
        self.port_combobox.bind("<<ComboboxSelected>>", self.connect_serial)

        self.connect_button = tk.Button(master, text="Connect", command=self.connect_serial)
        self.connect_button.grid(row=0, column=2)

        # --- Servo Control ---
        self.servo_label = tk.Label(master, text="Servo Position (0-180):")
        self.servo_label.grid(row=1, column=0, sticky=tk.W)

        self.servo_var = tk.IntVar()
        self.servo_scale = tk.Scale(master, from_=0, to=180, orient=tk.HORIZONTAL,
                                     variable=self.servo_var)  # Removed command here
        self.servo_scale.grid(row=1, column=1, sticky=tk.W+tk.E)

        self.servo_send_button = tk.Button(master, text="Send Servo", command=lambda: self.send_servo_command(self.servo_var.get()))
        self.servo_send_button.grid(row=1, column=2)

        self.servo_feedback_label = tk.Label(master, text="Last Servo Pos: N/A")
        self.servo_feedback_label.grid(row=2, column=0, columnspan=3, sticky=tk.W)


        # --- Servo Presets ---
        self.servo_preset_frame = tk.Frame(master)
        self.servo_preset_frame.grid(row=3, column=0, columnspan=3, pady=5)

        self.servo_open_button = tk.Button(self.servo_preset_frame, text="Open (0)", command=lambda: self.send_servo_command(0))
        self.servo_open_button.pack(side=tk.LEFT, padx=5)

        self.servo_close_button = tk.Button(self.servo_preset_frame, text="Close (180)", command=lambda: self.send_servo_command(180))
        self.servo_close_button.pack(side=tk.LEFT, padx=5)


        # --- LED Control ---
        self.led_label = tk.Label(master, text="LED Brightness (0-255):")
        self.led_label.grid(row=4, column=0, sticky=tk.W)

        self.led_var = tk.IntVar()
        self.led_scale = tk.Scale(master, from_=0, to=255, orient=tk.HORIZONTAL,
                                   variable=self.led_var) # Removed command
        self.led_scale.grid(row=4, column=1, sticky=tk.W+tk.E)

        self.led_send_button = tk.Button(master, text="Send LED", command=lambda: self.send_led_command(self.led_var.get()))
        self.led_send_button.grid(row=4, column=2)

        self.led_feedback_label = tk.Label(master, text="Last LED Brightness: N/A")
        self.led_feedback_label.grid(row=5, column=0, columnspan=3, sticky=tk.W)


        # --- LED Presets ---
        self.led_preset_frame = tk.Frame(master)
        self.led_preset_frame.grid(row=6, column=0, columnspan=3, pady=5)

        self.led_full_button = tk.Button(self.led_preset_frame, text="Full", command=lambda: self.send_led_command(255))
        self.led_full_button.pack(side=tk.LEFT, padx=5)

        self.led_half_button = tk.Button(self.led_preset_frame, text="Half", command=lambda: self.send_led_command(128))
        self.led_half_button.pack(side=tk.LEFT, padx=5)

        self.led_off_button = tk.Button(self.led_preset_frame, text="Off", command=lambda: self.send_led_command(0))
        self.led_off_button.pack(side=tk.LEFT, padx=5)


        # --- Status Label ---
        self.status_label = tk.Label(master, text="Disconnected", fg="red")
        self.status_label.grid(row=7, column=0, columnspan=3)

        # --- Log Window ---
        self.log_label = tk.Label(master, text="Log:")
        self.log_label.grid(row=8, column=0, sticky=tk.W)

        self.log_text = tk.Text(master, height=10, width=50)
        self.log_text.grid(row=9, column=0, columnspan=3, sticky=tk.W+tk.E)
        self.log_text.config(state=tk.DISABLED)  # Make it read-only

        # --- Serial Read Thread ---
        self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
        self.receive_thread_running = False

        # Configure grid weights for resizing
        master.grid_columnconfigure(1, weight=1)
        master.grid_rowconfigure(9, weight=1)



    def get_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def connect_serial(self, event=None):
        if self.serialInst.is_open:
            self.serialInst.close()
            self.status_label.config(text="Disconnected", fg="red")
            self.connect_button.config(text="Connect")
            self.receive_thread_running = False
            return

        selected_port = self.port_combobox.get()
        if not selected_port:
            return

        try:
            self.serialInst = serial.Serial(port=selected_port, baudrate=9600, timeout=1)
            self.log_message(f"Connecting to {selected_port}...")
            self.status_label.config(text=f"Connected to {selected_port}", fg="green")
            self.connect_button.config(text="Disconnect")
            self.receive_thread_running = True
            self.receive_thread.start()
            # Get initial values on connect
            self.send_command("GET:SERVO")
            self.send_command("GET:LED")


        except serial.SerialException as e:
            self.log_message(f"Error: {e}")
            self.status_label.config(text=f"Error: {e}", fg="red")
            self.connect_button.config(text="Connect")

    def send_servo_command(self, value):
        self.send_command(f"SERVO:{value}")

    def send_led_command(self, value):
        self.send_command(f"LED:{value}")

    def send_command(self, command):
        if self.serialInst.is_open:
            try:
                self.serialInst.write((command + '\n').encode('utf-8'))
                self.log_message(f"Sent: {command}") # Log sent command
            except serial.SerialException as e:
                self.log_message(f"Serial Error: {e}")
                self.status_label.config(text=f"Serial Error: {e}", fg="red")
                self.connect_serial() # Attempt Reconnect

    def receive_data(self):
        buffer = ""
        while self.receive_thread_running:
            if self.serialInst.is_open:
                try:
                    if self.serialInst.in_waiting > 0:
                        data = self.serialInst.read(self.serialInst.in_waiting).decode('utf-8', errors='ignore') # Read all available bytes
                        buffer += data

                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)  # Split into lines
                            self.process_received_line(line.strip())

                except serial.SerialException as e:
                    self.log_message(f"Serial Error: {e}")
                    self.status_label.config(text=f"Serial Error: {e}", fg="red")
                    self.connect_serial()  # Attempt to reconnect
                    return # Exit the thread
                except OSError as e:
                    self.log_message(f"OS Error: {e}")
                    self.receive_thread_running = False
                    return
                time.sleep(0.01)


    def process_received_line(self, line):
        self.log_message(f"Received: {line}")  # Log received data

        if line.startswith("OK:"):
            parts = line[3:].split(":")
            if len(parts) >= 2:
                command = parts[0]
                value_str = parts[1]

                if command == "SERVO":
                    try:
                        value = int(value_str)
                        self.servo_var.set(value)
                        self.servo_feedback_label.config(text=f"Last Servo Pos: {value}")
                    except ValueError:
                        self.log_message(f"Invalid servo value received: {value_str}")

                elif command == "LED":
                    try:
                        value = int(value_str)
                        self.led_var.set(value)
                        self.led_feedback_label.config(text=f"Last LED Brightness: {value}")
                    except ValueError:
                         self.log_message(f"Invalid LED value received: {value_str}")

        elif line.startswith("ERR:"):
            error_message = line[4:]
            self.log_message(f"Arduino Error: {error_message}")
            self.status_label.config(text=f"Arduino Error: {error_message}", fg="orange")

    def log_message(self, message):
        self.log_text.config(state=tk.NORMAL)  # Temporarily enable editing
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.config(state=tk.DISABLED)  # Disable editing again
        self.log_text.see(tk.END) #Autoscroll



root = tk.Tk()
gui = FlatFieldGUI(root)
root.mainloop()
