/*
  Button Control LED

  Turns an LED on when the button is pressed, and off when it is released.

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
int buttonState;          // Button state variable

void setup() {
  pinMode(ledPin, OUTPUT);           // Set LED pin as output mode
  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as pull-up input mode
}

void loop() {
  buttonState = digitalRead(buttonPin);  // Read button state

  if (buttonState == LOW) {      // When button is pressed (LOW)
    digitalWrite(ledPin, HIGH);  // Turn on LED
  } else {                       // When button is not pressed (HIGH)
    digitalWrite(ledPin, LOW);   // Turn off LED
  }
}