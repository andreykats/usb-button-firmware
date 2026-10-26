/*
  PWM Breathing LED

  Description: Creates a breathing LED effect using PWM.

  Wiring:
  - ESP32-S3-Zero
  - LED -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/PWM
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/PWM
*/

const int ledPin = 7;  // Pin connected to the LED

void setup() {
}

void loop() {
  // Fade in (duty cycle from 0 to 255)
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    analogWrite(ledPin, dutyCycle);  // Set PWM duty cycle
    delay(10);                       // Control fade speed
  }
  // Fade out (duty cycle from 255 to 0)
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    analogWrite(ledPin, dutyCycle);  // Set PWM duty cycle
    delay(10);                       // Control fade speed
  }
}