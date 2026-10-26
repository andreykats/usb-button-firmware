/*
  SSD1327 OLED SPI Demo

  Description: Demonstrates how to use the SSD1327 OLED display with SPI interface using specified pins. 
               This example requires the "Adafruit_SSD1327" library. Please search for and install the 'Adafruit SSD1327' 
               library in the Arduino IDE Library Manager.

  Wiring:
    - ESP32-S3-Zero
    - OLED VCC -> 3.3V
    - OLED GND -> GND
    - OLED SCK -> GPIO 13
    - OLED MOSI -> GPIO 11
    - OLED CS -> GPIO 10
    - OLED DC -> GPIO 8

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/SPI-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/SPI-Communication
*/

#include <SPI.h>
#include <Adafruit_SSD1327.h>

#define OLED_SCK 13   // Clock pin
#define OLED_MOSI 11  // Data out (MOSI) pin
#define OLED_CS 10    // Chip select pin

#define OLED_DC 8      // Data/Command select pin
#define OLED_RESET -1  // Reset pin (unused)

// Create display object
Adafruit_SSD1327 display(128, 128, &SPI, OLED_DC, OLED_RESET, OLED_CS);

void setup() {

  Serial.begin(9600);

  // Initialize SPI bus
  SPI.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);

  Serial.println("SSD1327 OLED test");

  // Initialize display
  if (!display.begin()) {
    Serial.println("Unable to initialize OLED");
    while (1) yield();
  }

  // Display settings
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
  display.setCursor(15, 60);
  display.setTextColor(SSD1327_WHITE);
  display.println("SPI TEST");
  display.display();

  delay(1000);
}

void loop() {
}