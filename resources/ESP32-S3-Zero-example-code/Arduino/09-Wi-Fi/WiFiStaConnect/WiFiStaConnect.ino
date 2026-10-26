/*
  Wi-Fi Station Connect

  Description: Demonstrates how to connect the ESP32-S3 to an existing Wi-Fi network (Station mode).
               It connects using a provided SSID and password, and prints the assigned IP address upon success.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
*/

#include <WiFi.h>

// Replace with your network credentials
const char *ssid = "yourssid";        // Replace with your Wi-Fi SSID
const char *password = "yourpasswd";  // Replace with your Wi-Fi password

void setup() {
  Serial.begin(115200);

  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
}