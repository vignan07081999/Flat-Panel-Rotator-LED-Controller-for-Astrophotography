import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import time
# import threading # Not strictly needed if only using root.after

# --- Constants ---
BAUD_RATE = 57600
READ_TIMEOUT = 1 # How long pyserial waits for data (seconds)
GUI_UPDATE_INTERVAL = 100 # How often to check for serial data (milliseconds)
# --- Angle Limits ---
MIN_SERVO_ANGLE = 20
MAX_SERVO_ANGLE = 180
# --- LED On/Off Control ---
LED_ON_BRIGHTNESS = 255 # Brightness value to send when GUI selects "On"
# ---

# --- Main Application Class ---
class TelescopeCoverApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Telescope Cover Control")
        # Adjusted geometry slightly for new layout
        self.geometry("450x350") # Reduced height as brightness controls removed

        self.serial_port = None
        self.port_var = tk.StringVar()
        self.connected = tk.BooleanVar(value=False)
        self.stop_reading_flag = False # Flag to stop the read loop

        # --- Status Frame (Create this early as log_status is called during init) ---
        status_frame = ttk.LabelFrame(self, text="Status")
        status_frame.grid(row=3, column=0, padx=10, pady=(5, 10), sticky="nsew") # Moved to row 3

        self.status_text = tk.Text(status_frame, height=5, wrap=tk.WORD, state=tk.DISABLED) # Reduced height
        status_scrollbar = ttk.Scrollbar(status_frame, orient=tk.VERTICAL, command=self.status_text.yview)
        self.status_text['yscrollcommand'] = status_scrollbar.set
        status_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.status_text.pack(side=tk.LEFT, padx=(5,0), pady=5, fill="both", expand=True)


        # --- Serial Connection Frame ---
        connection_frame = ttk.LabelFrame(self, text="Connection")
        connection_frame.grid(row=0, column=0, padx=10, pady=10, sticky="ew") # Row 0

        ttk.Label(connection_frame, text="COM Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.port_combobox = ttk.Combobox(connection_frame, textvariable=self.port_var, width=15, state="readonly")
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5)
        self.connect_button = ttk.Button(connection_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=2, padx=5, pady=5)
        self.refresh_button = ttk.Button(connection_frame, text="Refresh Ports", command=self.update_port_list)
        self.refresh_button.grid(row=0, column=3, padx=5, pady=5)

        # --- Servo Control Frame ---
        servo_frame = ttk.LabelFrame(self, text="Servo Control")
        servo_frame.grid(row=1, column=0, padx=10, pady=5, sticky="ew") # Row 1

        ttk.Label(servo_frame, text=f"Angle ({MIN_SERVO_ANGLE}-{MAX_SERVO_ANGLE}):").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.servo_angle_var = tk.IntVar(value=MIN_SERVO_ANGLE)
        self.servo_slider = ttk.Scale(servo_frame, from_=MIN_SERVO_ANGLE, to=MAX_SERVO_ANGLE, orient=tk.HORIZONTAL,
                                      variable=self.servo_angle_var, length=200,
                                      command=self.update_servo_entry_from_slider)
        self.servo_slider.bind("<ButtonRelease-1>", self.set_servo_from_slider_release)
        self.servo_slider.grid(row=0, column=1, columnspan=2, padx=5, pady=5, sticky="ew")
        self.servo_entry = ttk.Entry(servo_frame, textvariable=self.servo_angle_var, width=5)
        self.servo_entry.grid(row=0, column=3, padx=5, pady=5)
        self.servo_entry.bind("<Return>", self.set_servo_from_entry)
        servo_button_frame = ttk.Frame(servo_frame)
        servo_button_frame.grid(row=1, column=0, columnspan=4, pady=(0,5))
        ttk.Button(servo_button_frame, text=f"Open ({MAX_SERVO_ANGLE}°)", command=self.servo_open).pack(side=tk.LEFT, padx=5)
        ttk.Button(servo_button_frame, text=f"Close ({MIN_SERVO_ANGLE}°)", command=self.servo_close).pack(side=tk.LEFT, padx=5)

        # --- LED On/Off Control Frame ---
        led_on_off_frame = ttk.LabelFrame(self, text="LED Control")
        led_on_off_frame.grid(row=2, column=0, padx=10, pady=5, sticky="ew") # Row 2

        self.led_on_var = tk.BooleanVar(value=False) # Variable to track On/Off state
        self.led_checkbutton = ttk.Checkbutton(
            led_on_off_frame,
            text="LED On",
            variable=self.led_on_var,
            command=self.toggle_led # Function called when button is clicked
        )
        # Pad the checkbutton within its frame for better spacing
        self.led_checkbutton.pack(side=tk.LEFT, padx=10, pady=5)

        # Configure grid resizing behavior
        self.grid_rowconfigure(3, weight=1) # Status frame row expands
        self.grid_columnconfigure(0, weight=1) # Main column expands
        connection_frame.grid_columnconfigure(1, weight=1)
        servo_frame.grid_columnconfigure(1, weight=1)
        # led_on_off_frame doesn't need column config as Checkbutton fills

        # --- Initial State ---
        self.update_port_list()
        self.update_ui_connection_state()

        # --- Close Protocol ---
        self.protocol("WM_DELETE_WINDOW", self.on_closing)

    # --- Serial Communication Methods (update_port_list, toggle_connection, connect, disconnect, send_command, read_serial_data) ---
    # ... (These methods remain unchanged) ...
    def update_port_list(self):
        """Updates the COM port dropdown list."""
        try:
            ports = [port.device for port in serial.tools.list_ports.comports()]
            self.port_combobox['values'] = ports
            if ports:
                current_selection = self.port_var.get()
                if current_selection not in ports:
                    self.port_var.set(ports[0])
            else:
                self.port_var.set("")
            self.log_status("Refreshed COM port list.")
        except Exception as e:
            self.log_status(f"Error refreshing COM ports: {e}")
            messagebox.showerror("Port Error", f"Could not retrieve COM ports:\n{e}")

    def toggle_connection(self):
        """Connects or disconnects the serial port."""
        if not self.connected.get():
            self.connect()
        else:
            self.disconnect()

    def connect(self):
        """Establishes serial connection."""
        port_name = self.port_var.get()
        if not port_name:
            messagebox.showerror("Connection Error", "Please select a COM port.")
            return
        try:
            self.log_status(f"Attempting to connect to {port_name}...")
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
                time.sleep(0.1)
            self.serial_port = serial.Serial(port_name, BAUD_RATE, timeout=READ_TIMEOUT)
            time.sleep(1.5)
            self.connected.set(True)
            self.log_status(f"Connected to {port_name}.")
            self.stop_reading_flag = False
            self.after(GUI_UPDATE_INTERVAL, self.read_serial_data)
            self.send_command("COMMAND:GETSTATUS")
            self.update_ui_connection_state()
        except serial.SerialException as e:
            messagebox.showerror("Connection Error", f"Failed to connect to {port_name}:\n{e}")
            self.log_status(f"Connection failed: {e}")
            self.serial_port = None
            self.connected.set(False)
            self.update_ui_connection_state()
        except Exception as e:
             messagebox.showerror("Connection Error", f"An unexpected error occurred:\n{e}")
             self.log_status(f"Unexpected connection error: {e}")
             self.serial_port = None
             self.connected.set(False)
             self.update_ui_connection_state()

    def disconnect(self):
        """Closes the serial connection."""
        self.log_status("Disconnecting...")
        self.stop_reading_flag = True
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
            except Exception as e:
                self.log_status(f"Error closing port: {e}")
        self.serial_port = None
        self.connected.set(False)
        self.log_status("Disconnected.")
        self.update_ui_connection_state()

    def send_command(self, command):
        """Sends a command string to the Arduino."""
        if self.serial_port and self.serial_port.is_open:
            try:
                full_command = command + '\n'
                self.serial_port.write(full_command.encode('ascii'))
                self.log_status(f"Sent: {command}")
                return True
            except serial.SerialException as e:
                self.log_status(f"Serial write error: {e}")
                messagebox.showerror("Serial Error", f"Failed to send command:\n{e}")
                self.disconnect()
                return False
            except Exception as e:
                 self.log_status(f"Error sending command '{command}': {e}")
                 return False
        else:
            if command != "COMMAND:GETSTATUS" or self.connected.get():
                messagebox.showwarning("Not Connected", "Cannot send command. Please connect first.")
            return False

    def read_serial_data(self):
        """Periodically checks for incoming data via root.after."""
        if self.connected.get() and self.serial_port and self.serial_port.is_open and not self.stop_reading_flag:
            lines_read = 0
            try:
                while self.serial_port.in_waiting > 0:
                    line = self.serial_port.readline().decode('ascii', errors='ignore').strip()
                    if line:
                        lines_read += 1
                        self.log_status(f"Recv: {line}")
                        self.parse_response(line)
                    if lines_read > 50:
                        self.log_status("Warning: Read many lines in one interval.")
                        break
            except serial.SerialException as e:
                self.log_status(f"Serial read error: {e}. Disconnecting.")
                if self.connected.get():
                     messagebox.showerror("Serial Error", f"Serial read error:\n{e}\n\nDisconnecting.")
                     self.disconnect()
                return
            except Exception as e:
                self.log_status(f"Error processing serial data: {e}")

            if self.connected.get() and not self.stop_reading_flag:
                 self.after(GUI_UPDATE_INTERVAL, self.read_serial_data)
            else:
                if not self.connected.get(): self.log_status("Serial reading stopped due to disconnect.")
                elif self.stop_reading_flag: self.log_status("Serial reading stopped by flag.")

    # --- Feedback Parsing ---
    def parse_response(self, response):
        """Parses known responses from the Arduino and updates the GUI."""
        if response.startswith("RESULT:STATUS:"):
            try:
                parts = response.split(':')
                if len(parts) == 4: # RESULT, STATUS, angle, brightness
                    angle_str, brightness_str = parts[2], parts[3]
                    angle = int(angle_str)
                    brightness = int(brightness_str)

                    angle = max(MIN_SERVO_ANGLE, min(MAX_SERVO_ANGLE, angle))

                    # Update Servo controls
                    if self.servo_angle_var.get() != angle:
                         self.servo_angle_var.set(angle)

                    # --- Update LED On/Off Checkbutton based on brightness ---
                    # Check if the checkbutton state needs changing
                    is_led_on_arduino = (brightness > 0)
                    if self.led_on_var.get() != is_led_on_arduino:
                        self.led_on_var.set(is_led_on_arduino)
                        self.log_status(f"GUI LED state updated to {'On' if is_led_on_arduino else 'Off'}")

            except ValueError:
                self.log_status(f"Error parsing status: Invalid number in '{response}'")
            except Exception as e:
                self.log_status(f"Error parsing status '{response}': {e}")

        elif response == "RESULT:STATE:MOVING":
             self.log_status("Cover is moving...")
        elif response.startswith("ERROR:OUT_OF_RANGE"):
            self.log_status(f"Arduino Info: {response}")
            messagebox.showinfo("Angle Adjusted by Arduino", f"Requested angle was outside limits.\nArduino used constrained value.\n({response})")
        elif response.startswith("ERROR:"):
            self.log_status(f"Arduino Error: {response}")
            messagebox.showwarning("Arduino Error", f"Received error from Arduino:\n{response}")
        # No action needed for OK or other STATE messages here

    # --- GUI Callbacks & Updates ---

    def set_controls_state(self, state):
         """Helper to enable/disable all relevant controls, checking existence."""
         widgets_to_update = [
             self.servo_slider, self.servo_entry,
             self.led_checkbutton # Add the new checkbutton
         ]
         # Find servo buttons
         try:
             # Using internal names is fragile, better to store references if this breaks
             servo_button_container = self.children.get('!telescopecoverapp.!labelframe2.!frame')
             if servo_button_container and servo_button_container.winfo_exists():
                 for child in servo_button_container.winfo_children():
                     if isinstance(child, ttk.Button):
                         widgets_to_update.append(child)
         except Exception: pass

         # Update state for all collected widgets
         for widget in widgets_to_update:
             try:
                 if widget and widget.winfo_exists():
                     widget.config(state=state)
             except tk.TclError: pass

    def update_ui_connection_state(self):
        """Enable/disable controls based on connection status."""
        is_conn = self.connected.get()
        state = tk.NORMAL if is_conn else tk.DISABLED
        self.set_controls_state(state) # Use helper to set servo/LED controls

        # Handle connection specific controls separately
        try:
            if self.port_combobox and self.port_combobox.winfo_exists():
                 self.port_combobox.config(state="readonly" if not is_conn else tk.DISABLED)
            if self.refresh_button and self.refresh_button.winfo_exists():
                 self.refresh_button.config(state=tk.NORMAL if not is_conn else tk.DISABLED)
            if self.connect_button and self.connect_button.winfo_exists():
                 self.connect_button.config(text="Disconnect" if is_conn else "Connect")
        except tk.TclError: pass

    def log_status(self, message):
        """Adds a message to the status text box, safely."""
        try:
            if hasattr(self, 'status_text') and self.status_text and self.status_text.winfo_exists():
                self.status_text.config(state=tk.NORMAL)
                timestamp = time.strftime("%H:%M:%S", time.localtime())
                self.status_text.insert(tk.END, f"{timestamp}: {message}\n")
                self.status_text.see(tk.END)
                self.status_text.config(state=tk.DISABLED)
        except tk.TclError as e:
             print(f"Log Status TclError (likely during shutdown): {e} - Message: {message}")
        except Exception as e:
             print(f"Unexpected error in log_status: {e} - Message: {message}")


    def update_servo_entry_from_slider(self, value):
        """Called when servo slider is dragged. Updates entry visually."""
        pass # Updated by variable link

    def set_servo_from_slider_release(self, event=None):
        """Sends servo command when slider is released."""
        if self.connected.get():
            self.set_servo_from_entry()
        else:
            self.log_status("Cannot set angle: Not connected.")

    def set_servo_from_entry(self, event=None):
        """Sends servo angle command from the entry field value."""
        if not self.connected.get():
            messagebox.showwarning("Not Connected", "Cannot set angle. Please connect first.")
            return
        try:
            angle = self.servo_angle_var.get()
            if MIN_SERVO_ANGLE <= angle <= MAX_SERVO_ANGLE:
                self.send_command(f"COMMAND:SETPOS:{angle}")
            else:
                messagebox.showwarning("Input Error", f"Servo angle must be between {MIN_SERVO_ANGLE} and {MAX_SERVO_ANGLE}.")
                self.send_command("COMMAND:GETSTATUS") # Reset GUI from Arduino state
        except (tk.TclError, ValueError):
             messagebox.showwarning("Input Error", "Invalid servo angle entered. Please enter a number.")
             self.send_command("COMMAND:GETSTATUS") # Reset GUI from Arduino state

    # --- New LED Toggle Function ---
    def toggle_led(self):
        """Called when the LED Checkbutton state is changed."""
        if not self.connected.get():
            self.log_status("Cannot toggle LED: Not connected.")
            # Revert checkbutton state if not connected? Or just log/warn.
            # self.led_on_var.set(not self.led_on_var.get()) # Revert the change
            messagebox.showwarning("Not Connected", "Cannot control LED. Please connect first.")
            return

        if self.led_on_var.get(): # If checkbutton is now checked (True)
            self.log_status("Turning LED On...")
            self.send_command(f"COMMAND:SETLED:{LED_ON_BRIGHTNESS}") # Send On command (e.g., SETLED:255)
        else: # If checkbutton is now unchecked (False)
            self.log_status("Turning LED Off...")
            self.send_command("COMMAND:SETLED:0") # Send Off command

    # --- Servo Open/Close Functions ---
    def servo_open(self):
        """Sends command to fully open the cover (MAX_SERVO_ANGLE)."""
        self.send_command(f"COMMAND:SETPOS:{MAX_SERVO_ANGLE}")

    def servo_close(self):
        """Sends command to close the cover to the minimum angle (MIN_SERVO_ANGLE)."""
        self.send_command(f"COMMAND:SETPOS:{MIN_SERVO_ANGLE}")

    # --- Closing Function ---
    def on_closing(self):
        """Handles window close event: safely disconnects and destroys window."""
        self.log_status("Close button clicked...")
        self.stop_reading_flag = True
        # Use a short delay before disconnect/destroy to allow final tasks
        self.after(int(GUI_UPDATE_INTERVAL * 1.5), self._perform_disconnect_and_destroy)

    def _perform_disconnect_and_destroy(self):
        """Helper function to disconnect and destroy after a short delay."""
        if self.connected.get():
             self.disconnect()
        # Check if root window still exists before destroying
        if self.winfo_exists():
            self.destroy()

# --- Run the Application ---
if __name__ == "__main__":
    app = TelescopeCoverApp()
    app.mainloop()
