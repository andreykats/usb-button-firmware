/*
  Potentiometer LED Strip

  Description: Reads analog values from a potentiometer and maps them to a WS2812 LED strip.
               As the value increases, the LEDs light up in sequence (Yellow -> Green -> Red).

  Wiring:
  - ESP32-S3-Zero
  - Potentiometer VCC -> 3.3V
  - Potentiometer GND -> GND
  - Potentiometer -> GPIO 7
  - WS2812 VCC -> 5V
  - WS2812 GND -> GND
  - WS2812 DI -> GPIO 8

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-4-LED-Strip
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-4-LED-Strip
*/

#include <FastLED.h>

// --- Constants ---
const int potPin = 7;       // Potentiometer pin
const int ledPin = 8;       // WS2812 data pin
const int numLeds = 8;      // Number of LEDs
const int brightness = 50;  // Brightness

// Define the array for the LEDs
CRGB leds[numLeds];

void setup() {
  Serial.begin(115200);
  Serial.println("Potentiometer controlled WS2812");

  // Initialize potentiometer pin
  pinMode(potPin, INPUT);

  // Initialize FastLED
  FastLED.addLeds<WS2812B, ledPin, GRB>(leds, numLeds).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness); 
  FastLED.clear(); 
  FastLED.show();
}

void loop() {
  // Read the analog value from the potentiometer
  int analogValue = analogRead(potPin);

  // Update the LEDs
  updateLeds(analogValue);

  // Short delay to prevent flickering
  delay(50);
}

// Helper function placed after loop()
void updateLeds(int analogValue) {
  // Map the 0-4095 range to 0-24 (3 stages * 8 LEDs)
  int totalSteps = 3 * numLeds;
  
  // Use the map function for mapping
  long position = map(analogValue, 0, 4095, 0, totalSteps);

  // Constrain the maximum value to prevent overflow
  if (position > totalSteps) {
    position = totalSteps;
  }

  // Iterate through all LEDs
  for (int i = 0; i < numLeds; i++) {
    // Logic check: priority from high to low (Red -> Green -> Yellow)

    // Part 3: Red override (when progress exceeds 16 + LED index)
    if (position > (2 * numLeds + i)) {
      leds[i] = CRGB::Red;
    }
    // Part 2: Green override (when progress exceeds 8 + LED index)
    else if (position > (1 * numLeds + i)) {
      leds[i] = CRGB::Green;
    }
    // Part 1: Yellow light-up (when progress exceeds the LED index)
    else if (position > i) {
      leds[i] = CRGB::Yellow;
    }
    // Otherwise: turn off
    else {
      leds[i] = CRGB::Black;
    }
  }

  // Write data to the LED strip
  FastLED.show();
}