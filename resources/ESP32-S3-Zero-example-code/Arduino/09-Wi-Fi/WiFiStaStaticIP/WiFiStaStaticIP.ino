/*
  Wi-Fi Station with Static IP

  Description: Demonstrates how to connect the ESP32-S3 to a Wi-Fi network with a static IP address.
               It configures a static IP, gateway, and subnet before connecting to the network.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
*/

#include <WiFi.h>

const char *ssid = "Maker";         // Wi-Fi SSID
const char *password = "12345678";  // Wi-Fi password

// Set the static IP address, gateway, and subnet mask
// Adjust these settings to match your local network
IPAddress ip(192, 168, 137, 100);     // Static IP address
IPAddress gateway(192, 168, 137, 1);  // Gateway
IPAddress subnet(255, 255, 255, 0);   // Subnet mask

void setup() {
  Serial.begin(115200);

  delay(10);

  WiFi.mode(WIFI_STA);

  // Configure the static IP before connecting. This must be called before WiFi.begin().
  WiFi.config(ip, gateway, subnet);

  Serial.print("Connecting to ");
  Serial.println(ssid);

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