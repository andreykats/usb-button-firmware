/*
  Button Toggle LED

  Toggles the LED state (on/off) each time the button is pressed.

  Wiring:
  - ESP32-S3-Zero
  - LED -> GPIO 7
  - Button -> GPIO 8
  
  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Digital-IO#digital-io-exercise
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Digital-IO#digital-io-exercise
*/

const int ledPin = 7;     // LED pin
const int buttonPin = 8;  // Button pin

int lastButtonState = HIGH;  // Previous button state
int ledState = LOW;          // Current LED state (LOW=off, HIGH=on)
int currentButtonState;      // Current button state

void setup() {
  pinMode(ledPin, OUTPUT);           // Set LED pin as output
  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with pull-up
}

void loop() {
  currentButtonState = digitalRead(buttonPin);  // Read current button state

  // Detect button release (transition from pressed to released)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    ledState = !ledState;            // Toggle LED state
    digitalWrite(ledPin, ledState);  // Apply new LED state
    delay(100);                      // Debounce delay
  }

  lastButtonState = currentButtonState;  // Save current state for next loop
}