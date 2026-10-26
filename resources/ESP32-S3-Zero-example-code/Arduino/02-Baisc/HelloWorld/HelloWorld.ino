/*
  HelloWorld
*/

void setup() {
  // Code here runs once:
  Serial.begin(9600);  // Initialize serial communication with baud rate 9600
  while (!Serial){
    ; // Wait for serial connection to establish
  };
  Serial.println("Hello World!");  // Send the string "Hello World!" via serial and add newline
}

void loop() {
  // Code here runs repeatedly:
}