/*
  Traffic Light

  Description: Simulates a traffic light system with Red, Green, and blinking Yellow lights.
               The current state is also printed to the Serial Monitor.

  Wiring:
  - ESP32-S3-Zero
  - Red LED    -> GPIO 7
  - Yellow LED -> GPIO 8
  - Green LED  -> GPIO 9

  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-1-Traffic-Light
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-1-Traffic-Light
*/

// Pin definitions
const int redPin = 7;
const int yellowPin = 8;
const int greenPin = 9;

// Timing parameters (in milliseconds)
const unsigned long redDuration = 10000;    // Red light duration
const unsigned long greenDuration = 8000;   // Green light duration
const unsigned long yellowDuration = 3000;  // Total duration for the yellow light
const unsigned long blinkInterval = 500;    // Blinking interval

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  Serial.println("Traffic light simulation program started...");
  Serial.print("Current configuration: Red=");
  Serial.print(redDuration / 1000);
  Serial.print("s, Green=");
  Serial.print(greenDuration / 1000);
  Serial.print("s, Yellow=");
  Serial.println(yellowDuration / 1000);

  // Configure LED pins as output
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
}

// Helper function: turn off all lights
void allLightsOff() {
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, LOW);
}

void loop() {
  // --- Green Light Phase ---
  Serial.println("State: Green light ON");
  allLightsOff();                // Ensure a clean state before starting
  digitalWrite(greenPin, HIGH);
  delay(greenDuration);

  // --- Yellow Light Blinking Phase ---
  Serial.println("State: Yellow light blinking");
  digitalWrite(greenPin, LOW);

  // Calculate the number of blinks
  // A full cycle includes ON and OFF, with a duration of blinkInterval * 2
  int numBlinks = yellowDuration / (blinkInterval * 2);

  // Ensure at least one blink, even if the duration is short
  if (numBlinks == 0) {
    numBlinks = 1;
  }

  for (int i = 0; i < numBlinks; i++) {
    digitalWrite(yellowPin, HIGH);
    delay(blinkInterval);
    digitalWrite(yellowPin, LOW);
    delay(blinkInterval);
  }

  // --- Red Light Phase ---
  Serial.println("State: Red light ON");
  // At this point, the yellow and green lights are off, so just turn on the red light
  digitalWrite(redPin, HIGH);
  delay(redDuration);
}