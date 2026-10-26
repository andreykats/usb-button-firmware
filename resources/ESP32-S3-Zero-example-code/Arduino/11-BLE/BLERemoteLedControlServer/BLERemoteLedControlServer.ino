/*
  BLE Remote LED Control Server

  Description: This sketch configures the ESP32 as a BLE Server (Peripheral).
               It accepts connections from a Client and controls an LED based on the brightness values received.

  Wiring:
    - ESP32-S3-Zero
    - LED -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Bluetooth-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Bluetooth-Communication
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// Global variables
BLEServer *pServer = NULL;
BLECharacteristic *pBrightnessCharacteristic = NULL;

bool deviceConnected = false;
bool newDataAvailable = false;
uint8_t brightness = 0;

// LED-related definitions
const int ledPin = 7; // LED connected to GPIO 7

// Define unique UUIDs for the service and characteristic
#define SERVICE_UUID "458063a1-02bf-4664-857e-16c1030be066"
#define BRIGHTNESS_CHARACTERISTIC_UUID "a5209632-66a9-411d-9353-9be5507790fa"

// Server callbacks to handle connection and disconnection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Client connected successfully");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected, restarting advertisement");
    // Restart advertising immediately so the client can reconnect
    pServer->getAdvertising()->start();
  }
};

// Characteristic callbacks to handle write requests from the client
class MyBrightnessCallbacks : public BLECharacteristicCallbacks {
  // This function is called when the client writes to this characteristic
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value_str = pCharacteristic->getValue();

    if (value_str.length() > 0) {
      // Get the brightness value from the received data
      brightness = value_str[0];
      newDataAvailable = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE LED Controller");

  // Set the pin to output mode
  pinMode(ledPin, OUTPUT);

  // 1. Initialize BLE device
  BLEDevice::init("ESP32_LED");

  // 2. Create BLE server and set callbacks
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create BLE characteristic
  pBrightnessCharacteristic = pService->createCharacteristic(
    BRIGHTNESS_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE // Write-only
  );

  // Set the write callback for the characteristic
  pBrightnessCharacteristic->setCallbacks(new MyBrightnessCallbacks());

  // 5. Start the service
  pService->start();
  Serial.println("BLE service started");

  // 6. Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pServer->getAdvertising()->start();

  Serial.println("Advertisement started, ready for connections");
}

void loop() {
  if (newDataAvailable) {
    // Reset the flag to prevent reprocessing
    newDataAvailable = false;

    Serial.print("Brightness received: ");
    Serial.println(brightness);

    // Set the LED brightness
    analogWrite(ledPin, brightness);
  }
}