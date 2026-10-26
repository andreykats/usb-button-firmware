/*
  BLE Potentiometer Server

  Description: Creates a BLE Server that reads a potentiometer value and notifies connected clients
               when the value changes significantly.

  Wiring:
    - ESP32-S3-Zero
    - Potentiometer -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Bluetooth-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Bluetooth-Communication
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Global variables
BLEServer *pServer = NULL;
BLECharacteristic *pPotentiometerCharacteristic = NULL;
bool deviceConnected = false;

// Potentiometer-related definitions
const int potentiometerPin = 7;  // Potentiometer connected to GPIO 7
uint16_t lastPotValue;            // Stores the previous potentiometer reading (0-4095)

// Define UUIDs for the service and characteristic
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define POTENTIOMETER_CHARACTERISTIC_UUID "1b9a473a-4493-4536-8b2b-9d4133488256"

// Server callbacks to handle connection and disconnection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device Connected");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device Disconnected, restarting advertising...");
    // When a device disconnects, restart advertising immediately
    pServer->getAdvertising()->start();
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Potentiometer example...");

  // Initialize the potentiometer input
  lastPotValue = analogRead(potentiometerPin);

  // 1. Initialize BLE device
  BLEDevice::init("ESP32_Potentiometer");

  // 2. Create BLE server and set callbacks
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create BLE characteristic
  pPotentiometerCharacteristic = pService->createCharacteristic(
    POTENTIOMETER_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |    // Readable
      BLECharacteristic::PROPERTY_NOTIFY  // Notifiable (for subscriptions)
  );
  pPotentiometerCharacteristic->addDescriptor(new BLE2902());  // Add a 2902 descriptor to enable notifications

  // Set the initial value
  pPotentiometerCharacteristic->setValue(lastPotValue);

  // 5. Start the service
  pService->start();

  // 6. Start advertising
  pServer->getAdvertising()->start();
  Serial.println("BLE Server started, waiting for a client connection...");
}

void loop() {
  // Only check the potentiometer when a device is connected
  if (deviceConnected) {
    uint16_t currentPotValue = analogRead(potentiometerPin);

    // To prevent flooding with data due to analog signal jitter, only send when the value changes by a certain threshold
    if (abs(currentPotValue - lastPotValue) > 30) {
      lastPotValue = currentPotValue;

      // Update the characteristic's value and notify the client
      pPotentiometerCharacteristic->setValue(currentPotValue);
      pPotentiometerCharacteristic->notify();

      Serial.print("Potentiometer value changed to: ");
      Serial.println(currentPotValue);
    }

    delay(50);
  }
}