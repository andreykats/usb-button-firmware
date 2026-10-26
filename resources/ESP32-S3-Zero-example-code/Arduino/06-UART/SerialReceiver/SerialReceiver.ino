/*
  Serial Receiver

  Description: Receives commands via UART and controls an LED; this code is intended to be used with "SerialSender.ino" where receiving '1' turns the LED ON and receiving '0' turns the LED OFF.

  Wiring:
    Tips: The wiring connections are crossed (RX to TX, TX to RX).

    [Receiver Device]            [Sender Device]
    ESP32-S3-Zero                 ESP32-S3-Zero
    LED (GPIO 7)                  Button (GPIO 7)
    RX (GPIO 1)        <------    TX (GPIO 12)
    TX (GPIO 2)        ------>    RX (GPIO 11)
    GND                <------>   GND

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/UART-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/UART-Communication
*/

#define UART1_RX_PIN 1  // UART1 receive pin (RX)
#define UART1_TX_PIN 2  // UART1 transmit pin (TX)

const int ledPin = 7;

void setup() {
  // Start the primary serial port for debug output
  Serial.begin(115200);
  // while(!Serial){};

  // Start Serial1 for device-to-device communication, specifying RX/TX pins
  Serial1.begin(9600, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);

  pinMode(ledPin, OUTPUT);  // Set the LED pin to output mode

  Serial.println("Receiver Ready. Waiting for commands...");
}

void loop() {
  // Check for incoming data on Serial1
  if (Serial1.available()) {
    char command = Serial1.read();  // Read one byte (character)

    // Control the LED based on the command
    if (command == '1') {
      // If '1' is received, turn the LED on
      digitalWrite(ledPin, HIGH);
      Serial.println("Received: 1 -> LED ON");
    } else if (command == '0') {
      // If '0' is received, turn the LED off
      digitalWrite(ledPin, LOW);
      Serial.println("Received: 0 -> LED OFF");
    }
  }
}