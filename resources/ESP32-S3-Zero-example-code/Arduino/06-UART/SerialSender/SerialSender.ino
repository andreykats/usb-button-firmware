/*
  Serial Sender

  Description: Sends commands via UART based on button state; this code is intended to be used with "SerialReceiver.ino" where it sends '1' when the button is pressed (LED ON) and '0' when released (LED OFF).

  Wiring:
    Tips: The wiring connections are crossed (TX to RX, RX to TX).
    
    [Sender Device]              [Receiver Device]
    ESP32-S3-Zero                 ESP32-S3-Zero
    Button (GPIO 7)               LED (GPIO 7)
    TX (GPIO 12)       ------>    RX (GPIO 1)
    RX (GPIO 11)       <------    TX (GPIO 2)
    GND                <------>   GND

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/UART-Communication
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/UART-Communication
*/

#define UART1_RX_PIN 11  // UART1 receive pin (RX)
#define UART1_TX_PIN 12  // UART1 transmit pin (TX)

const int buttonPin = 7;    // Pin connected to the button
int lastButtonState = LOW;  // Previous button state

void setup() {
  // Start the primary serial port for debug output
  Serial.begin(115200);
  // while(!Serial){};

  // Start Serial1 for device-to-device communication, specifying RX/TX pins
  Serial1.begin(9600, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);

  pinMode(buttonPin, INPUT_PULLUP);  // Configure button pin as a pull-up input

  Serial.println("Sender Ready. Press the button.");
}

void loop() {
  int currentButtonState = digitalRead(buttonPin);

  // Check if the button state has changed
  if (currentButtonState != lastButtonState) {
    if (currentButtonState == HIGH) {
      Serial1.write('0');  // Button released, send '0'
      Serial.println("Sent: 0 (Button Released)");
    } else {
      Serial1.write('1');  // Button pressed, send '1'
      Serial.println("Sent: 1 (Button Pressed)");
    }

    lastButtonState = currentButtonState;  // Update the last button state
    delay(50);                             // Basic debounce delay
  }
}