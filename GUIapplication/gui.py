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
                        self.led_var.set(value) # Update slider IMMEDIATELY
                        self.led_current_brightness = value  # Update current brightness
                        self.led_feedback_label.config(text=f"Last LED Brightness: {value}")
                    except ValueError:
                        self.log_message(f"Invalid LED value: {value_str}")

                elif command == "PRESET":
                    try:
                        preset_num = int(parts[1])
                        # Handle both OPEN_OFF and other presets consistently
                        if len(parts) > 3 :
                            servo_val = int(parts[3])
                            led_val = int(parts[5])

                            self.servo_var.set(servo_val)
                            self.servo_current_pos = servo_val
                            self.led_var.set(led_val)
                            self.led_current_brightness = led_val
                            self.servo_feedback_label.config(text=f"Last Servo Pos: {servo_val}")
                            self.led_feedback_label.config(text=f"Last LED Brightness: {led_val}")  # Corrected line

                        elif preset_num == 1 and len(parts) > 4:  #If preset num is 1, we have open one
                            servo_val = 50
                            self.servo_var.set(servo_val)
                            self.servo_current_pos = servo_val
                            if(parts[2] == "SERVO"): #If we have Servo
                                led_val = int(parts[4])
                                self.led_var.set(led_val)
                                self.led_current_brightness = led_val
                                self.servo_feedback_label.config(text=f"Last Servo Pos: {servo_val}")
                                self.led_feedback_label.config(text=f"Last LED Brightness: {led_val}") #If open command sent
                    except (ValueError, IndexError) as e:
                        print(f"Error processing preset feedback: {e}, Line:{line}")

        elif line.startswith("ERR:"):
            error_message = line[4:]
            self.log_message(f"Arduino Error: {error_message}")
            self.status_label.config(text=f"Arduino Error: {error_message}", fg="orange")
