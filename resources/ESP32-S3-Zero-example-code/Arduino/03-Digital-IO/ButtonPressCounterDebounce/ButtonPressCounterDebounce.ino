/*
  Button Press Counter Debounce

  Counts button presses with software debounce to ensure accurate counting.

  Wiring:
  - ESP32-S3-Zero
  - Button -> GPIO 8
  
  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Digital-IO#digital-input
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Digital-IO#digital-input
*/

const int buttonPin = 8;

int lastButtonState = 1;  // Previous button state
int currentButtonState;   // Current button state
int count = 0;            // Press counter

void setup() {
  Serial.begin(9600);                // Initialize serial communication
  while (!Serial) {}                 // Wait for serial connection

  pinMode(buttonPin, INPUT_PULLUP);  // Configure button pin with internal pull-up resistor
}

void loop() {
  currentButtonState = digitalRead(buttonPin);  // Read current button state

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Button pressed

  } else if (lastButtonState == LOW && currentButtonState == HIGH) {
    // Button released
    count = count + 1;      // Increment counter
    Serial.println(count);  // Print current count to serial monitor
    delay(100);             // Debounce delay (100ms)
  }

  lastButtonState = currentButtonState;  // Update previous state
}