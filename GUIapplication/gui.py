import tkinter as tk
import tkinter.ttk as ttk
import serial
import serial.tools.list_ports
import threading
import time
from tkinter import messagebox

class FlatFieldGUI:
    def __init__(self, master):
        self.master = master
        master.title("Flat Field Panel Control")

        # --- Serial Port Configuration ---
        self.port = None
        self.serialInst = serial.Serial()
        self.servo_current_pos = None
        self.led_current_brightness = None

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
        self.servo_scale = tk.Scale(master, from_=0, to=180, orient=tk.HORIZONTAL,
                                     variable=self.servo_var)
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
                                   variable=self.led_var)
        self.led_scale.grid(row=4, column=1, sticky=tk.W + tk.E)

        self.led_send_button = tk.Button(master, text="Send LED", command=self.send_led_command_wrapper) # Use wrapper
        self.led_send_button.grid(row=4, column=2)

        self.led_feedback_label = tk.Label(master, text="Last LED Brightness: N/A")
        self.led_feedback_label.grid(row=5, column=0, columnspan=3, sticky=tk.W)

        # --- LED Presets ---
        self.led_preset_frame = tk.Frame(master)
        self.led_preset_frame.grid(row=6, column=0, columnspan=3, pady=5)

        self.led_full_button = tk.Button(self.led_preset_frame, text="Full", command=lambda: self.send_led_command_wrapper(255)) #Use Wrapper
        self.led_full_button.pack(side=tk.LEFT, padx=5)

        self.led_half_button = tk.Button(self.led_preset_frame, text="Half", command=lambda: self.send_led_command_wrapper(128)) #Use Wrapper
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
                self.servo_var.set(self.servo_current_pos)  # Revert
        else:
            self.send_servo_command(value)

    def open_lid(self):
        """Opens the lid (servo to position 50) with LED off, and includes a delay."""
        if self.servo_current_pos == 50:
            messagebox.showinfo("Info", "Lid is already open.")
            return

        self.send_led_command(0)  # Turn LED off
        self.master.after(1000, self.send_servo_command, 50)  # Delay then open

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
            return

        selected_port = self.port_combobox.get()
        if not selected_port:
            return

        try:
            self.serialInst = serial.Serial(port=selected_port, baudrate=9600, timeout=1, write_timeout=1)
            self.log_message(f"Connecting to {selected_port}...")
            self.serialInst.dtr = False
            time.sleep(0.1)
            self.serialInst.open()
            time.sleep(2.0)  # Wait for Arduino to boot
            self.serialInst.reset_input_buffer()
            self.serialInst.dtr = True
            self.status_label.config(text=f"Connected to {selected_port}", fg="green")
            self.connect_button.config(text="Disconnect")
            self.receive_thread_running = True
            self.receive_thread.start()
            self.send_command("GET:SERVO")
            self.send_command("GET:LED")

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
        if value == 3 and self.servo_current_pos == 180:
            messagebox.showinfo("Info", "Lid is already closed.")
            return

        # Turn LED off *before* any preset that moves the servo:
        self.send_led_command(0)
        # Schedule actual preset command after a delay
        self.master.after(500, self._send_preset_after_led, value)

    def _send_preset_after_led(self, value):
        # This function sends the *actual* preset command
        self.send_command(f"PRESET:{value}")


    def send_led_command_wrapper(self, value=None):
        #Use the value from the slider if called without an argument, else use a provided preset value
        if value is None:
             value = self.led_var.get()

        # Check if the lid is closed (servo at 180) *before* allowing LED to turn on
        if self.servo_current_pos == 180 or value == 0 :
            self.send_led_command(value)
        else:
            # Prompt the user
            response = messagebox.askyesno(
                "Warning",
                "The lid is open.  Do you want to turn on the flat panel?"
            )
            if response:  # User clicked "Yes"
                 #Close the lid and turn led on
                self.send_preset_command(3) #Closes lid
                self.master.after(1500,self.send_led_command,value) #Turn LED on after
            else:
                self.led_var.set(0)

    def send_led_command(self, value):
          self.send_command(f"LED:{value}")


    def send_command(self, command):
        if self.serialInst.is_open:
            try:
                self.serialInst.write((command + '\n').encode('utf-8'))
                self.log_message(f"Sent: {command}")
            except (serial.SerialException, OSError) as e:
                self.log_message(f"Error: {e}")
                self.status_label.config(text=f"Error: {e}", fg="red")
                self.connect_serial()

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
                    self.connect_serial()
                    return  # Exit thread on serial error
                time.sleep(0.01)

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
                        self.servo_var.set(value)
                        self.servo_current_pos = value
                        self.servo_feedback_label.config(text=f"Last Servo Pos: {value}")
                    except ValueError:
                        self.log_message(f"Invalid servo value: {value_str}")

                elif command == "LED":
                    try:
                        value = int(value_str)
                        self.led_var.set(value)
                        self.led_current_brightness = value
                        self.led_feedback_label.config(text=f"Last LED Brightness: {value}")
                    except ValueError:
                        self.log_message(f"Invalid LED value: {value_str}")

                elif command == "PRESET":
                  try:
                      preset_num = int(parts[1])
                      if len(parts) > 3: #For other than OPEN_ON, OPEN_OFF
                          servo_val = int(parts[3])
                          led_val = int(parts[5])
                          self.servo_var.set(servo_val)
                          self.servo_current_pos = servo_val
                          self.led_var.set(led_val)
                          self.led_current_brightness = led_val
                      elif preset_num == 1:  #If preset num is 1, we have open one
                          servo_val = 50
                          self.servo_var.set(servo_val)
                          self.servo_current_pos = servo_val
                          if(parts[2] == "SERVO"): #If we have Servo
                              led_val = int(parts[4])
                              self.led_var.set(led_val)
                              self.led_current_brightness = led_val


                      self.servo_feedback_label.config(text=f"Last Servo Pos: {servo_val}")
                      self.led_feedback_label.config(text=f"Last LED Brightness: {led_val}")


                  except (ValueError, IndexError) as e:
                      print(f"Error processing preset feedback: {e}")
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
