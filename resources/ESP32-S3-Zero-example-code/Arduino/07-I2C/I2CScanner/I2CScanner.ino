/*
  I2C Scanner

  Description: Scans for I2C devices on the I2C bus and prints their addresses to the Serial Monitor.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/I2C-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/I2C-Communication
*/

#include <Wire.h>

#define SDA_PIN 1  // Any available GPIO can be used, just match the wiring
#define SCL_PIN 2  // Any available GPIO can be used, just match the wiring

void setup() {
  Serial.begin(9600);
  Wire.begin(SDA_PIN,SCL_PIN);
}

void loop() {
  byte error, address;
  int nDevices = 0;

  delay(5000);

  Serial.println("Scanning for I2C devices ...");
  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("I2C device found at address 0x%02X\n", address);
      nDevices++;
    } else if (error != 2) {
      Serial.printf("Error %d at address 0x%02X\n", error, address);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  }
}