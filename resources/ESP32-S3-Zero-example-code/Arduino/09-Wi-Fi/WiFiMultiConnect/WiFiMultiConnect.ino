/*
  Wi-Fi Multi Connect

  Description: Demonstrates how to use the WiFiMulti library to connect to the strongest available Wi-Fi network
               from a list of multiple credentials. It attempts to connect and prints the IP address upon success.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
*/

#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;

void setup() {
  Serial.begin(115200);
  delay(10);

  // Add multiple Wi-Fi networks to connect to
  wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  // Check the connection status and try to reconnect if disconnected
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    delay(1000);
  }
}