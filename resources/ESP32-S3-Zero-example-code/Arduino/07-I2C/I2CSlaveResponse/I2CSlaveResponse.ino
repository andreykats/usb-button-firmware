/*
  I2C Slave Response

  Description: Responds to data requests from the I2C master device; this code is intended to be used with "I2cMasterRequest.ino".

  Wiring:
    [Slave Device]               [Master Device]
    ESP32-S3-Zero                 ESP32-S3-Zero
    SDA (GPIO 12)      <------>   SDA (GPIO 1)
    SCL (GPIO 11)      <------>   SCL (GPIO 2)
    GND                <------>   GND

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/I2C-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/I2C-Communication
*/

#include <Wire.h>

#define SDA 12  // SDA pin
#define SCL 11  // SCL pin

void setup() {
  Wire.begin(8, SDA, SCL, 100000);  // Initialize I2C slave at address 8, 100kHz
  Wire.onRequest(requestEvent);     // Register event handler for data requests
}

void loop() {
  delay(100);
}

// Callback function for when master requests data
void requestEvent() {
  Wire.write((const uint8_t*)"hello", 5);  // Send 5 bytes ("hello") to the master
}