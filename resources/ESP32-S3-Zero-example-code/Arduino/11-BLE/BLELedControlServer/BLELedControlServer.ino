/*
  BLE LED Control Server

  Description: Creates a BLE Server that controls an LED based on commands received from a client.
               It exposes a characteristic that can be written to control the LED state (1: ON, 0: OFF).

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
#include <BLE2902.h>

// Global variables
BLEServer *pServer = NULL;
BLECharacteristic *pLedCharacteristic = NULL;
bool deviceConnected = false;

// LED-related definitions
const int ledPin = 7;  // LED connected to GPIO 7
uint8_t ledState = 0;   // Stores the LED state (0: OFF, 1: ON)

// Define UUIDs for the service and characteristic. Use your own generated UUIDs to avoid conflicts.
#define SERVICE_UUID "48407a44-6e13-4d28-a559-210de862bc29"
#define LED_CHARACTERISTIC_UUID "539ca2ac-09e5-49be-90da-3b157549eac3"

// Server callbacks to handle connection and disconnection events.
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device Connected");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device Disconnected, restarting advertising...");
    // When a device disconnects, restart advertising immediately.
    pServer->getAdvertising()->start();
  }
};

// Characteristic callbacks to handle write requests from the client.
class MyLedCallbacks : public BLECharacteristicCallbacks {
  // This function is called when the client writes to this characteristic.
  void onWrite(BLECharacteristic *pCharacteristic) {
    // Get the data sent by the client.
    String value_str = pCharacteristic->getValue();

    if (value_str.length() > 0) {
      // We only care about the first byte.
      uint8_t command = value_str[0];

      Serial.print("Received command: ");
      Serial.println(command);

      // Control the LED based on the received command.
      if (command == 1) {
        Serial.println("Turning LED ON");
        digitalWrite(ledPin, HIGH);
        ledState = 1;
      } else if (command == 0) {
        Serial.println("Turning LED OFF");
        digitalWrite(ledPin, LOW);
        ledState = 0;
      } else {
        Serial.print("Unknown command: ");
        Serial.println(command);
      }

      // Update the characteristic value to reflect the current LED state, so the client gets the correct info on read.
      pLedCharacteristic->setValue(&ledState, 1);
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE LED Control example...");

  // Initialize the LED pin.
  pinMode(ledPin, OUTPUT);
  ledState = 0;

  // 1. Initialize BLE device.
  BLEDevice::init("ESP32_LED_Control");

  // 2. Create BLE server and set callbacks.
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create BLE service.
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create BLE characteristic.
  pLedCharacteristic = pService->createCharacteristic(
    LED_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |   // Readable
      BLECharacteristic::PROPERTY_WRITE  // Writable
  );

  // Set write callback for the characteristic.
  pLedCharacteristic->setCallbacks(new MyLedCallbacks());

  // Set the initial value of the characteristic (LED is initially OFF).
  pLedCharacteristic->setValue(&ledState, 1);

  // 5. Start the service.
  pService->start();

  // 6. Start advertising.
  pServer->getAdvertising()->start();
  Serial.println("BLE Server started, waiting for a client connection...");
}

void loop() {
  // The main loop can be empty because all BLE events are handled asynchronously via callbacks.
}