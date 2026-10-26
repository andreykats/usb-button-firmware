/*
  Read Button State

  Reads the state of a button connected to a digital pin and prints the result to the Serial Monitor.

  Wiring:
  - ESP32-S3-Zero
  - Button -> GPIO 8
  
  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Digital-IO#digital-input
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Digital-IO#digital-input
*/

const int buttonPin = 8;  // Pin connected to the button

void setup() {
  Serial.begin(9600);                // Initialize serial communication at 9600 baud
  while (!Serial) {}                 // Wait for serial connection

  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with pull-up resistor
}

void loop() {
  int buttonState = digitalRead(buttonPin);  // Read current button state
  Serial.println(buttonState);               // Print button state to Serial Monitor
}