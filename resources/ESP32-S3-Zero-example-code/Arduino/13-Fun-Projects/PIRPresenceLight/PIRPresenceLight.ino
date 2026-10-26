/*
  PIR Presence Light

  Description: Turns on a WS2812 LED strip when motion is detected by a PIR sensor.
               The light automatically turns off if no motion is detected for a set duration.

  Wiring:
  - ESP32-S3-Zero
  - PIR Sensor VCC -> 5V
  - PIR Sensor GND -> GND
  - PIR Sensor -> GPIO 7
  - WS2812 VCC -> 5V
  - WS2812 GND -> GND
  - WS2812 DI -> GPIO 8

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-3-Presence-Light
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-3-Presence-Light
*/

#include <FastLED.h>

// --- Configuration Parameters ---
const int pirPin = 7;             // Pin connected to the PIR sensor
const int ledPin = 8;             // Pin connected to the LED strip
const int numLeds = 8;            // Number of LEDs in the strip
const unsigned long lightDuration = 5000; // Duration to keep the light on (milliseconds)

// Define the array of LEDs
CRGB leds[numLeds];

// --- State Variables ---
boolean isLightOn = false;        // Tracks whether the light is currently on
unsigned long lastMotionTime = 0; // Records the time of the last detected motion

void setup() {
  Serial.begin(115200);

  // Set PIR pin as pull-down input (keeps it LOW when not triggered)
  pinMode(pirPin, INPUT_PULLDOWN);

  // Initialize the LED strip
  FastLED.addLeds<WS2812, ledPin, GRB>(leds, numLeds);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();

  Serial.println("System started...");
  turnLightOff(); // Ensure the light is off at startup
}

void loop() {
  unsigned long currentTime = millis();
  int motionState = digitalRead(pirPin);

  // 1. If motion is detected, reset the timer
  if (motionState == HIGH) {
    lastMotionTime = currentTime;
    
    // If the light isn't on, turn it on
    if (!isLightOn) {
      Serial.println("Motion detected -> Turning light on");
      turnLightOn();
      isLightOn = true;
    }
  }
  // 2. If no motion, check for timeout
  else {
    if (isLightOn) {
      unsigned long duration = currentTime - lastMotionTime;

      // If the time since the last motion exceeds the set duration
      if (duration > lightDuration) {
        Serial.println("Timeout -> Turning light off");
        turnLightOff();
        isLightOn = false;
        delay(1000); // Brief delay to prevent flickering at the threshold
      }
    }
  }

  delay(100); // Simple loop delay
}

// Function to turn the light on
void turnLightOn() {
  fill_solid(leds, numLeds, CRGB::Purple);
  FastLED.show();
}

// Function to turn the light off
void turnLightOff() {
  fill_solid(leds, numLeds, CRGB::Black);
  FastLED.show();
}