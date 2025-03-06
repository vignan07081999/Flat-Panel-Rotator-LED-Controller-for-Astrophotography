#!/usr/bin/env python3

import serial
import sys
import time
import argparse  # For parsing command-line arguments

def send_command(port, command, baudrate=9600, timeout=1):
    """Sends a command to the Arduino and waits for a response."""
    try:
        with serial.Serial(port, baudrate, timeout=timeout) as ser:
            ser.write((command + '\n').encode('utf-8'))
            time.sleep(0.1)  # Give Arduino time to process
            response = ser.readline().decode('utf-8').strip()
            return response
    except serial.SerialException as e:
        print(f"Serial communication error: {e}")
        return None

def main():
    parser = argparse.ArgumentParser(description="Control flat panel from KStars.")
    parser.add_argument("port", help="Serial port of the Arduino (e.g., /dev/ttyACM0)")
    parser.add_argument("action", help="Action to perform (servo, led, preset)")
    parser.add_argument("value", help="Value for the action", nargs='?', default=None)

    args = parser.parse_args()

    if args.action == "servo":
        if args.value is None:
            print("Error: Servo position required.")
            sys.exit(1)
        response = send_command(args.port, f"SERVO:{args.value}")

    elif args.action == "led":
        if args.value is None:
            print("Error: LED brightness required.")
            sys.exit(1)
        response = send_command(args.port, f"LED:{args.value}")

    elif args.action == "preset":
        if args.value is None:
            print("Error: Preset number required.")
            sys.exit(1)
        response = send_command(args.port, f"PRESET:{args.value}")
    elif args.action == "get":
          if args.value is None:
            print("Error: GET request value.")
            sys.exit(1)
          response = send_command(args.port, f"GET:{args.value}")

    else:
        print(f"Error: Unknown action '{args.action}'")
        sys.exit(1)

    if response:
        print(f"Arduino response: {response}")  # Output the response for KStars
    else:
        print("Error: No response from Arduino.")

if __name__ == "__main__":
    main()
