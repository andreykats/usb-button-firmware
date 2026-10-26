/*
  Web Server Station Basic

  Description: Connects to Wi-Fi in Station mode and serves a simple "Hello from ESP32!" message on the root path.

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Web-Server
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Web-Server
*/

#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "Maker";         // Replace with your Wi-Fi SSID
const char *password = "12345678";  // Replace with your Wi-Fi password

WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

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
  server.begin();
}

void loop() {
  server.handleClient();
}

// Handler for the root path
void handleRoot() {
  server.send(200, "text/html", generateHTML());
}

// Generate the HTML page content
String generateHTML() {
  String htmlContent = "<!DOCTYPE html> <html>\n";
  htmlContent += "<head><meta charset=\"utf-8\" name=\"viewport\" content=\"width=device-width\">\n";
  htmlContent += "<title>ESP32S3 Test</title>\n";
  htmlContent += "</head><body>\n";
  htmlContent += "<h1>Hello World!</h1>\n";
  htmlContent += "<p>Hello from ESP32</p>\n";
  htmlContent += "</body>\n";
  htmlContent += "</html>\n";
  return htmlContent;
}