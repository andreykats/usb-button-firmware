/*
  WiFi Weather Display (Seniverse)

  Description: Connects to Wi-Fi, fetches weather data (JSON) from Seniverse API via HTTP, 
               and displays the weather information on an SSD1327 OLED screen.
               This example requires the "Adafruit_SSD1327" library. Please search for and install the 'Adafruit SSD1327' 
               library in the Arduino IDE Library Manager.
  
  API Provider: https://www.seniverse.com/ Seniverse

  Wiring:
  - ESP32-S3-Zero
  - OLED VCC -> 3.3V
  - OLED GND -> GND
  - OLED SCK  -> GPIO 13
  - OLED MOSI -> GPIO 11
  - OLED CS   -> GPIO 10
  - OLED DC   -> GPIO 8

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-6-Weather-Display
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-6-Weather-Display
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>

// Wi-Fi configuration (replace with your credentials)
const char* ssid = "Maker";
const char* password = "12345678";

// Seniverse Weather API configuration (replace with your private key)
String apiKey = "your_api_key";
// The city for which you want to check the weather
String location = "shenzhen"; 

// API URL template
const String apiUrlTemplate = "https://api.seniverse.com/v3/weather/now.json?key=%s&location=%s&language=en&unit=c";

// Update interval: 30 minutes (in milliseconds)
const unsigned long updateInterval = 1800000; 
unsigned long lastUpdateTime = 0;

// SPI pin configuration
const int SCK_PIN = 13;
const int MOSI_PIN = 11;
const int CS_PIN = 10;
const int DC_PIN = 8;

// Initialize OLED (SPI)
// 128x128 resolution
Adafruit_SSD1327 display(128, 128, &SPI, DC_PIN, -1, CS_PIN);

// For I2C, use the constructor below (confirm the I2C address, usually 0x3D)
// const int SDA_PIN = 2;
// const int SCL_PIN = 1;
// Adafruit_SSD1327 display(128, 128, &Wire, -1); // -1 means no reset pin

void setup() {
  Serial.begin(115200);

  // Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED (I2C)
  // if (!display.begin(0x3D)) {
  //   Serial.println("Unable to initialize OLED");
  //   while (true) yield();
  // }

  SPI.begin(SCK_PIN, -1, MOSI_PIN, CS_PIN);

  // Initialize OLED (SPI)
  if (!display.begin()) {
    Serial.println("Unable to initialize OLED");
    while (true) yield();
  }
  
  // Set text size and color
  display.setTextSize(1);
  display.setTextColor(SSD1327_WHITE);
  display.clearDisplay();
  display.display();

  connectWiFi();
  
  // Get initial weather data
  getWeather();
  lastUpdateTime = millis();
}

void loop() {
  // Update periodically
  if (millis() - lastUpdateTime >= updateInterval) {
    getWeather();
    lastUpdateTime = millis();
  }
}

void connectWiFi() {
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  display.clearDisplay();
  display.setCursor(5, 20);
  display.print("Connecting to");
  display.setCursor(5, 40);
  display.print("WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(5, 20);
  display.print("WiFi Connected!");
  display.setCursor(5, 40);
  display.print("IP:");
  display.setCursor(5, 55);
  display.print(WiFi.localIP());
  display.display();
  delay(2000);
}

void displayWeather(String city, String weather, String temperature) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1327_WHITE);

  // City name
  display.setCursor(5, 10);
  display.print("City: ");
  display.println(city);

  // Weather condition
  display.setCursor(5, 40);
  display.println("Weather:");
  display.setCursor(5, 55);
  display.println(weather);

  // Temperature
  display.setCursor(5, 85);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.display();
  Serial.printf("Display updated: %s, %s, %s C\n", city.c_str(), weather.c_str(), temperature.c_str());
}

void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Build the full request URL
    char url[200];
    sprintf(url, apiUrlTemplate.c_str(), apiKey.c_str(), location.c_str());
    
    Serial.print("Fetching weather from: ");
    Serial.println(url);

    display.clearDisplay();
    display.setCursor(5, 20);
    display.print("Fetching...");
    display.display();

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("API Response:");
        Serial.println(payload);

        // Parse the JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          JsonObject result = doc["results"][0];
          String locationName = result["location"]["name"].as<String>();
          String weatherText = result["now"]["text"].as<String>();
          String temperature = result["now"]["temperature"].as<String>();

          displayWeather(locationName, weatherText, temperature);
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          displayWeather("Error", "JSON Fail", "");
        }
      } else {
        Serial.println("API Error: " + http.getString()); 
      }
    } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      displayWeather("Error", "HTTP Fail", "");
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
    // Try to reconnect
    connectWiFi(); 
  }
}