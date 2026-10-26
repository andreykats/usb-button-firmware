
/*
  Wi-Fi Scan

  Description: Demonstrates how to scan for available Wi-Fi networks.
               It prints the SSID, RSSI, channel, and encryption type of each found network to the Serial Monitor.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/WiFi-Networking-Basic
*/

#include "WiFi.h"

void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from any previous networks
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");
}

void loop() {
  Serial.println("Scan start");
  // WiFi.scanNetworks() returns the number of networks found.
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");

  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i) {
      // Print the SSID, RSSI, channel and encryption type for each network
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
      Serial.print(" | ");
      Serial.printf("%4ld", WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2ld", WiFi.channel(i));
      Serial.print(" | ");
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN: Serial.print("open"); break;
        case WIFI_AUTH_WEP: Serial.print("WEP"); break;
        case WIFI_AUTH_WPA_PSK: Serial.print("WPA"); break;
        case WIFI_AUTH_WPA2_PSK: Serial.print("WPA2"); break;
        case WIFI_AUTH_WPA_WPA2_PSK: Serial.print("WPA+WPA2"); break;
        case WIFI_AUTH_WPA2_ENTERPRISE: Serial.print("WPA2-EAP"); break;
        case WIFI_AUTH_WPA3_PSK: Serial.print("WPA3"); break;
        case WIFI_AUTH_WPA2_WPA3_PSK: Serial.print("WPA2+WPA3"); break;
        case WIFI_AUTH_WAPI_PSK: Serial.print("WAPI"); break;
        default: Serial.print("unknown");
      }
      Serial.println();
      delay(10);
    }
  }
  Serial.println("");

  // Delete the scan results to free memory
  WiFi.scanDelete();

  // Wait a moment before scanning again.
  delay(5000);
}