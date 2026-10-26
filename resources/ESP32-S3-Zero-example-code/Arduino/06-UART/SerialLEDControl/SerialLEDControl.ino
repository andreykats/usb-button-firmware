/*
  Serial LED Control

  Controls an LED via commands entered in Serial Monitor ('on'/'off').

  Wiring:
  - ESP32-S3-Zero
  - LED -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/UART-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/UART-Communication
*/

const int ledPin = 7;  // Pin connected to the LED

void setup() {
  pinMode(ledPin, OUTPUT);  // Set LED pin to output mode

  Serial.begin(115200);  // Initialize serial communication at 115200 baud
  while (!Serial) {};    // Wait for serial port to connect
}

void loop() {
  if (Serial.available() > 0) {                 // If serial data is available
    String msg = Serial.readStringUntil('\n');  // Read incoming string until newline
    msg.trim();                                 // Trim leading/trailing whitespace

    if (msg == "on") {
      // If the command is "on"
      digitalWrite(ledPin, HIGH);
      Serial.println("LED turned on");
    } else if (msg == "off") {
      // If the command is "off"
      digitalWrite(ledPin, LOW);
      Serial.println("LED turned off");
    } else {
      Serial.println("Invalid command. Please enter 'on' or 'off'.");  // Invalid command
    }
  }
}