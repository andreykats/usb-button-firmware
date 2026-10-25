/*
 * USB Button — Adafruit QT Py SAMD21
 * =========================================================
 * Basic USB keyboard button. The trigger button types the selected key; the
 * select button steps to the next key and types it. The onboard NeoPixel shows
 * the device state.
 *
 * BUTTONS (both polled every BUTTON_POLL_MS):
 *   - Trigger (A3 → GND) — key goes down on press and up on release, so holding
 *     it makes the computer repeat the key.
 *   - Select (A0 → GND) — steps to the next key (a–z, then 0–9), saves it to
 *     flash and types it like the trigger. The key is also written at every boot.
 *   Keys are sent without Shift: letters type lower case. Default key: '1'.
 *
 * NEOPIXEL (full brightness):
 *   Blinking yellow (150ms) — booting: waiting for a computer (up to 4 s)
 *   Solid green             — computer connected
 *   Blinking green  (500ms) — no computer: asleep, unplugged or power only
 *   Blinking red    (250ms) — error (flash write failed); held until reboot
 *   White while held        — button press delivered
 *   Red while held          — button press not delivered
 *
 * STATUS LED (external, MO, 10-bit PWM from TCC0):
 *   - Idle: continuous breathing, one fade-in/fade-out per LED_IDLE_PERIOD_MS.
 *   - Trigger or select held: fast blinking for as long as it is held, then back to idle.
 *
 * REQUIRED LIBRARIES: Adafruit NeoPixel, FlashStorage, Keyboard.
 * BOARD: Adafruit QT Py M0 (SAMD21), USB Stack: Arduino.
 */

#include <Adafruit_NeoPixel.h>
#include <Keyboard.h>
#include <FlashStorage.h>
#include "wiring_private.h"   // pinPeripheral()

// ─── Hardware ──────────────────────────────────────────────────────────────
#define SELECT_BTN_PIN   0    // A0, active LOW, internal pull-up
#define TRIGGER_BTN_PIN  3    // A3, active LOW, internal pull-up
#define STATUS_LED_PIN   10   // MO (PA10), external LED — TCC0 waveform output WO[2]
#define STATUS_LED_TCC_CH 2
#define STATUS_RGB_PIN   11   // onboard NeoPixel (its power pin 12 is high by default)

const unsigned long BUTTON_POLL_MS   = 10;
const unsigned long USB_ENUM_WAIT_MS = 4000;  // boot window for a computer to enumerate us
const unsigned long USB_IDLE_MS      = 10;    // no start-of-frame for this long = host asleep/away
const char          DEFAULT_KEY      = '1';

// ─── External status LED ───────────────────────────────────────────────────
// TCC0 at 48 MHz / 8 with a 1024-count period: 10-bit PWM at ~5.9 kHz, close to
// the ESP32 branches' 5 kHz so the transistor stage switches cleanly.
const uint32_t      STATUS_LED_MAX_DUTY = 1023;
const unsigned long LED_IDLE_PERIOD_MS  = 3000;  // one breath (fade up + down) every 3 s while idle
const unsigned long LED_FAST_PERIOD_MS  = 100;   // while a button is held: 50 ms on / 50 ms off (10 Hz)

// Rising half of one breath: round(1023 * ((1 - cos(pi * i / 128)) / 2)^2), i = 0..128.
// Same raised-cosine-squared curve as the ESP32 branches, without soft-float maths at run time.
const uint16_t BREATH_TABLE[129] = {
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    1,    1,    1,    1,    2,    2,    3,    4,    4,    5,    6,
     7,    9,   10,   11,   13,   15,   17,   19,   22,   25,   28,   31,
    34,   38,   42,   46,   51,   55,   60,   66,   71,   77,   84,   90,
    97,  105,  112,  120,  129,  138,  147,  156,  166,  176,  186,  197,
   208,  220,  231,  243,  256,  268,  281,  295,  308,  322,  336,  351,
   365,  380,  395,  410,  426,  441,  457,  473,  489,  505,  521,  537,
   554,  570,  586,  603,  619,  635,  651,  667,  683,  699,  715,  730,
   745,  760,  775,  790,  804,  818,  832,  845,  858,  870,  883,  894,
   906,  917,  927,  937,  947,  956,  964,  972,  979,  986,  993,  998,
  1003, 1008, 1012, 1015, 1018, 1020, 1022, 1023, 1023,
};

// HID keyboard report ID used by the Keyboard library
#define HID_REPORT_ID_KEYBOARD 2

Adafruit_NeoPixel NeoPixel(1, STATUS_RGB_PIN, NEO_GRB + NEO_KHZ800);

// Holds the key character. Zero-filled on every upload, which reads as invalid → DEFAULT_KEY.
FlashStorage(flash_store, int);

// ─── Globals ───────────────────────────────────────────────────────────────
struct Button {
  uint8_t       pin;
  bool          pressed;      // state at the last poll
  unsigned long lastPollMs;
};
enum ButtonEdge { BTN_NONE, BTN_PRESSED, BTN_RELEASED };

Button        triggerBtn     = { TRIGGER_BTN_PIN, false, 0 };
Button        selectBtn      = { SELECT_BTN_PIN,  false, 0 };

char          hidKey         = DEFAULT_KEY;
char          hidKeyDown     = 0;       // key the host may see held; a release is owed while set
bool          pendingPress   = false;   // press during the boot window; sent once it ends

bool          usbHostReady   = false;   // configured and not suspended: reports can be delivered
bool          usbWaitActive  = true;
uint16_t      usbLastFrame   = 0;
unsigned long usbLastFrameMs = 0;

bool          errorLatched   = false;   // flash write failed; shown until reboot
bool          pressDelivered = false;   // feedback colour while a button is held

uint32_t      rgbShown       = 0xFFFFFFFFUL;  // last colour written; invalid until first write

bool          ledFastActive   = false;
unsigned long ledPhaseStartMs = 0;            // start of the current fast or idle cycle
uint32_t      ledLastDuty     = 0xFFFFFFFFUL;

const uint32_t COLOR_GREEN  = Adafruit_NeoPixel::Color(0,   255, 0);
const uint32_t COLOR_YELLOW = Adafruit_NeoPixel::Color(255, 200, 0);
const uint32_t COLOR_RED    = Adafruit_NeoPixel::Color(255, 0,   0);
const uint32_t COLOR_WHITE  = Adafruit_NeoPixel::Color(200, 200, 200);
const uint32_t COLOR_OFF    = 0;

// ─── Keys ──────────────────────────────────────────────────────────────────
bool isValidKey(int c) {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

// Cycle order: a–z then 0–9.
char nextKeyChar(char c) {
  if (c >= 'a' && c < 'z') return c + 1;
  if (c == 'z')            return '0';
  if (c >= '0' && c < '9') return c + 1;
  return 'a';
}

// Write the key and read it back; a mismatch latches the error state.
void saveHidKey() {
  flash_store.write(hidKey);
  if (flash_store.read() != hidKey) errorLatched = true;
}

// ─── HID output ────────────────────────────────────────────────────────────
// Reports go through HID().SendReport() because Keyboard.press() hides send failures.
// Letters and digits map to HID usage IDs without Shift, so letters type lower case.
bool sendKeyReport(char key) {
  KeyReport report = {};
  if (key >= 'a' && key <= 'z')      report.keys[0] = 0x04 + (key - 'a');
  else if (key >= '1' && key <= '9') report.keys[0] = 0x1E + (key - '1');
  else if (key == '0')               report.keys[0] = 0x27;
  return HID().SendReport(HID_REPORT_ID_KEYBOARD, &report, sizeof(report)) == (int)sizeof(report) + 1;
}

bool hidRelease() {
  if (!sendKeyReport(0)) return false;
  hidKeyDown = 0;
  return true;
}

// Press key and hold it until no button is down (handleHidOutput). A key still
// down is released first so the host always sees a fresh key-down.
bool hidPress(char key) {
  if (!usbHostReady) return false;
  if (hidKeyDown) hidRelease();
  // A send that times out may still reach the host, so a release is always owed.
  hidKeyDown = key;
  return sendKeyReport(key);
}

void handleHidOutput() {
  if (hidKeyDown && usbHostReady && !triggerBtn.pressed && !selectBtn.pressed) hidRelease();
}

// Send now, or hold the press until the boot window ends.
void keyPressed() {
  if (usbWaitActive) pendingPress = true;
  else               pressDelivered = hidPress(hidKey);
}

// ─── Buttons ───────────────────────────────────────────────────────────────
ButtonEdge pollButton(Button &btn, unsigned long now) {
  if (now - btn.lastPollMs < BUTTON_POLL_MS) return BTN_NONE;
  btn.lastPollMs = now;
  bool down = digitalRead(btn.pin) == LOW;
  if (down == btn.pressed) return BTN_NONE;
  btn.pressed = down;
  return down ? BTN_PRESSED : BTN_RELEASED;
}

void handleTriggerButton() {
  if (pollButton(triggerBtn, millis()) == BTN_PRESSED) keyPressed();
}

void handleSelectButton() {
  if (pollButton(selectBtn, millis()) != BTN_PRESSED) return;
  hidKey = nextKeyChar(hidKey);
  keyPressed();
  saveHidKey();
}

// ─── USB host tracking ─────────────────────────────────────────────────────
// The core does not track suspend. The host sends a start-of-frame every 1 ms
// while the bus is active, so a frozen frame number means it is asleep or gone.
void handleHostEnumeration() {
  unsigned long now   = millis();
  uint16_t      frame = USB->DEVICE.FNUM.bit.FNUM;
  if (frame != usbLastFrame) {
    usbLastFrame   = frame;
    usbLastFrameMs = now;
  }
  bool mounted = USBDevice.configured();
  usbHostReady = mounted && now - usbLastFrameMs < USB_IDLE_MS;

  if (usbWaitActive && (mounted || now >= USB_ENUM_WAIT_MS)) {
    usbWaitActive = false;
    if (pendingPress) {
      pendingPress = false;
      keyPressed();
    }
  }
}

// ─── Status display ────────────────────────────────────────────────────────
void showStatus(uint32_t c) {
  if (c == rgbShown) return;
  rgbShown = c;
  NeoPixel.setPixelColor(0, c);
  NeoPixel.show();
}

uint32_t blinkColor(uint32_t c, unsigned long periodMs) {
  return (millis() / periodMs) % 2 ? c : COLOR_OFF;
}

// Press feedback wins; during the boot window a press is still pending, so show booting.
void handleStatusRgb() {
  bool held = triggerBtn.pressed || selectBtn.pressed;
  if (held && !usbWaitActive)                  showStatus(pressDelivered ? COLOR_WHITE : COLOR_RED);
  else if (errorLatched)                       showStatus(blinkColor(COLOR_RED, 250));
  else if (usbWaitActive)                      showStatus(blinkColor(COLOR_YELLOW, 150));
  else if (usbHostReady)                       showStatus(COLOR_GREEN);
  else                                         showStatus(blinkColor(COLOR_GREEN, 500));
}

// ─── External status LED (non-blocking) ────────────────────────────────────
// The buffered compare register (CCB) takes effect at the next PWM period, so
// duty changes never glitch the output.
void writeStatusLed(uint32_t duty) {
  if (duty == ledLastDuty) return;
  ledLastDuty = duty;
  TCC0->CCB[STATUS_LED_TCC_CH].reg = duty;
  while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CCB2) { }
}

// analogWrite() only gives 8-bit PWM at ~730 Hz, so TCC0 is set up directly.
void initStatusLed() {
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC0_TCC1);
  while (GCLK->STATUS.bit.SYNCBUSY) { }

  TCC0->CTRLA.bit.SWRST = 1;
  while (TCC0->SYNCBUSY.bit.SWRST) { }
  TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV8 | TCC_CTRLA_PRESCSYNC_GCLK;
  TCC0->WAVE.reg  = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC0->SYNCBUSY.bit.WAVE) { }
  TCC0->PER.reg   = STATUS_LED_MAX_DUTY;
  while (TCC0->SYNCBUSY.bit.PER) { }
  TCC0->CC[STATUS_LED_TCC_CH].reg = 0;
  while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC2) { }
  TCC0->CTRLA.bit.ENABLE = 1;
  while (TCC0->SYNCBUSY.bit.ENABLE) { }

  pinPeripheral(STATUS_LED_PIN, PIO_TIMER_ALT);
  ledPhaseStartMs = millis();
  ledLastDuty     = 0;
}

// Fast blink while the trigger or select is held, idle breathing otherwise. Each
// switch restarts the cycle, so the blink starts lit and the breathing starts dark.
void handleStatusLed() {
  unsigned long now = millis();

  bool fast = triggerBtn.pressed || selectBtn.pressed;
  if (fast != ledFastActive) {
    ledFastActive   = fast;
    ledPhaseStartMs = now;
  }
  unsigned long t = now - ledPhaseStartMs;

  if (ledFastActive) {
    writeStatusLed((t % LED_FAST_PERIOD_MS) < LED_FAST_PERIOD_MS / 2 ? STATUS_LED_MAX_DUTY : 0);
    return;
  }

  // Idle: 256 table steps per breath, up the rising half then back down it.
  uint32_t step = (t % LED_IDLE_PERIOD_MS) * 256 / LED_IDLE_PERIOD_MS;
  writeStatusLed(BREATH_TABLE[step <= 128 ? step : 256 - step]);
}

// ─── setup() / loop() ──────────────────────────────────────────────────────
void setup() {
  pinMode(TRIGGER_BTN_PIN, INPUT_PULLUP);
  pinMode(SELECT_BTN_PIN, INPUT_PULLUP);
  initStatusLed();
  NeoPixel.begin();
  Keyboard.begin();

  int stored = flash_store.read();
  hidKey = isValidKey(stored) ? (char)stored : DEFAULT_KEY;
  saveHidKey();
}

// Host state is refreshed first and the trigger is read before display work,
// so a local press reaches HID as soon as possible.
void loop() {
  handleHostEnumeration();
  handleTriggerButton();
  handleHidOutput();
  handleSelectButton();
  handleStatusRgb();
  handleStatusLed();
}
