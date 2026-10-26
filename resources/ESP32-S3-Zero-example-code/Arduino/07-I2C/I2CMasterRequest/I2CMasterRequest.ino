/*
  I2C Master Request

  Description: Requests data from the I2C slave device; this code is intended to be used with "I2cSlaveResponse.ino".

  Wiring:
    [Master Device]              [Slave Device]
    ESP32-S3-Zero                 ESP32-S3-Zero
    SDA (GPIO 1)       <------>   SDA (GPIO 12)
    SCL (GPIO 2)       <------>   SCL (GPIO 11)
    GND                <------>   GND

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/I2C-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/I2C-Communication
*/

#include <Wire.h>

#define SDA 1  // SDA pin
#define SCL 2  // SCL pin

void setup() {
  Serial.begin(9600);

  Wire.begin(SDA, SCL, 100000);  // Initialize I2C master at 100kHz
}

void loop() {
  // Request 5 bytes from slave at address 8. `requestFrom` returns the number of bytes received.
  int dataLength = Wire.requestFrom(8, 5);

  Serial.print("Received " + String(dataLength) + " bytes: "); // Print the number of bytes received

  while (Wire.available()) {  // Read and print each byte received
    char c = Wire.read();
    Serial.print(c);
  }

  Serial.println();
  delay(500);
}