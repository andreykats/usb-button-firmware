/*
  Web Server Station LED Control

  Description: Connects to an existing Wi-Fi network and hosts a web page to control an external LED connected to GPIO 7.

  Wiring:
  - ESP32-S3-Zero
  - LED -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Web-Server
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Web-Server
*/

#include <WiFi.h>
#include <WebServer.h>

const int ledPin = 7;

const char *ssid = "Maker";         // Replace with your Wi-Fi SSID
const char *password = "12345678";  // Replace with your Wi-Fi password

WebServer server(80);

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  // Connect to the Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Wait for the connection to be established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure routes and start the server
  server.on("/", handleRoot);
  server.on("/ledon", handleLedOn);
  server.on("/ledoff", handleLedOff);
  server.begin();
}

void loop() {
  server.handleClient();
}

// Handler for the root path
void handleRoot() {
  server.send(200, "text/html", generateHTML(digitalRead(ledPin)));
}

// Handler to turn the LED on
void handleLedOn() {
  digitalWrite(ledPin, HIGH);
  server.send(200, "text/html", generateHTML(true));
}

// Handler to turn the LED off
void handleLedOff() {
  digitalWrite(ledPin, LOW);
  server.send(200, "text/html", generateHTML(false));
}

// Generate the HTML page content
String generateHTML(bool ledState) {
  String htmlContent = "<!DOCTYPE html> <html>\n";
  htmlContent += "<head><meta charset=\"utf-8\" name=\"viewport\" content=\"width=device-width\">\n";
  htmlContent += "<title>ESP32S3 Test</title>\n";
  htmlContent += "</head><body>\n";

  htmlContent += "<h1>Hello World!</h1>\n";

  if (ledState) {
    htmlContent += "<p>LED Status: ON</p>";
    htmlContent += "<a href=\"/ledoff\">Turn off the LED</a>\n";
  } else {
    htmlContent += "<p>LED Status: OFF</p>";
    htmlContent += "<a href=\"/ledon\">Turn on the LED</a>\n";
  }

  htmlContent += "</body>\n";
  htmlContent += "</html>\n";
  return htmlContent;
}