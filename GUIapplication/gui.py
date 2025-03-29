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
# --- LED On/Off Value (Now just 1 for On) ---
# LED_ON_BRIGHTNESS = 255 # No longer needed
# ---

# --- Main Application Class ---
class TelescopeCoverApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Telescope Cover Control")
        self.geometry("450x350") # Keep reduced height

        self.serial_port = None
        self.port_var = tk.StringVar()
        self.connected = tk.BooleanVar(value=False)
        self.stop_reading_flag = False

        # --- Status Frame ---
        status_frame = ttk.LabelFrame(self, text="Status")
        status_frame.grid(row=3, column=0, padx=10, pady=(5, 10), sticky="nsew")
        self.status_text = tk.Text(status_frame, height=5, wrap=tk.WORD, state=tk.DISABLED)
        status_scrollbar = ttk.Scrollbar(status_frame, orient=tk.VERTICAL, command=self.status_text.yview)
        self.status_text['yscrollcommand'] = status_scrollbar.set
        status_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.status_text.pack(side=tk.LEFT, padx=(5,0), pady=5, fill="both", expand=True)

        # --- Serial Connection Frame ---
        connection_frame = ttk.LabelFrame(self, text="Connection")
        connection_frame.grid(row=0, column=0, padx=10, pady=10, sticky="ew")
        ttk.Label(connection_frame, text="COM Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.port_combobox = ttk.Combobox(connection_frame, textvariable=self.port_var, width=15, state="readonly")
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5)
        self.connect_button = ttk.Button(connection_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=2, padx=5, pady=5)
        self.refresh_button = ttk.Button(connection_frame, text="Refresh Ports", command=self.update_port_list)
        self.refresh_button.grid(row=0, column=3, padx=5, pady=5)

        # --- Servo Control Frame ---
        servo_frame = ttk.LabelFrame(self, text="Servo Control")
        servo_frame.grid(row=1, column=0, padx=10, pady=5, sticky="ew")
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
        led_on_off_frame.grid(row=2, column=0, padx=10, pady=5, sticky="ew")
        self.led_on_var = tk.BooleanVar(value=False)
        self.led_checkbutton = ttk.Checkbutton(
            led_on_off_frame,
            text="LED On",
            variable=self.led_on_var,
            command=self.toggle_led # Calls updated function
        )
        self.led_checkbutton.pack(side=tk.LEFT, padx=10, pady=5)

        # Configure grid resizing behavior
        self.grid_rowconfigure(3, weight=1)
        self.grid_columnconfigure(0, weight=1)
        connection_frame.grid_columnconfigure(1, weight=1)
        servo_frame.grid_columnconfigure(1, weight=1)

        # --- Initial State ---
        self.update_port_list()
        self.update_ui_connection_state()

        # --- Close Protocol ---
        self.protocol("WM_DELETE_WINDOW", self.on_closing)

    # --- Serial Communication Methods (Unchanged) ---
    def update_port_list(self):
        try: ports = [p.device for p in serial.tools.list_ports.comports()]; self.port_combobox['values'] = ports; current = self.port_var.get(); self.port_var.set(ports[0] if ports and current not in ports else (current if current in ports else "")); self.log_status("Refreshed COM port list.")
        except Exception as e: self.log_status(f"Error refreshing COM ports: {e}"); messagebox.showerror("Port Error", f"Could not retrieve COM ports:\n{e}")
    def toggle_connection(self): (self.connect if not self.connected.get() else self.disconnect)()
    def connect(self):
        port_name=self.port_var.get();
        if not port_name: messagebox.showerror("Connection Error", "Select COM port."); return
        try:
            self.log_status(f"Connecting to {port_name}...");
            if self.serial_port and self.serial_port.is_open: self.serial_port.close(); time.sleep(0.1)
            self.serial_port=serial.Serial(port_name,BAUD_RATE,timeout=READ_TIMEOUT); time.sleep(1.5); self.connected.set(True); self.log_status(f"Connected to {port_name}.");
            self.stop_reading_flag=False; self.after(GUI_UPDATE_INTERVAL,self.read_serial_data); self.send_command("COMMAND:GETSTATUS"); self.update_ui_connection_state()
        except Exception as e: messagebox.showerror("Connection Error",f"Failed: {e}"); self.log_status(f"Connection failed: {e}"); self.serial_port=None; self.connected.set(False); self.update_ui_connection_state()
    def disconnect(self):
        self.log_status("Disconnecting..."); self.stop_reading_flag=True
        if self.serial_port and self.serial_port.is_open:
            try: self.serial_port.close()
            except Exception as e: self.log_status(f"Error closing port: {e}")
        self.serial_port=None; self.connected.set(False); self.log_status("Disconnected."); self.update_ui_connection_state()
    def send_command(self,command):
        if self.serial_port and self.serial_port.is_open:
            try: self.serial_port.write((command+'\n').encode('ascii')); self.log_status(f"Sent: {command}"); return True
            except Exception as e: self.log_status(f"Serial write error: {e}"); messagebox.showerror("Serial Error",f"Send failed: {e}"); self.disconnect(); return False
        else:
            if command!="COMMAND:GETSTATUS" or self.connected.get(): messagebox.showwarning("Not Connected","Connect first."); return False
    def read_serial_data(self):
        if self.connected.get() and self.serial_port and self.serial_port.is_open and not self.stop_reading_flag:
            lines_read=0
            try:
                while self.serial_port.in_waiting>0:
                    line=self.serial_port.readline().decode('ascii',errors='ignore').strip()
                    if line: lines_read+=1; self.log_status(f"Recv: {line}"); self.parse_response(line)
                    if lines_read>50: self.log_status("Warn: Many lines read."); break
            except serial.SerialException as e: self.log_status(f"Read error: {e}. Disconnecting."); messagebox.showerror("Serial Error",f"Read error:\n{e}\n\nDisconnecting."); self.disconnect(); return
            except Exception as e: self.log_status(f"Error processing data: {e}")
            if self.connected.get() and not self.stop_reading_flag: self.after(GUI_UPDATE_INTERVAL,self.read_serial_data)
            else: self.log_status("Serial reading stopped.") # Log reason if needed

    # --- Feedback Parsing (Unchanged Logic, just interpreting 0/1 now) ---
    def parse_response(self, response):
        if response.startswith("RESULT:STATUS:"):
            try:
                parts = response.split(':')
                if len(parts) == 4:
                    angle = int(parts[2])
                    led_state = int(parts[3]) # Now 0 or 1

                    angle = max(MIN_SERVO_ANGLE, min(MAX_SERVO_ANGLE, angle))

                    if self.servo_angle_var.get() != angle: self.servo_angle_var.set(angle)

                    # Update LED Checkbutton state (0=False, non-zero=True)
                    is_led_on_arduino = (led_state != 0) # True if led_state is 1 (or any non-zero)
                    if self.led_on_var.get() != is_led_on_arduino:
                        self.led_on_var.set(is_led_on_arduino)
                        # No need to log here, already logged Recv: line
            except Exception as e: self.log_status(f"Error parsing status '{response}': {e}")
        elif response == "RESULT:STATE:MOVING": self.log_status("Cover is moving...")
        elif response.startswith("ERROR:"): self.log_status(f"Arduino Error: {response}"); messagebox.showwarning("Arduino Error",f"Error:\n{response}")

    # --- GUI Callbacks & Updates ---
    def set_controls_state(self, state):
         widgets = [self.servo_slider, self.servo_entry, self.led_checkbutton]
         try:
             container = self.children.get('!telescopecoverapp.!labelframe2.!frame')
             if container and container.winfo_exists(): widgets.extend(c for c in container.winfo_children() if isinstance(c, ttk.Button))
         except: pass
         for w in widgets:
             try:
                 if w and w.winfo_exists(): w.config(state=state)
             except: pass
    def update_ui_connection_state(self):
        is_conn = self.connected.get(); state = tk.NORMAL if is_conn else tk.DISABLED; self.set_controls_state(state)
        try:
            if self.port_combobox and self.port_combobox.winfo_exists(): self.port_combobox.config(state="readonly" if not is_conn else tk.DISABLED)
            if self.refresh_button and self.refresh_button.winfo_exists(): self.refresh_button.config(state=tk.NORMAL if not is_conn else tk.DISABLED)
            if self.connect_button and self.connect_button.winfo_exists(): self.connect_button.config(text="Disconnect" if is_conn else "Connect")
        except: pass
    def log_status(self, message):
        try:
            if hasattr(self,'status_text') and self.status_text and self.status_text.winfo_exists():
                self.status_text.config(state=tk.NORMAL); ts=time.strftime("%H:%M:%S",time.localtime()); self.status_text.insert(tk.END,f"{ts}: {message}\n"); self.status_text.see(tk.END); self.status_text.config(state=tk.DISABLED)
        except Exception as e: print(f"Log Status Error: {e} - Msg: {message}")
    def update_servo_entry_from_slider(self, value): pass
    def set_servo_from_slider_release(self, event=None):
        if self.connected.get(): self.set_servo_from_entry()
        else: self.log_status("Cannot set angle: Not connected.")
    def set_servo_from_entry(self, event=None):
        if not self.connected.get(): messagebox.showwarning("Not Connected","Connect first."); return
        try:
            angle=self.servo_angle_var.get()
            if MIN_SERVO_ANGLE<=angle<=MAX_SERVO_ANGLE: self.send_command(f"COMMAND:SETPOS:{angle}")
            else: messagebox.showwarning("Input Error",f"Angle must be {MIN_SERVO_ANGLE}-{MAX_SERVO_ANGLE}."); self.send_command("COMMAND:GETSTATUS")
        except: messagebox.showwarning("Input Error","Invalid angle."); self.send_command("COMMAND:GETSTATUS")

    # --- Updated LED Toggle Function ---
    def toggle_led(self):
        """Called when the LED Checkbutton state is changed."""
        if not self.connected.get():
            messagebox.showwarning("Not Connected", "Connect first.")
            # Revert the checkbox state visually if not connected
            self.led_on_var.set(not self.led_on_var.get())
            return

        if self.led_on_var.get(): # Checkbutton is now checked (True) -> Turn ON
            self.log_status("Turning LED On...")
            self.send_command("COMMAND:SETLED:1") # Send non-zero value (1 is clear)
        else: # Checkbutton is now unchecked (False) -> Turn OFF
            self.log_status("Turning LED Off...")
            self.send_command("COMMAND:SETLED:0") # Send zero value

    # --- Servo Open/Close Functions (Unchanged) ---
    def servo_open(self): self.send_command(f"COMMAND:SETPOS:{MAX_SERVO_ANGLE}")
    def servo_close(self): self.send_command(f"COMMAND:SETPOS:{MIN_SERVO_ANGLE}")

    # --- Closing Function ---
    def on_closing(self):
        self.log_status("Closing..."); self.stop_reading_flag=True; self.after(int(GUI_UPDATE_INTERVAL*1.5),self._perform_disconnect_and_destroy)
    def _perform_disconnect_and_destroy(self):
        if self.connected.get(): self.disconnect()
        if self.winfo_exists(): self.destroy()

# --- Run the Application ---
if __name__ == "__main__":
    app = TelescopeCoverApp()
    app.mainloop()
