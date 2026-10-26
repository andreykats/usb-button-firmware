/*
  SSD1327 OLED I2C Demo

  Description: Demonstrates how to use the SSD1327 OLED display with I2C interface using specified pins. 
               This example requires the "Adafruit_SSD1327" library. Please search for and install the 'Adafruit SSD1327' 
               library in the Arduino IDE Library Manager.

  Wiring:
    - ESP32-S3-Zero
    - OLED VCC -> 3.3V
    - OLED GND -> GND
    - OLED SDA -> GPIO 1
    - OLED SCL -> GPIO 2

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/I2C-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/I2C-Communication
*/

#include <Adafruit_SSD1327.h>  // Adafruit SSD1327 library for the OLED display
#include <Wire.h>              // I2C communication library

// I2C pin definitions
#define SDA_PIN 1      // Data pin
#define SCL_PIN 2      // Clock pin
#define OLED_RESET -1  // Reset pin

// Create the display object
Adafruit_SSD1327 display(128, 128, &Wire, OLED_RESET);

void setup() {
  // Initialize I2C bus
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.begin(9600);
  Serial.println("SSD1327 OLED test");

  // Initialize the display with I2C address 0x3D
  if (!display.begin(0x3D)) {
    Serial.println("Unable to initialize OLED");
    while (1) yield();
  }

  // Configure display settings
  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(2);
  display.setTextColor(SSD1327_WHITE);

  // Display text
  display.setCursor(10, 10);
  display.println("Hello,");
  display.setCursor(40, 30);
  display.setTextColor(SSD1327_BLACK, SSD1327_WHITE);
  display.println(" World!");
  display.display();

  delay(1000);
}

void loop() {
}