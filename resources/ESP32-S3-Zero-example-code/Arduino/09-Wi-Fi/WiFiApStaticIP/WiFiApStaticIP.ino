/*
  Wi-Fi Access Point with Static IP

  Description: Demonstrates how to configure the ESP32-S3 as a soft access point (AP) with a static IP address.
               It sets up a custom IP, gateway, and subnet mask before starting the AP.
               It also monitors station connections and disconnections.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
*/

#include <WiFi.h>

const char *ssid = "ESP32S3-TEST";  // SSID for the access point
const char *password = "12345678";  // Password for the access point (at least 8 characters)

IPAddress ip(192, 168, 5, 1);        // Set a static IP address
IPAddress gateway(192, 168, 5, 1);   // Set the gateway
IPAddress subnet(255, 255, 255, 0);  // Set the subnet mask

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // Set Wi-Fi event handlers
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(WiFiStationGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  // Configure the soft AP with a static IP before starting it
  WiFi.softAPConfig(ip, gateway, subnet);

  // Create the Wi-Fi soft Access Point
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed.");
    while (1)
      ;
  }

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
}

// Event: a station connects to the AP
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("Device connected, MAC: ");
  Serial.println(macToString(info.wifi_ap_staconnected.mac));
}

// Event: a station gets an IP address
void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("Device IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}

// Event: a station disconnects from the AP
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("Device disconnected, MAC: ");
  Serial.println(macToString(info.wifi_ap_stadisconnected.mac));
}

// Helper function to convert MAC address to a string
String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}