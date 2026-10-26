/*
  OLED Progress Bar

  Description: Demonstrates how to draw a horizontal progress bar and a semi-circular gauge on a 128x128 OLED screen.
               It uses a potentiometer to control the progress interactively.
               This example requires the "Adafruit_SSD1327" library. Please search for and install the 'Adafruit SSD1327' 
               library in the Arduino IDE Library Manager.

  Wiring:
  - ESP32-S3-Zero
  - OLED VCC -> 5V
  - OLED GND -> GND
  - OLED SCK  -> GPIO 13
  - OLED MOSI -> GPIO 11
  - OLED CS   -> GPIO 10
  - OLED DC   -> GPIO 8
  - Potentiometer VCC -> 3.3V
  - Potentiometer GND -> GND
  - Potentiometer -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-5-Progress-Bar
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-5-Progress-Bar
*/

#include <Adafruit_SSD1327.h>

// SPI pin configuration
const int SCK_PIN = 13;
const int MOSI_PIN = 11;
const int CS_PIN = 10;
const int DC_PIN = 8;

// Potentiometer pin
const int POT_PIN = 7;

// Create display object (SPI)
// 128x128 resolution
Adafruit_SSD1327 display(128, 128, &SPI, DC_PIN, -1, CS_PIN);

// For I2C, use the constructor below (confirm the I2C address, usually 0x3D)
// const int SDA_PIN = 2;
// const int SCL_PIN = 1;
// Adafruit_SSD1327 display(128, 128, &Wire, -1); // -1 means no reset pin

void setup() {
  Serial.begin(115200);

  // Initialize potentiometer pin
  pinMode(POT_PIN, INPUT);

  // Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED (I2C)
  // if (!display.begin(0x3D)) {
  //   Serial.println("Unable to initialize OLED");
  //   while (1) yield();
  // }

  SPI.begin(SCK_PIN, -1, MOSI_PIN, CS_PIN);

  // Initialize OLED (SPI)
  if (!display.begin()) {
    Serial.println("Unable to initialize OLED");
    while (1) yield();
  }
  
  display.setTextSize(1);
  display.setTextColor(SSD1327_WHITE);
  // Adjust rotation as needed
  display.setRotation(0); 
}

void loop() {
  // Read data
  int val = getPercentage();

  // Select a display mode (uncomment one)

  // Mode A: Horizontal progress bar
  showHorizontalBar(val);

  // Mode B: Semi-circular gauge
  // showGauge(val);

  // Short delay to prevent rapid flickering
  delay(50);
}

int getPercentage() {
  // Read the potentiometer and return an integer from 0-100
  // ESP32 ADC is 12-bit by default (0-4095)
  int val = analogRead(POT_PIN); 
  int percent = map(val, 0, 4095, 0, 100);
  return constrain(percent, 0, 100);
}

// Effect 1: Horizontal progress bar
void showHorizontalBar(int percent) {
  // Clear the buffer
  display.clearDisplay(); 

  // Layout parameters
  int barX = 10;
  int barY = 55;
  int barWidth = 108;
  int barHeight = 18;

  // 1. Draw the outer frame
  // SSD1327 supports grayscale, but the GFX library's basic drawing uses monochrome logic.
  // We'll just use WHITE here.
  display.drawRect(barX, barY, barWidth, barHeight, SSD1327_WHITE);

  // 2. Draw the inner fill
  // Calculate fill width, leaving a 2-pixel margin
  int innerMaxWidth = barWidth - 4;
  int fillWidth = (int)((percent / 100.0) * innerMaxWidth);

  if (fillWidth > 0) {
    display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, SSD1327_WHITE);
  }

  // 3. Draw text information
  display.setCursor(32, 35);
  display.print("Progress");

  // Simple centering
  display.setCursor(50, 80); 
  display.print(percent);
  display.print("%");

  // Refresh the display
  display.display(); 
}

// Effect 2: Semi-circular gauge
void showGauge(int percent) {
  display.clearDisplay();

  // Gauge parameters
  int centerX = 64;
  int centerY = 105;
  int radius = 55;
  int pointerLen = 48;

  // 1. Draw tick marks (simulating a semi-circle)
  // Angle range: 180 degrees (left) -> 0 degrees (right)
  for (int i = 0; i <= 10; i++) {
    float angle = 180 - (i * 18);
    float rad = angle * PI / 180.0;

    // Outer circle point
    int x1 = centerX + (int)(cos(rad) * radius);
    int y1 = centerY - (int)(sin(rad) * radius);

    // Inner circle point (tick length 5)
    int x2 = centerX + (int)(cos(rad) * (radius - 6));
    int y2 = centerY - (int)(sin(rad) * (radius - 6));

    display.drawLine(x1, y1, x2, y2, SSD1327_WHITE);
  }

  // 2. Draw the pointer
  float currentAngle = 180 - (percent / 100.0 * 180);
  float currentRad = currentAngle * PI / 180.0;

  int needleX = centerX + (int)(cos(currentRad) * pointerLen);
  int needleY = centerY - (int)(sin(currentRad) * pointerLen);

  display.drawLine(centerX, centerY, needleX, needleY, SSD1327_WHITE);

  // 3. Draw center hub decoration
  display.fillRect(centerX - 2, centerY - 2, 5, 5, SSD1327_WHITE);

  // 4. Text
  display.setCursor(58, 110);
  display.print(percent);
  display.setCursor(50, 10);
  display.print("GAUGE");

  display.display();
}