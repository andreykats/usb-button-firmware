/*
  Potentiometer Reader

  Reads the value of a potentiometer connected to an analog pin and prints the result to the Serial Monitor.

  Wiring:
  - ESP32-S3-Zero
  - Potentiometer -> GPIO 7

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/Analog-Input
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/Analog-Input
*/

const int potentiometerPin = 7;  // Pin connected to the potentiometer

void setup() {
  Serial.begin(9600);  // Initialize serial communication at 9600 baud
}

void loop() {
  int analogValue = analogRead(potentiometerPin);            // Read analog value (0-4095)
  int analogVolts = analogReadMilliVolts(potentiometerPin);  // Read voltage in millivolts (mV)

  // Print ADC value
  Serial.print("ADC_Value:");
  Serial.print(analogValue);

  // Print separator
  Serial.print(",");

  // Print voltage and end line
  Serial.print("Voltage_mV:");
  Serial.println(analogVolts);

  delay(100);  // Delay to prevent flooding the serial output
}