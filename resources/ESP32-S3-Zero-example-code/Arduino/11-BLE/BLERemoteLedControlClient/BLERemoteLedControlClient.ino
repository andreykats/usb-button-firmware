/*
  BLE Remote LED Control Client

  Description: This sketch configures the ESP32 as a BLE Client (Central).
               It reads a local potentiometer value and sends it to the remote BLE Server to control an LED.

  Wiring:
    - ESP32-S3-Zero
    - Potentiometer -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Bluetooth-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Bluetooth-Communication
*/

#include <BLEDevice.h>

// The service and characteristic UUIDs of the server to connect to (must match the server's code)
#define SERVICE_UUID "458063a1-02bf-4664-857e-16c1030be066"
#define BRIGHTNESS_CHARACTERISTIC_UUID "a5209632-66a9-411d-9353-9be5507790fa"

// Global variables
static boolean doConnect = false;
static boolean connected = false;
static BLEAddress *pServerAddress;
static BLERemoteCharacteristic *pRemoteCharacteristic;

// Potentiometer-related definitions
const int potentiometerPin = 7;  // Potentiometer connected to GPIO 7
uint8_t lastBrightness = 0;       // Stores the last brightness value sent (0-255)

class MyClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect: Client Disconnected");
  }
};

// Scan callback class, called when a BLE device is discovered
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Device found, check if it's advertising the service we're looking for.
    if (advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.print("Found target server by Service UUID: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());

      // Stop scanning
      advertisedDevice.getScan()->stop();

      // Save the server address and set the connection flag
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

// Function to connect to the server
bool connectToServer(BLEAddress pAddress) {
  Serial.print("Connecting to ");
  Serial.println(pAddress.toString().c_str());

  // Create a BLE client
  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Client created");

  pClient->setClientCallbacks(new MyClientCallbacks());

  // Connect to the remote BLE server
  if (!pClient->connect(pAddress)) {
    Serial.println(" - Connection failed");
    return false;
  }
  Serial.println(" - Connected to server");

  // Get the service on the server
  BLERemoteService *pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service UUID: ");
    Serial.println(SERVICE_UUID);
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Service found");

  // Get the characteristic in the service
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BRIGHTNESS_CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find characteristic UUID: ");
    Serial.println(BRIGHTNESS_CHARACTERISTIC_UUID);
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Characteristic found");

  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE LED Brightness Controller (Client)...");

  // Initialize BLE. The device name is not necessary for a client as it only scans, not advertises.
  BLEDevice::init("");

  // Get the scan object and set the callback
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // Active scan
  pBLEScan->start(30, false);     // Start scanning for 30 seconds
}

void loop() {
  // If we have a connection instruction and are not yet connected, try to connect
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("Successfully connected to the server!");
      doConnect = false;  // Clear the connection instruction
    } else {
      Serial.println("Failed to connect to the server. Rescanning after 3 seconds...");
      delay(3000);
      BLEDevice::getScan()->start(5, false);  // Rescan for 5 seconds
    }
  }

  // If connected, read the potentiometer and send the data
  if (connected) {
    // Read the analog value from the potentiometer (ESP32 ADC is 12-bit, 0-4095)
    int potValue = analogRead(potentiometerPin);

    // Map the 0-4095 value to a 0-255 brightness range
    uint8_t brightness = map(potValue, 0, 4095, 0, 255);

    // Only send if the brightness has changed significantly to reduce unnecessary communication
    if (abs(brightness - lastBrightness) > 2) {
      Serial.print("Potentiometer value: ");
      Serial.print(potValue);
      Serial.print(" -> Sending brightness: ");
      Serial.println(brightness);

      // Write the single byte brightness value to the server's characteristic
      pRemoteCharacteristic->writeValue(&brightness, 1);

      lastBrightness = brightness;
    }

    delay(100);  // Check every 100ms
  } else {
    // If disconnected, rescan
    if (!doConnect) {
      Serial.println("Disconnected. Rescanning...");
      BLEDevice::getScan()->start(5, false);
    }
  }
}