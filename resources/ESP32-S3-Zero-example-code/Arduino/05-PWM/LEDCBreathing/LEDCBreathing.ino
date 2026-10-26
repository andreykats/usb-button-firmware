/*
  LEDC Breathing LED

  Description: Creates a breathing LED effect using ESP32 LEDC API.

  Wiring:
  - ESP32-S3-Zero
  - LED -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/PWM
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/PWM
*/

const int ledPin = 7;  // GPIO pin for the LED

const int frequency = 5000;  // PWM frequency (5kHz)
const int resolution = 8;    // PWM resolution (8-bit, 0-255)
const int ledChannel = 0;    // LEDC channel

void setup() {
  // Setup LEDC
  ledcAttach(ledPin, frequency, resolution);

  // To specify a channel, use:
  // ledcAttachChannel(ledPin, frequency, resolution, ledChannel);
}

void loop() {
  // Fade in
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(ledPin, dutyCycle);  // Set duty cycle
    delay(10);                     // Delay to control fade speed
  }

  // Fade out
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    ledcWrite(ledPin, dutyCycle);  // Set duty cycle
    delay(10);                     // Delay to control fade speed
  }
}
