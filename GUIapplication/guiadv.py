import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import queue # For thread-safe GUI updates

# --- Constants based on Arduino Firmware ---
BAUD_RATE = 57600
COMMAND_PING = "COMMAND:PING\n"
COMMAND_INFO = "COMMAND:INFO\n"
COMMAND_GETSTATE = "COMMAND:GETSTATE\n"
COMMAND_OPEN = "COMMAND:OPEN\n"
COMMAND_CLOSE = "COMMAND:CLOSE\n"
RESULT_STATE_OPEN = "RESULT:STATE:OPEN"
RESULT_STATE_CLOSED = "RESULT:STATE:CLOSED"
# --- End Constants ---

class ServoControllerApp:
    def __init__(self, master):
        self.master = master
        self.ser = None
        self.port_list = []
        self.selected_port = tk.StringVar()
        self.read_thread = None
        self.is_running = False
        self.message_queue = queue.Queue() # Queue for messages from read_thread to GUI

        master.title("Flat Panel Servo Control")
        master.geometry("450x400") # Adjusted size for better layout

        # --- Style ---
        style = ttk.Style()
        style.theme_use('clam') # Or 'alt', 'default', 'classic'

        # --- Connection Frame ---
        connection_frame = ttk.LabelFrame(master, text="Connection", padding=(10, 5))
        connection_frame.pack(pady=10, padx=10, fill="x")

        # Port selection
        port_label = ttk.Label(connection_frame, text="Serial Port:")
        port_label.grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.port_combobox = ttk.Combobox(connection_frame, textvariable=self.selected_port, state="readonly", width=20)
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5, sticky="ew")
        self.refresh_button = ttk.Button(connection_frame, text="Refresh", command=self.populate_ports, width=8)
        self.refresh_button.grid(row=0, column=2, padx=5, pady=5)

        # Connect/Disconnect Buttons
        self.connect_button = ttk.Button(connection_frame, text="Connect", command=self.connect_serial, width=15)
        self.connect_button.grid(row=1, column=0, columnspan=2, padx=5, pady=8, sticky="ew")
        self.disconnect_button = ttk.Button(connection_frame, text="Disconnect", command=self.disconnect_serial, state="disabled", width=15)
        self.disconnect_button.grid(row=1, column=2, padx=5, pady=8, sticky="ew")

        connection_frame.columnconfigure(1, weight=1) # Make combobox expand horizontally

        # --- Control Frame ---
        control_frame = ttk.LabelFrame(master, text="Manual Control", padding=(10, 5))
        control_frame.pack(pady=10, padx=10, fill="x")

        self.open_button = ttk.Button(control_frame, text="Open Cover", command=self.open_cover_action, state="disabled")
        self.open_button.pack(pady=8, padx=20, fill="x")

        self.close_button = ttk.Button(control_frame, text="Close Cover", command=self.close_cover_action, state="disabled")
        self.close_button.pack(pady=8, padx=20, fill="x")

        # --- Status Frame ---
        status_frame = ttk.LabelFrame(master, text="Status & Response", padding=(10, 5))
        status_frame.pack(pady=10, padx=10, fill="both", expand=True)

        self.status_label = ttk.Label(status_frame, text="Status: Disconnected", anchor="w", wraplength=400)
        self.status_label.pack(pady=(5,0), padx=5, fill="x")

        self.cover_state_label = ttk.Label(status_frame, text="Cover State: Unknown", anchor="w", wraplength=400, font=('TkDefaultFont', 9, 'bold'))
        self.cover_state_label.pack(pady=(0,5), padx=5, fill="x")

        # Text area for Arduino responses
        self.response_text = tk.Text(status_frame, height=5, state="disabled", wrap="word", relief=tk.SUNKEN, borderwidth=1)
        self.response_scrollbar = ttk.Scrollbar(status_frame, orient="vertical", command=self.response_text.yview)
        self.response_text.configure(yscrollcommand=self.response_scrollbar.set)

        self.response_scrollbar.pack(side="right", fill="y", padx=(0,5), pady=5)
        self.response_text.pack(side="left", fill="both", expand=True, padx=(5,0), pady=5)


        # --- Initial Setup ---
        self.populate_ports()
        self.master.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.master.after(100, self.process_queue) # Start checking the message queue

    def populate_ports(self):
        """Finds available serial ports and updates the combobox."""
        self.port_list = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combobox['values'] = self.port_list
        if self.port_list:
            if not self.selected_port.get() or self.selected_port.get() not in self.port_list:
                 self.selected_port.set(self.port_list[0])
        else:
            self.selected_port.set("")
            self.port_combobox['values'] = []
            self.log_response("No serial ports found.")


    def connect_serial(self):
        """Establishes serial connection to the selected port."""
        port = self.selected_port.get()
        if not port:
            messagebox.showerror("Connection Error", "No serial port selected.")
            return

        try:
            self.log_response(f"Attempting to connect to {port} at {BAUD_RATE} baud...")
            # Added timeout for non-blocking reads later
            self.ser = serial.Serial(port, BAUD_RATE, timeout=1)
            time.sleep(2) # Give Arduino time to reset after connection

            if self.ser.is_open:
                self.is_running = True
                self.read_thread = threading.Thread(target=self.read_from_port, daemon=True)
                self.read_thread.start()

                self.update_status("Connected", "green")
                self.cover_state_label.config(text="Cover State: Unknown (Requesting...)")
                self.log_response(f"Successfully connected to {port}.")

                # Update GUI state
                self.connect_button.config(state="disabled")
                self.disconnect_button.config(state="normal")
                self.port_combobox.config(state="disabled")
                self.refresh_button.config(state="disabled")
                self.open_button.config(state="normal")
                self.close_button.config(state="normal")

                # Send initial commands to get info/state
                self.send_command(COMMAND_PING)
                time.sleep(0.2) # Small delay between commands
                self.send_command(COMMAND_GETSTATE)


            else:
                 messagebox.showerror("Connection Error", f"Failed to open port {port}.")
                 self.update_status("Connection Failed", "red")

        except serial.SerialException as e:
            messagebox.showerror("Connection Error", f"Could not connect to {port}.\nError: {e}")
            self.update_status(f"Error: {e}", "red")
            self.log_response(f"Error connecting: {e}")
            if self.ser:
                self.ser.close()
            self.ser = None
        except Exception as e:
            messagebox.showerror("Error", f"An unexpected error occurred: {e}")
            self.update_status(f"Unexpected Error", "red")
            self.log_response(f"Unexpected error: {e}")
            if self.ser:
                self.ser.close()
            self.ser = None

    def disconnect_serial(self):
        """Closes the serial connection."""
        self.is_running = False # Signal the reading thread to stop
        if self.read_thread:
             self.read_thread.join(timeout=2) # Wait for thread to finish

        if self.ser and self.ser.is_open:
            try:
                self.ser.close()
                self.log_response("Serial port disconnected.")
            except Exception as e:
                self.log_response(f"Error closing port: {e}")

        self.ser = None
        self.update_status("Disconnected", "black")
        self.cover_state_label.config(text="Cover State: Unknown")

        # Update GUI state
        self.connect_button.config(state="normal")
        self.disconnect_button.config(state="disabled")
        self.port_combobox.config(state="readonly" if self.port_list else "disabled")
        self.refresh_button.config(state="normal")
        self.open_button.config(state="disabled")
        self.close_button.config(state="disabled")
        self.populate_ports() # Refresh port list in case it changed


    def send_command(self, command):
        """Sends a command string over the serial port."""
        if self.ser and self.ser.is_open:
            try:
                # Ensure command ends with newline and is bytes
                if isinstance(command, str):
                    if not command.endswith('\n'):
                         command += '\n'
                    command_bytes = command.encode('ascii')
                elif isinstance(command, bytes):
                     if not command.endswith(b'\n'):
                         command += b'\n'
                     command_bytes = command
                else:
                    self.log_response(f"Error: Invalid command type {type(command)}")
                    return

                self.ser.write(command_bytes)
                # Log command without the newline for readability
                self.log_response(f"Sent: {command_bytes.decode('ascii').strip()}")

            except serial.SerialException as e:
                self.log_response(f"Serial write error: {e}")
                messagebox.showerror("Serial Error", f"Failed to send command.\nError: {e}")
                self.disconnect_serial() # Disconnect on write error
            except Exception as e:
                 self.log_response(f"Unexpected write error: {e}")
                 messagebox.showerror("Error", f"An unexpected error occurred during send.\nError: {e}")
        else:
            self.log_response("Cannot send command: Not connected.")
            # messagebox.showwarning("Not Connected", "Please connect to the serial port first.")


    def read_from_port(self):
        """Reads data from serial port in a separate thread."""
        while self.is_running and self.ser and self.ser.is_open:
            try:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline()
                    if line:
                        try:
                            decoded_line = line.decode('ascii').strip()
                            if decoded_line: # Don't queue empty lines
                                self.message_queue.put(decoded_line)
                        except UnicodeDecodeError:
                            self.message_queue.put(f"Received non-ASCII data: {line}")
            except serial.SerialException as e:
                 # Put error message in queue for main thread to handle
                 self.message_queue.put(f"SERIAL_ERROR:{e}")
                 break # Exit thread on serial error
            except Exception as e:
                 self.message_queue.put(f"READ_THREAD_ERROR:{e}")
                 break # Exit thread on other errors
            time.sleep(0.05) # Small delay to prevent high CPU usage


    def process_queue(self):
        """Processes messages from the read thread queue in the main GUI thread."""
        try:
            while True: # Process all messages currently in the queue
                message = self.message_queue.get_nowait()
                # Check for special error messages from read_thread
                if isinstance(message, str):
                    if message.startswith("SERIAL_ERROR:"):
                        error_msg = message.split(":", 1)[1]
                        self.log_response(f"Serial read error: {error_msg}")
                        messagebox.showerror("Serial Error", f"Lost connection or read error.\n{error_msg}")
                        self.disconnect_serial() # Attempt graceful disconnect
                    elif message.startswith("READ_THREAD_ERROR:"):
                         error_msg = message.split(":", 1)[1]
                         self.log_response(f"Read thread error: {error_msg}")
                         messagebox.showerror("Thread Error", f"Error in reading thread.\n{error_msg}")
                         self.disconnect_serial() # Attempt graceful disconnect
                    else:
                        # Process normal Arduino response
                        self.log_response(f"Recv: {message}")
                        self.update_cover_state(message) # Update state label if applicable

        except queue.Empty:
             pass # No messages currently in queue
        finally:
             # Reschedule after processing queue or if it was empty
             self.master.after(100, self.process_queue)


    def update_cover_state(self, response):
        """Updates the cover state label based on Arduino response."""
        if RESULT_STATE_OPEN in response:
             self.cover_state_label.config(text="Cover State: OPEN", foreground="dark green")
        elif RESULT_STATE_CLOSED in response:
             self.cover_state_label.config(text="Cover State: CLOSED", foreground="dark red")
        # Keep "Unknown" if response doesn't match known states


    def log_response(self, message):
        """Appends a message to the response text area."""
        self.response_text.config(state="normal") # Enable writing
        self.response_text.insert(tk.END, message + "\n")
        self.response_text.see(tk.END) # Scroll to the bottom
        self.response_text.config(state="disabled") # Disable writing


    def update_status(self, text, color="black"):
        """Updates the main status label."""
        self.status_label.config(text=f"Status: {text}", foreground=color)


    def open_cover_action(self):
        """Sends the open command."""
        self.send_command(COMMAND_OPEN)
        self.cover_state_label.config(text="Cover State: Opening...", foreground="orange")


    def close_cover_action(self):
        """Sends the close command."""
        self.send_command(COMMAND_CLOSE)
        self.cover_state_label.config(text="Cover State: Closing...", foreground="orange")


    def on_closing(self):
        """Handles window close event."""
        if messagebox.askokcancel("Quit", "Do you want to quit?"):
            self.is_running = False # Signal thread to stop
            if self.read_thread and self.read_thread.is_alive():
                self.read_thread.join(timeout=1) # Wait briefly for thread

            if self.ser and self.ser.is_open:
                 self.ser.close()
            self.master.destroy()


# --- Main Execution ---
if __name__ == "__main__":
    root = tk.Tk()
    app = ServoControllerApp(root)
    root.mainloop()
