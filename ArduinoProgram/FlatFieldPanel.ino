void applyPreset(int presetNumber) {
    int newServoPos;
    switch (presetNumber) {
        case 1: //  Not Used
            return; // Do nothing
        case 2:  // "Medium" (Not currently used, but kept for potential future use)
            newServoPos = 90;
            ledBrightness = 150;
             moveServo(servoPos, newServoPos);
            servoPos = newServoPos;
            analogWrite(ledPin, ledBrightness);
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:2:SERVO:");  // Give specific feedback
            Serial.print(servoPos);
            Serial.print(":LED:");
            Serial.println(ledBrightness);
            break;

        case 3: // "Close"
            newServoPos = 180;  // Changed to 180 degrees
            ledBrightness = 0; // Keep LED off as requested
            moveServo(servoPos, newServoPos);
            servoPos = newServoPos;
            analogWrite(ledPin, ledBrightness); // Keep LED off
            EEPROM.write(servoPosAddress, servoPos);
            EEPROM.write(ledBrightnessAddress, ledBrightness);
            Serial.print("OK:PRESET:3:SERVO:180:LED:0"); //Consistent feedback
            Serial.println();
            break;
        default:
            Serial.println("ERR:Invalid preset number");
            return;
    }
}
