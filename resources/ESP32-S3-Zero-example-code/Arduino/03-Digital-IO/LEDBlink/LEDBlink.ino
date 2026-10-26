/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Wiring:
  - ESP32-S3-Zero
  - LED -> GPIO 7
  
  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Digital-IO#digital-output
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Digital-IO#digital-output
*/

const int ledPin = 7;  // Pin connected to the LED

// Runs once when the board powers up or resets
void setup() {
  // Set pin as output
  pinMode(ledPin, OUTPUT);
}

// Runs repeatedly in an infinite loop
void loop() {
  digitalWrite(ledPin, HIGH);  // Turn LED on (HIGH = voltage applied)
  delay(1000);                 // Wait one second
  digitalWrite(ledPin, LOW);   // Turn LED off
  delay(1000);                 // Wait one second
}