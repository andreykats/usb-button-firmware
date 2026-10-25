#include <Adafruit_NeoPixel.h>
#include <Keyboard.h>
#include <FlashStorage.h>

#define NUM_NEO_PIXELS 1
#define PIN_NEO_PIXEL 11

#define PIN_SELECT_BUTTON 0
#define PIN_ACTION_BUTTON 3
#define PIN_ACTION_LED 10

#define LED_TIMEOUT 1000

#define TOTAL 4

Adafruit_NeoPixel NeoPixel(NUM_NEO_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

// Reserve a portion of flash memory to store an "int" variable
FlashStorage(flash_store, int);

int actionButtonState = 0;
int previousActionButtonState = 1;
int selectButtonState = 0;
int previousSelectButtonState = 1;
int ledState = 0;
uint8_t key;
int currentSelection = flash_store.read();
unsigned long led_timer = 0;

void setup()
{
  // setup pins 
  pinMode(PIN_ACTION_BUTTON, INPUT_PULLUP);
  pinMode(PIN_SELECT_BUTTON, INPUT_PULLUP);
  pinMode(PIN_ACTION_LED, OUTPUT);

  // start Neopixel
  NeoPixel.begin();
  
  // start USB keyboard
  Keyboard.begin();

  // set initial configuration
  setConfig();
}

void setConfig() 
{
  // Check if nil then set default config
  if (!currentSelection) {
    currentSelection = 1;
  }

  if (currentSelection == 1) {
    // Set config led color then show
    NeoPixel.clear();
    NeoPixel.setPixelColor(0, NeoPixel.Color(0, 255, 0));
    NeoPixel.show();

    // Set keyboard key to selection
    key = '1';

    // Save config to flash memory
    flash_store.write(1);
  }
  
  if (currentSelection == 2) {
    // Set config led color then show
    NeoPixel.clear();
    NeoPixel.setPixelColor(0, NeoPixel.Color(255, 0, 0));
    NeoPixel.show();

    // Add a startup delay to the led
    led_timer = 1000;

    // Set keyboard key to selection
    key = '2';

    // Save config to flash memory
    flash_store.write(2);
  }
  
  if (currentSelection == 3) {
    // Set config led color then show
    NeoPixel.clear();
    NeoPixel.setPixelColor(0, NeoPixel.Color(255, 255, 0));
    NeoPixel.show();

    // Add a startup delay to the led
    led_timer = 2000;

    // Set keyboard key to selection
    key = '3';

    // Save config to flash memory
    flash_store.write(3);
  }  

  if (currentSelection == 4) {
    // Set config led color then show
    NeoPixel.clear();
    NeoPixel.setPixelColor(0, NeoPixel.Color(0, 0, 255));
    NeoPixel.show();

    // Add a startup delay to the led
    led_timer = 3000;

    // Set keyboard key to selection
    key = '4';

    // Save config to flash memory
    flash_store.write(4);
  }  
}
  
void loop()
{
  // select button configuration
  selectButtonState = digitalRead(PIN_SELECT_BUTTON);
  if (selectButtonState == LOW) { 
    if (selectButtonState != previousSelectButtonState) {
      if (currentSelection != TOTAL) {
        currentSelection++;
      } else {
        currentSelection = 1;
      }
      setConfig();
      previousSelectButtonState = selectButtonState;
    }
  } else {
    if (selectButtonState != previousSelectButtonState) { 
      previousSelectButtonState = selectButtonState;
    }
  }
  
  // check button state then send keyboard stroke
  actionButtonState = digitalRead(PIN_ACTION_BUTTON);
  if (actionButtonState == LOW) {
    if (actionButtonState != previousActionButtonState) {
      // turn LED on
      digitalWrite(PIN_ACTION_LED, HIGH);
      Keyboard.press(key);
      previousActionButtonState = actionButtonState;
    }
  } else {
    if (actionButtonState != previousActionButtonState) {
      // turn LED off
      digitalWrite(PIN_ACTION_LED, LOW);
      Keyboard.releaseAll();
      previousActionButtonState = actionButtonState;
    }
  }

  // blink led at a set interval
  if (millis() > led_timer + LED_TIMEOUT) { 
    led_timer = millis();
    if (ledState == 0) {
      digitalWrite(PIN_ACTION_LED, HIGH);
      ledState = 1;
    } else {
      digitalWrite(PIN_ACTION_LED, LOW);
      ledState = 0;
    }
  }

  // Delay a little bit to improve performance
  delay(10);
}