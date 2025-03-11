import tkinter as tk
import tkinter.ttk as ttk
import serial
import serial.tools.list_ports
import threading
import time
from tkinter import messagebox  # Keep for other messages (e.g., connection errors)

class FlatFieldGUI:
    def __init__(self, master):
        self.master = master
        master.title("Flat Field Panel Control")

        # --- Serial Port Configuration ---
        self.port = None
        self.serialInst = serial.Serial()
        self.servo_current_pos = None  # Store current servo position (reliably)
        self.led_current_brightness = None # Store the led brightness


        # --- Serial Port Selection ---
        self.port_label = tk.Label(master, text="Select Serial Port:")
        self.port_label.grid(row=0, column=0, sticky=tk.W)

        self.port_combobox = ttk.Combobox(master, values=self.get_serial_ports())
        self.port_combobox.grid(row=0, column=1, sticky=tk.W + tk.E)
        self.port_combobox.bind("<<ComboboxSelected>>", self.connect_serial)

        self.connect_button = tk.Button(master, text="Connect", command=self.connect_serial)
        self.connect_button.grid(row=0, column=2)

        # --- Refresh Ports Button ---
        self.refresh_button = tk.Button(master, text="Refresh Ports", command=self.refresh_ports)
        self.refresh_button.grid(row=0, column=3)

        # --- Servo Control ---
        self.servo_label = tk.Label(master, text="Servo Position (0-180):")
        self.servo_label.grid(row=1, column=0, sticky=tk.W)

        self.servo_var = tk.IntVar()
        # Slider's position is set *only* from Arduino feedback.
        self.servo_scale = tk.Scale(master, from_=0, to=180, orient=tk.HORIZONTAL,
                                     variable=self.servo_var, state=tk.DISABLED)
        self.servo_scale.grid(row=1, column=1, sticky=tk.W + tk.E)

        self.servo_send_button = tk.Button(master, text="Send Servo",
                                           command=lambda: self.check_servo_limits(self.servo_var.get()))
        self.servo_send_button.grid(row=1, column=2)

        self.servo_feedback_label = tk.Label(master, text="Last Servo Pos: N/A")
        self.servo_feedback_label.grid(row=2, column=0, columnspan=3, sticky=tk.W)

        # --- Servo Presets ---
        self.servo_preset_frame = tk.Frame(master)
        self.servo_preset_frame.grid(row=3, column=0, columnspan=3, pady=5)

        self.servo_open_button = tk.Button(self.servo_preset_frame, text="Open", command=self.open_lid)
        self.servo_open_button.pack(side=tk.LEFT, padx=5)

        self.servo_close_button = tk.Button(self.servo_preset_frame, text="Close (180)",
                                           command=lambda: self.send_preset_command(3))
        self.servo_close_button.pack(side=tk.LEFT, padx=5)

        # --- LED Control ---
        self.led_label = tk.Label(master, text="LED Brightness (0-255):")
        self.led_label.grid(row=4, column=0, sticky=tk.W)

        self.led_var = tk.IntVar()
        self.led_scale = tk.Scale(master, from_=0, to=255, orient=tk.HORIZONTAL,
                                   variable=self.led_var, state=tk.DISABLED)
        self.led_scale.grid(row=4, column=1, sticky=tk.W + tk.E)

        self.led_send_button = tk.Button(master, text="Send LED", command=self.send_led_command_wrapper)
        self.led_send_button.grid(row=4, column=2)

        self.led_feedback_label = tk.Label(master, text="Last LED Brightness: N/A")
        self.led_feedback_label.grid(row=5, column=0, columnspan=3, sticky=tk.W)

        # --- LED Presets ---
        self.led_preset_frame = tk.Frame(master)
        self.led_preset_frame.grid(row=6, column=0, columnspan=3, pady=5)

        self.led_full_button = tk.Button(self.led_preset_frame, text="Full", command=lambda: self.send_led_command_wrapper(255))
        self.led_full_button.pack(side=tk.LEFT, padx=5)

        self.led_half_button = tk.Button(self.led_preset_frame, text="Half", command=lambda: self.send_led_command_wrapper(128))
        self.led_half_button.pack(side=tk.LEFT, padx=5)

        self.led_off_button = tk.Button(self.led_preset_frame, text="Off", command=lambda: self.send_led_command_wrapper(0))
        self.led_off_button.pack(side=tk.LEFT, padx=5)

        # --- Status Label ---
        self.status_label = tk.Label(master, text="Disconnected", fg="red")
        self.status_label.grid(row=7, column=0, columnspan=3)

        # --- Log Window ---
        self.log_label = tk.Label(master, text="Log:")
        self.log_label.grid(row=8, column=0, sticky=tk.W)

        self.log_text = tk.Text(master, height=10, width=50)
        self.log_text.grid(row=9, column=0, columnspan=4, sticky=tk.W + tk.E)
        self.log_text.config(state=tk.DISABLED)

        # --- Serial Read Thread ---
        self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
        self.receive_thread_running = False

        # Configure grid weights for resizing
        master.grid_columnconfigure(1, weight=1)
        master.grid_rowconfigure(9, weight=1)

    def check_servo_limits(self, value):
        """Checks servo limits, prompts, and sends command or reverts slider."""
        if 0 <= value <= 49:
            response = messagebox.askyesno(
                "Warning",
                "Are you sure you want to turn outside the limits?\nThis is for testing only."
            )
            if response:
                self.send_servo_command(value)
            else:
                # Revert slider *after* getting feedback.
                self.send_command("GET:SERVO")
        else:
            self.send_servo_command(value)

    def open_lid(self):
        """Opens the lid (servo to position 50) with LED off."""
        self.send_led_command(0)  # Turn LED off
        self.send_servo_command(50)  # Move servo (no prompt needed here)

    def get_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def refresh_ports(self):
        ports = self.get_serial_ports()
        self.port_combobox['values'] = ports
        if ports:
            self.port_combobox.set(ports[0])
        else:
            self.port_combobox.set("")
        self.log_message("Serial ports refreshed.")

    def connect_serial(self, event=None):
        if self.serialInst.is_open:
            self.serialInst.close()
            self.status_label.config(text="Disconnected", fg="red")
            self.connect_button.config(text="Connect")
            self.receive_thread_running = False
            self.servo_feedback_label.config(text="Last Servo Pos: N/A")
            self.led_feedback_label.config(text="Last LED Brightness: N/A")
            self.servo_scale.config(state=tk.DISABLED)
            self.led_scale.config(state=tk.DISABLED)
            return

        selected_port = self.port_combobox.get()
        if not selected_port:
            return

        try:
            self.serialInst = serial.Serial(port=selected_port, baudrate=9600, timeout=1, write_timeout=1)
            self.log_message(f"Connecting to {selected_port}...")
            self.serialInst.dtr = False  # Prevent DTR reset on some Arduinos
            time.sleep(0.1)
            self.serialInst.reset_input_buffer() # Clear buffer
            self.serialInst.dtr = True # Set the dtr
            time.sleep(2)  # Allow time for Arduino to initialize (important!)
            self.status_label.config(text=f"Connected to {selected_port}", fg="green")
            self.connect_button.config(text="Disconnect")
            self.receive_thread_running = True
            self.receive_thread.start()

            # Get initial values *after* starting the thread:
            self.send_command("GET:SERVO")
            self.send_command("GET:LED")
            self.servo_scale.config(state=tk.NORMAL)
            self.led_scale.config(state=tk.NORMAL)

        except serial.SerialException as e:
            self.log_message(f"Error: {e}")
            self.status_label.config(text=f"Error: {e}", fg="red")
            self.connect_button.config(text="Connect")

    def send_servo_command(self, value):
       # Turn off LED before *any* servo movement:
        if self.led_current_brightness !=0: #Only when led is on
            self.send_led_command(0)
            # Use after() so the LED command has time to be processed
            self.master.after(500, self._send_servo_after_led, value)  # 500ms delay
        else: #If led is already off
             self._send_servo_after_led(value)

    def _send_servo_after_led(self, value):
        # This function sends the *actual* servo command
        self.send_command(f"SERVO:{value}")

    def send_preset_command(self, value):
        # Turn off LED *before* any preset that moves the servo (always safe)
        self.send_led_command(0)
        # Schedule sending servo after a delay
        self.master.after(500, self._send_preset_after_led, value)

    def _send_preset_after_led(self,value):
        # Send the preset command after turning off the led
        self.send_command(f"PRESET:{value}")

    def send_led_command_wrapper(self, value=None):
        """Sends the LED command directly, without any prompts."""
        if value is None:
            value = self.led_var.get()
        self.send_led_command(value)

    def send_led_command(self, value):
        """Sends an LED control command to the Arduino."""
        self.send_command(f"LED:{value}")

    def send_command(self, command):
        if self.serialInst.is_open:
            try:
                self.serialInst.write((command + '\n').encode('utf-8'))
                self.log_message(f"Sent: {command}")
            except (serial.SerialException, OSError) as e:
                self.log_message(f"Error: {e}")
                self.status_label.config(text=f"Error: {e}", fg="red")
                self.connect_serial() # Attempt reconnection

    def receive_data(self):
        buffer = ""
        while self.receive_thread_running:
            if self.serialInst.is_open:
                try:
                    if self.serialInst.in_waiting > 0:
                        data = self.serialInst.read(self.serialInst.in_waiting).decode('utf-8', errors='ignore')
                        buffer += data

                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                            self.process_received_line(line.strip())

                except (serial.SerialException, OSError) as e:
                    self.log_message(f"Error: {e}")
                    self.status_label.config(text=f"Error: {e}", fg="red")
                    self.connect_serial()  # Attempt reconnection
                    return
                time.sleep(0.01) # Short delay

    def process_received_line(self, line):
        self.log_message(f"Received: {line}")

        if line.startswith("OK:"):
            parts = line[3:].split(":")
            if len(parts) >= 2:
                command = parts[0]
                value_str = parts[1]

                if command == "SERVO":
                    try:
                        value = int(value_str)
                        self.servo_var.set(value)  # Update slider IMMEDIATELY
                        self.servo_current_pos = value  # Update current position
                        self.servo_feedback_label.config(text=f"Last Servo Pos: {value}")

                    except ValueError:
                        self.log_message(f"Invalid servo value: {value_str}")

                elif command == "LED":
                    try:
                        value = int(value_str)
                        self.led_var.set(value)  # Update slider IMMEDIATELY
                        self.led_current_brightness = value  # Update current brightness
                        self.led_feedback_label.config(text=f"Last LED Brightness: {value}")
                    except ValueError:
                        self.log_message(f"Invalid LED value: {value_str}")

                elif command == "PRESET":
                    try:
                        preset_num = int(parts[1])
                        # Handle both OPEN_OFF and other presets consistently
                        if len(parts) > 3: # If preset details are in feedback
                            servo_val = int(parts[3])
                            led_val = int(parts[5])
                            self.servo_var.set(servo_val)
                            self.servo_current_pos = servo_val
                            self.led_var.set(led_val)
                            self.led_current_brightness = led_val
                            self.servo_feedback_label.config(text=f"Last Servo Pos: {servo_val}")
                            self.led_feedback_label.config(text=f"Last LED Brightness: {led_val}")

                    except (ValueError, IndexError) as e:
                        print(f"Error processing preset feedback: {e}, Line:{line}")

        elif line.startswith("ERR:"):
            error_message = line[4:]
            self.log_message(f"Arduino Error: {error_message}")
            self.status_label.config(text=f"Arduino Error: {error_message}", fg="orange")

    def log_message(self, message):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.config(state=tk.DISABLED)
        self.log_text.see(tk.END)

root = tk.Tk()
gui = FlatFieldGUI(root)
root.mainloop()
