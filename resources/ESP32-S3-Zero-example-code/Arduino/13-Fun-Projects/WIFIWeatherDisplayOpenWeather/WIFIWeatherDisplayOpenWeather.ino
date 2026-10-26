/*
  WiFi Weather Display (OpenWeather)

  Description: Connects to Wi-Fi, fetches weather data (JSON) from OpenWeather API via HTTP,
               and displays the weather information on an SSD1327 OLED screen.
               This example requires the "Adafruit_SSD1327" library. Please search for and install the 'Adafruit SSD1327'
               library in the Arduino IDE Library Manager.

  API Provider: https://openweathermap.org/ OpenWeather

  Wiring:
  - ESP32-S3-Zero
  - OLED VCC  -> 3.3V
  - OLED GND  -> GND
  - OLED SCK  -> GPIO 13
  - OLED MOSI -> GPIO 11
  - OLED CS   -> GPIO 10
  - OLED DC   -> GPIO 8

  Link:
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-6-Weather-Display
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-6-Weather-Display
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>

// Wi-Fi Configuration (Please replace with your Wi-Fi credentials)
const char* ssid = "Maker";
const char* password = "12345678";

// OpenWeatherMap API Configuration (Replace with your API Key)
String apiKey = "your_api_key";

// City to query (e.g., "Shenzhen", "New%20York,US", "London", "Stockholm")
String location = "Stockholm";

// API URL Template
// Get your key here: https://home.openweathermap.org/api_keys
// Parameters: appid (Key), q (City), units (metric = Celsius)
const String apiUrlTemplate = "http://api.openweathermap.org/data/2.5/weather?appid=%s&q=%s&units=metric";

// Update interval: 30 minutes (in milliseconds)
const unsigned long updateInterval = 1800000;
unsigned long lastUpdateTime = 0;

// SPI Pin Configuration
const int SCK_PIN = 13;
const int MOSI_PIN = 11;
const int CS_PIN = 10;
const int DC_PIN = 8;

// Initialize OLED (SPI)
// 128x128 resolution
Adafruit_SSD1327 display(128, 128, &SPI, DC_PIN, -1, CS_PIN);

// If using I2C, please use the following constructor (I2C address needs to be confirmed, usually 0x3D)
// const int SDA_PIN = 2;
// const int SCL_PIN = 1;
// Adafruit_SSD1327 display(128, 128, &Wire, -1); // -1 indicates no reset pin

void setup() {
  Serial.begin(115200);

  // Wire.begin(SDA_PIN, SCL_PIN);  // Initialize OLED (I2C)
  // if (!display.begin(0x3D)) {
  //   Serial.println("Unable to initialize OLED");
  //   while (true) yield();
  // }

  SPI.begin(SCK_PIN, -1, MOSI_PIN, CS_PIN);

  // Initialize OLED
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

  // Initial weather fetch
  getWeather();
  lastUpdateTime = millis();
}

void loop() {
  // Timed updates
  if (millis() - lastUpdateTime >= updateInterval) {
    getWeather();
    lastUpdateTime = millis();
  }
}

void connectWiFi() {
  // Connect to Wi-Fi
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

void displayError(String message, String detail) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1327_WHITE);
  display.setCursor(5, 20);
  display.print("Error: ");
  display.println(message);
  display.setCursor(5, 50);
  display.print("Detail: ");
  display.println(detail);
  display.display();
  Serial.printf("Error displayed: %s, %s\n", message.c_str(), detail.c_str());
}

void displayWeather(String city, String weather, String temperature, String humidity) {
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
  display.setCursor(5, 80);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  // Humidity
  display.setCursor(5, 100);
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  display.display();
  Serial.printf("Display updated: %s, %s, %s C, %s%%\n", city.c_str(), weather.c_str(), temperature.c_str(), humidity.c_str());
}

void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Build the complete request URL
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

        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          // OpenWeatherMap JSON structure
          String locationName = doc["name"].as<String>();
          String weatherText = doc["weather"][0]["main"].as<String>();
          float tempVal = doc["main"]["temp"].as<float>();
          int humVal = doc["main"]["humidity"].as<int>();

          // Format temperature to 1 decimal place
          String temperature = String(tempVal, 1);
          String humidity = String(humVal);

          displayWeather(locationName, weatherText, temperature, humidity);
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          displayWeather("Error", "JSON Fail", "", "");
          displayError("JSON Fail", error.c_str());
        }
      } else {
        Serial.println("API Error: " + http.getString());
        displayError("API Error", String(httpCode));
      }
    } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      displayError("HTTP Fail", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
    // Attempt to reconnect
    connectWiFi();
  }
}
