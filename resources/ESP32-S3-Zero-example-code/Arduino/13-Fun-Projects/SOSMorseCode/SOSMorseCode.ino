/*
  SOS Morse Code

  Description: Simulates an SOS signal using an LED and a buzzer.
               
  Wiring:
  - ESP32-S3-Zero
  - LED    -> GPIO 7
  - Buzzer -> GPIO 8
  
  Link:
  - Tutorial [ZH]: https://docs.waveshare.net/ESP32-Arduino-Tutorials/13-2-SOS
  - Tutorial [EN]: https://docs.waveshare.com/ESP32-Arduino-Tutorials/13-2-SOS
*/

// Pins for the LED and buzzer
const int ledPin = 7;
const int buzzerPin = 8;

// --- Morse Code Timing Definitions (in milliseconds) ---
const int dotDuration = 200;                 // Duration of a "dot" (1t)
const int dashDuration = dotDuration * 3;    // Duration of a "dash" (3t)
const int interElementGap = dotDuration;     // Gap between elements within the same letter (1t)
const int interLetterGap = dotDuration * 3;  // Gap between letters (3t)
const int interWordGap = dotDuration * 7;    // Gap between SOS sequences (inter-word gap) (7t)

// Function declarations
void signalOn();
void signalOff();
void dot();
void dash();
void letterS();
void letterO();
void playSOS();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Program started, preparing to send SOS signal...");

  // Configure pins as output
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Ensure devices are off initially
  signalOff();
  delay(2000);  // Wait for 2 seconds before starting
}

void loop() {
  playSOS();

  // After a complete SOS sequence, wait for the inter-word gap
  Serial.print("Waiting for ");
  Serial.print(interWordGap);
  Serial.println(" ms before repeating...\n");
  delay(interWordGap);
}

// --- Core Functions ---

// Turn on both the LED and buzzer
void signalOn() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(buzzerPin, HIGH);
}

// Turn off both the LED and buzzer
void signalOff() {
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
}

// Send a "dot" signal
void dot() {
  signalOn();
  delay(dotDuration);
  signalOff();
}

// Send a "dash" signal
void dash() {
  signalOn();
  delay(dashDuration);
  signalOff();
}

// Send the letter 'S' (...): three dots
void letterS() {
  Serial.print(".");
  dot();
  delay(interElementGap);
  Serial.print(".");
  dot();
  delay(interElementGap);
  Serial.print(".");
  dot();
}

// Send the letter 'O' (---): three dashes
void letterO() {
  Serial.print("-");
  dash();
  delay(interElementGap);
  Serial.print("-");
  dash();
  delay(interElementGap);
  Serial.print("-");
  dash();
}

// Play one complete SOS signal sequence
void playSOS() {
  Serial.print("Sending S: ");
  letterS();
  Serial.print(" | ");
  delay(interLetterGap);

  Serial.print("Sending O: ");
  letterO();
  Serial.print(" | ");
  delay(interLetterGap);

  Serial.print("Sending S: ");
  letterS();
  Serial.println();
  Serial.println("SOS sequence sent.");
}