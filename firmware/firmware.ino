/*
 * USB Button
 * =========================================================
 * Port of the ESP32-S3-Matrix firmware on main (itself a port of the QT Py
 * firmware) to the Waveshare ESP32-S3-Zero. The trigger button types the
 * selected key; the BOOT button steps to the next key and types it. The onboard
 * RGB LED shows the device state.
 *
 * DELIVERY PATHS (chosen automatically by USB enumeration, same for both buttons):
 *   - Hosted (a computer enumerated the unit at any point this uptime) → USB HID
 *     keystroke, ahead of every other channel. The unit is also the ESP-NOW
 *     "primary" and types keystrokes relayed from other units, behind its own.
 *     Hosted is latched: if the host sleeps or is unplugged (e.g. from a hub), the
 *     unit waits for it to come back and never falls back to ESP-NOW.
 *   - Never hosted (charger / hub without a computer) → keystroke is sent to the
 *     primary via ESP-NOW unicast; the primary types it on its computer.
 *
 * OPTIONAL CHANNELS (compile-time, both OFF by default):
 *   - WEB_SERIAL_ENABLED=1 — debug Web Serial protocol over USB CDC in normal mode
 *     (debug.html on branch web-serial-interface). Settings mode has its own protocol,
 *     always built in.
 *   - WIFI_ENABLED=1       — WebSocket over WiFi + encrypted ESP-NOW credential
 *     sharing. Replaces the ESP-NOW keystroke relay (the two are exclusive).
 *
 * BUTTONS (both polled every BUTTON_POLL_MS; while a press is held the RGB LED
 *          shows white if the key was delivered, red if not):
 *   - Trigger (external, TRIGGER_BTN_PIN → GND) — types the selected key, on press
 *     (default) or on release (setting "edge"). On press with "repeat" on (default)
 *     the key is held over USB HID while the button is, so the computer repeats it;
 *     otherwise it is a single tap. Over ESP-NOW the primary always types a tap.
 *   - BOOT (GPIO0) — on press, steps to the next key (a–z, then 0–9), applies it at
 *     once, saves it to NVS and types it as a single tap (never repeats). The key is
 *     also written at every boot. The USB product name ("Zero Remote [x]") updates
 *     at the next power-up. Held for SETTINGS_HOLD_MS: the key steps back and the
 *     unit restarts into settings mode.
 *   Keys are sent without Shift: letters type lower case. Default key: '1'.
 *
 * SETTINGS MODE (BOOT held SETTINGS_HOLD_MS):
 *   The unit restarts with USB serial only (no keyboard, no ESP-NOW) as "USB
 *   Button [key] (last 3 MAC bytes)", e.g. "USB Button [2] (EB:F4:58)", RGB LED
 *   cycling the rainbow. The settings page (branch web-serial-interface,
 *   https://button.drey.dev) changes the settings below; each change is saved to
 *   NVS and applied at once.
 *   The trigger doesn't type: it previews the status LED patterns and the settings
 *   page shows what it would type (TRIGGER/TYPE lines). A new BOOT press, RESET or
 *   a power cycle returns to normal mode: the mode flag is cleared as settings
 *   mode starts. A restart from software does not re-sample the strapping pins
 *   (ESP32-S3 TRM, Table 7-1), so BOOT held through it does not enter download mode.
 *
 *   Setting    Values (default first)
 *   edge       press, release        — when the trigger types
 *   repeat     on, off               — trigger held repeats (edge=press only)
 *   pressed    fast, on, slow, off   — status LED while a button is held
 *   idle       breathe, fast, slow, off — status LED otherwise
 *   rgb        normal, high, low     — onboard RGB LED at 20 %, 100 % or 1 % brightness
 *
 * STATUS LED (external, STATUS_LED_PIN → transistor switching 5V to an LED):
 *   - Idle (setting "idle"): breathing, one fade-in/fade-out per LED_BREATHE_PERIOD_MS.
 *   - Trigger or BOOT held (setting "pressed"): fast blinking for as long as it is
 *     held, then back to idle.
 *   Fast blink: LED_FAST_PERIOD_MS cycle; slow blink: LED_SLOW_PERIOD_MS cycle;
 *   "on": a LED_FLICKER_PERIOD_MS blink, too fast to see, so it looks steady.
 *
 * REQUIRED LIBRARIES (install via Arduino Library Manager):
 *   - None for the default build.
 *   - ESPAsyncWebServer by ESP32Async (only needed when WIFI_ENABLED=1)
 *   - AsyncTCP by ESP32Async         (only needed when WIFI_ENABLED=1)
 *
 * Already built-in (no install needed):
 *   - USB (USBHIDKeyboard, USBCDC)  — part of the esp32 board package
 *   - rgbLedWriteOrdered()          — part of the esp32 board package
 *   - WiFi                          — part of the esp32 board package
 *   - esp_now                       — part of the esp32 board package
 *
 * BOARD SETTINGS (Tools menu in Arduino IDE, esp32 core 3.3.5+):
 *   Board           : Waveshare ESP32-S3-Zero  ← 4MB flash fixed by the board
 *   PSRAM           : Enabled                  ← 2MB quad PSRAM (ESP32-S3FH4R2)
 *   USB Mode        : USB-OTG (TinyUSB)        ← CRITICAL — HID needs TinyUSB;
 *                                                the board defaults to Hardware CDC
 *   USB CDC On Boot : Disabled                 ← CRITICAL — the sketch creates the CDC
 *                                                port (Cdc) itself and starts USB in
 *                                                setup(), so it can leave the keyboard
 *                                                out in settings mode. "Enabled" starts
 *                                                USB before setup().
 *   Upload Mode     : USB-OTG CDC (TinyUSB)    ← 1200-baud touch on Cdc resets into
 *                                                the bootloader
 *
 * HARDWARE PINS (from ESP32-S3-Zero schematic, resources/):
 *   GPIO 0   — BOOT button (active LOW, 10K external pull-up) — key select
 *   GPIO 1   — status LED transistor drive (PWM, HIGH = LED on)    — STATUS_LED_PIN
 *   GPIO 13  — trigger button (external, to GND, internal pull-up) — TRIGGER_BTN_PIN
 *   GPIO 21  — onboard WS2812B RGB LED data (RGB colour order)     — STATUS_RGB_PIN
 *   There is no VBUS sense pin: host vs. power-only is detected by USB
 *   enumeration.
 *
 * SETTINGS PROTOCOL (settings mode only, 115200 baud, newline-terminated):
 *
 *   Browser → ESP32:
 *     GET\n                  Report the settings
 *     SET:name=value\n       Change one setting (names and values above)
 *     DEFAULTS\n             Restore every setting to its default
 *     REBOOT\n               Restart into normal mode
 *
 *   ESP32 → Browser:
 *     SETTINGS:edge=press,repeat=on,pressed=fast,idle=breathe,rgb=normal,key=1\n   (GET,
 *                            DEFAULTS; key is read-only)
 *     OK:name=value\n        Setting saved and applied
 *     TRIGGER:down|up\n      Trigger pressed / released (trigger test)
 *     TYPE:hold|tap\n        What that edge would type: a key held until
 *                            TRIGGER:up (computer repeats it) or a single tap
 *     ERR:message\n          Unknown command or setting, invalid value, NVS failure
 *     REBOOTING...\n         Restarting into normal mode
 *
 * DEBUG SERIAL PROTOCOL (normal mode, WEB_SERIAL_ENABLED=1, 115200 baud,
 * newline-terminated; debug.html on branch web-serial-interface):
 *
 *   ESP32 → Browser:
 *     READY\n                    Device booted, waiting
 *     CONFIG:key=X\n             Reports current HID key
 *     KEY_SAVED:X\n              New HID key saved to NVS
 *     REBOOTING...\n             Device is restarting
 *     LOG:message ts=Xms\n       Diagnostic / timing log line
 *     ERR:message\n              Error description
 *
 *   ESP-NOW relay messages (WIFI_ENABLED=0):
 *     PRIMARY\n                  This device became the primary hub
 *     RELAY:X\n                  Keystroke relayed from remote device X
 *     PEER:X\n                   Remote device X first seen
 *     PEERS:N\n                  Total remote peer count
 *
 *   WiFi/WebSocket messages (WIFI_ENABLED=1):
 *     WIFI_CONNECTING\n          Starting WiFi association
 *     WIFI_CONNECTED:IP\n        WiFi up with assigned IP address
 *     WIFI_FAILED:...\n          WiFi connection timed out
 *     WIFI_CLEARED\n             WiFi credentials erased from NVS
 *     WS_STARTED:ws://IP:81\n    WebSocket server ready at this URL
 *     ESPNOW_RECEIVED:ssid\n     Credentials received from another unit
 *     ESPNOW_BROADCASTING:ssid\n Broadcasting credentials to nearby units
 *     ESPNOW_BROADCAST_DONE\n    Broadcast complete, reconnecting to WiFi
 *
 *   Browser → ESP32:
 *     KEY:x\n              Set new HID key
 *     KEY_CLEAR\n           Reset HID key to default '1'
 *     REBOOT\n              Reboot device
 *     WIFI:ssid:password\n  Send credentials (WIFI_ENABLED=1 only)
 *     WIFI_CLEAR\n          Erase saved WiFi credentials (WIFI_ENABLED=1 only)
 *
 * RGB STATUS LED (brightness: setting "rgb"):
 *   Blinking yellow (150ms) — booting: waiting to learn whether a computer is attached
 *   Solid green             — computer connected: HID + primary hub (also Web Serial)
 *   Blinking green  (500ms) — computer was connected but is asleep/unplugged: waiting
 *   Solid blue              — no computer, connected to the mesh via a primary
 *   Blinking blue   (500ms) — no computer, looking for a primary
 *   Blinking red    (250ms) — error (ESP-NOW failed to start); held until reboot
 *   White while held        — button press delivered (trigger or BOOT). With
 *                             edge=release: for PRESS_FEEDBACK_MS after release
 *   Red while held          — button press could not be delivered (same timing)
 *   Rainbow (3 s cycle)     — settings mode, starting from purple
 *   WiFi mode (WIFI_ENABLED=1):
 *   Blinking blue   (500ms) — connecting to WiFi
 *   Alternating purple/blue — WiFi up, waiting for WS client
 *   Solid blue              — WiFi + WebSocket connected
 *   Blinking red 10x(250ms) — WiFi failed
 */

// ─── Compile-time tuning switches ────────────────────────────────────────────
// (defined before includes so the conditional includes below see them)

// 0 = ESP-NOW relay mode (device-to-device). 1 = WiFi/WebSocket mode.
#ifndef WIFI_ENABLED
#define WIFI_ENABLED 0
#endif

// 1 = Web Serial protocol (browser app) over USB CDC. 0 = disabled.
#ifndef WEB_SERIAL_ENABLED
#define WEB_SERIAL_ENABLED 0
#endif

// 1 = emit verbose LOG:* lines over CDC Cdc. 0 = suppress verbose logs.
#ifndef ENABLE_VERBOSE_LOGS
#define ENABLE_VERBOSE_LOGS 0
#endif

// ─── Includes ──────────────────────────────────────────────────────────────
#include <USB.h>              // ESP32 built-in USB stack (TinyUSB)
#include <USBHIDKeyboard.h>   // Built-in HID keyboard (part of USB library)
#include <tusb.h>             // tud_mounted() / tud_suspended() for host tracking
#include <WiFi.h>
#include <esp_wifi.h>             // WiFi power-save control + channel setting
#include <esp_now.h>              // ESP-NOW peer-to-peer communication
#include <esp_mac.h>              // esp_efuse_mac_get_default() — settings-mode USB name
#include <Preferences.h>         // NVS flash — persists key across reboots/reflash

#if WIFI_ENABLED
#include <mbedtls/aes.h>          // AES-128-CBC encryption (hardware-accelerated)
#include <ESPAsyncWebServer.h>    // ESP32Async version — only for WiFi/WS mode
#endif

// ─── Board-setting guards ────────────────────────────────────────────────────
#if ARDUINO_USB_MODE
#error "Set Tools → USB Mode: USB-OTG (TinyUSB). USB HID does not work in Hardware CDC and JTAG mode."
#endif
#if ARDUINO_USB_CDC_ON_BOOT
#error "Set Tools → USB CDC On Boot: Disabled. The sketch starts USB itself, with or without the keyboard."
#endif

// Hold duration of a relayed key tap before release (handled asynchronously in loop()).
const unsigned long HID_HOLD_MS = 10;

// ─── USB Objects ───────────────────────────────────────────────────────────
// USB interfaces register when their objects are constructed and are fixed by
// USB.begin() in setup(). The CDC port is in both modes, so it is a global.
// Keyboard registers the HID keyboard interface and is created in setup() in
// normal mode only; reports are sent through hid directly because
// Keyboard.press() discards the delivery result. hid must be created after
// keyboard so the keyboard protocol is the one registered.
USBCDC          Cdc(0);
USBHIDKeyboard *keyboard = nullptr;
USBHID         *hid      = nullptr;

#define LOG_PRINTF(...) do { if (ENABLE_VERBOSE_LOGS && (bool)Cdc) Cdc.printf(__VA_ARGS__); } while (0)
#define LOG_PRINTLN(x)  do { if (ENABLE_VERBOSE_LOGS && (bool)Cdc) Cdc.println(x); } while (0)

// Web Serial protocol output — compiled out unless WEB_SERIAL_ENABLED=1
#define PROTO_PRINTF(...) do { if (WEB_SERIAL_ENABLED && (bool)Cdc) Cdc.printf(__VA_ARGS__); } while (0)
#define PROTO_PRINTLN(x)  do { if (WEB_SERIAL_ENABLED && (bool)Cdc) Cdc.println(x); } while (0)

// ─── Hardware ──────────────────────────────────────────────────────────────
// Trigger: external momentary switch between this pin and GND. GPIO13 is a header
// pin with no strapping/USB/LED role; avoid strapping pins 0, 3, 45, 46.
#define TRIGGER_BTN_PIN  13   // Active LOW, internal pull-up
#define KEY_BTN_PIN      0    // BOOT (K2), active LOW, 10K external pull-up
const unsigned long BUTTON_POLL_MS = 10;

// ─── External status LED (transistor-switched 5V) ──────────────────────────
// GPIO drives the base of an NPN emitter follower feeding the LED from 5V; HIGH = LED on.
// Set STATUS_LED_INVERT 1 for a stage where LOW = LED on (e.g. PNP / P-MOSFET).
#define STATUS_LED_PIN     1
#define STATUS_LED_INVERT  0
const uint32_t      STATUS_LED_PWM_HZ    = 5000;
const uint8_t       STATUS_LED_PWM_BITS  = 10;
const uint32_t      STATUS_LED_MAX_DUTY  = (1UL << STATUS_LED_PWM_BITS) - 1;
const unsigned long LED_BREATHE_PERIOD_MS = 3000;  // one breath (fade up + down) every 3 s
const unsigned long LED_FAST_PERIOD_MS    = 100;   // fast blink: 50 ms on / 50 ms off (10 Hz)
const unsigned long LED_SLOW_PERIOD_MS    = 1000;  // slow blink: 500 ms on / 500 ms off
const unsigned long LED_FLICKER_PERIOD_MS = 8;     // "on": 4 ms on / 4 ms off (125 Hz), looks steady

// ─── Onboard RGB status LED (WS2812B) ──────────────────────────────────────
// Driven by the core's rgbLedWriteOrdered() (RMT). The S3-Zero's LED takes RGB
// order, not the usual WS2812B GRB (matches the core's waveshare_esp32_s3_zero
// variant). If red and green look swapped on hardware, change STATUS_RGB_ORDER.
#define STATUS_RGB_PIN         21
#define STATUS_RGB_ORDER       LED_COLOR_ORDER_RGB
const unsigned long RAINBOW_PERIOD_MS  = 3000;  // settings mode: one trip round the colour wheel
const uint16_t      RAINBOW_START_POS  = 4 * 256 + 160;  // colorWheel() purple (160, 0, 255)
const unsigned long RAINBOW_FRAME_MS   = 20;    // settings mode: RGB update interval
const unsigned long PRESS_FEEDBACK_MS  = 200;   // edge=release: white/red shown after the release

// ─── Settings mode ─────────────────────────────────────────────────────────
const unsigned long SETTINGS_HOLD_MS   = 3000;  // BOOT held this long → restart into settings mode
bool                settingsMode       = false; // this boot is settings mode (NVS flag, cleared at boot)
unsigned long       settingsStartMs    = 0;     // settings mode: rainbow starts here

// ─── User settings (NVS, changed in settings mode) ─────────────────────────
// Each setting stores one value from its choices; the first choice is the default.
enum TriggerEdge : uint8_t { EDGE_PRESS, EDGE_RELEASE };
enum LedPattern  : uint8_t { LED_BREATHE, LED_FAST, LED_SLOW, LED_ON, LED_OFF };

uint8_t trigEdge   = EDGE_PRESS;
uint8_t trigRepeat = 1;
uint8_t ledPressed = LED_FAST;
uint8_t ledIdle    = LED_BREATHE;
uint8_t rgbPercent = 20;           // onboard RGB LED brightness, percent of each channel

struct SettingChoice { const char *name; uint8_t value; };
struct SettingDef {
  const char          *name;     // protocol name
  const char          *nvsKey;
  uint8_t             *value;
  const SettingChoice *choices;  // choices[0] is the default
  uint8_t              count;
};

const SettingChoice EDGE_CHOICES[]    = { {"press", EDGE_PRESS}, {"release", EDGE_RELEASE} };
const SettingChoice REPEAT_CHOICES[]  = { {"on", 1}, {"off", 0} };
const SettingChoice PRESSED_CHOICES[] = { {"fast", LED_FAST}, {"on", LED_ON}, {"slow", LED_SLOW}, {"off", LED_OFF} };
const SettingChoice IDLE_CHOICES[]    = { {"breathe", LED_BREATHE}, {"fast", LED_FAST}, {"slow", LED_SLOW}, {"off", LED_OFF} };
const SettingChoice RGB_CHOICES[]     = { {"normal", 20}, {"high", 100}, {"low", 1} };

#define COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
const SettingDef SETTINGS[] = {
  { "edge",    "trig_edge",   &trigEdge,   EDGE_CHOICES,    COUNT_OF(EDGE_CHOICES)    },
  { "repeat",  "trig_repeat", &trigRepeat, REPEAT_CHOICES,  COUNT_OF(REPEAT_CHOICES)  },
  { "pressed", "led_pressed", &ledPressed, PRESSED_CHOICES, COUNT_OF(PRESSED_CHOICES) },
  { "idle",    "led_idle",    &ledIdle,    IDLE_CHOICES,    COUNT_OF(IDLE_CHOICES)    },
  { "rgb",     "rgb_bright",  &rgbPercent, RGB_CHOICES,     COUNT_OF(RGB_CHOICES)     },
};

// ─── WebSocket server (only for WiFi mode) ─────────────────────────────────
#if WIFI_ENABLED
AsyncWebServer wsHttpServer(80);
AsyncWebSocket ws("/");
#endif

// ─── State machine ─────────────────────────────────────────────────────────
enum DeviceState {
  STATE_BOOT_OK,           // Blink blue  — no USB host (charger/power only), no primary yet
  STATE_SERIAL_CONNECTED,  // Solid green — browser connected via Web Serial (= primary, serial open)
#if WIFI_ENABLED
  STATE_WIFI_CONNECTING,   // Blink blue   — waiting for WiFi association
  STATE_WS_WAITING,        // Alternate purple/blue — WiFi up, waiting for WS client
  STATE_WS_READY,          // Solid blue   — WiFi + WebSocket client connected
  STATE_WIFI_FAILED,       // Blink red 10x — WiFi failed, then revert to green
#else
  STATE_USB_HOST,          // Solid green — USB host enumerated, primary role active, no serial yet
  STATE_HOST_AWAY,         // Blink green — hosted, host asleep/unplugged, waiting for it
  STATE_RELAY_READY,       // Solid blue  — remote device, knows primary, ready to relay
#endif
  STATE_ERROR              // Blink red   — unrecoverable error (held until reboot)
};
DeviceState currentState = STATE_BOOT_OK;

// ─── Configurable HID key (persisted in NVS flash) ─────────────────────────
const char     DEFAULT_KEY   = '1';
char           hidKey        = DEFAULT_KEY;
Preferences    prefs;

// ─── Globals ───────────────────────────────────────────────────────────────
// Buttons (active LOW). See pollButton().
struct Button {
  uint8_t       pin;
  bool          pressed;      // state at the last poll
  unsigned long lastPollMs;
};
enum ButtonEdge { BTN_NONE, BTN_PRESSED, BTN_RELEASED };

Button        triggerBtn     = { TRIGGER_BTN_PIN, false, 0 };
Button        keyBtn         = { KEY_BTN_PIN,     false, 0 };
int           seqCounter     = 0;
bool          pendingPress   = false;   // trigger/BOOT press during the boot USB wait; sent once the path is known
bool          pendingHold    = false;   // ...and whether it asked to hold the key

// BOOT hold towards settings mode. Tracked from a press edge only, so a BOOT
// still held through a restart never counts.
bool          keyHoldTracked = false;
unsigned long keyHoldStartMs = 0;
char          keyBeforeHold  = DEFAULT_KEY;  // restored when the hold enters settings mode

// HID output: the host sees one key at a time. A local press is either held
// while the trigger is down (repeat) or a HID_HOLD_MS tap, and cuts a relayed
// tap short; relayed taps queue behind it.
char          hidLocalKey    = 0;       // key of the local press in progress; 0 = none
bool          hidLocalHold   = false;   // held until the trigger is up, else a tap
unsigned long hidLocalPressMs = 0;
char          hidSentKey     = 0;       // key in the last report sent; 0 = all up. A release is owed while set
char          relayKey       = 0;       // relayed tap in progress; 0 = none
unsigned long relayPressMs   = 0;
const int     RELAY_TAP_QUEUE_LEN = 8;
char          relayTapQueue[RELAY_TAP_QUEUE_LEN];
int           relayTapHead   = 0;
int           relayTapCount  = 0;

// Status LED
bool          statusLedReady   = false;
uint8_t       ledShownPattern  = LED_OFF; // pattern being shown; a change restarts its cycle
unsigned long ledPhaseStartMs  = 0;       // start of the current pattern cycle
uint32_t      ledLastDuty      = 0xFFFFFFFFUL;

// RGB status LED: blink phase, press feedback, write cache
unsigned long lastBlinkMs    = 0;
bool          blinkPhase     = false;
bool          pressDelivered = false;   // white (delivered) vs red (not delivered) while a button is held
bool          releaseFeedback   = false; // edge=release: feedback shown for PRESS_FEEDBACK_MS from releaseFeedbackMs
unsigned long releaseFeedbackMs = 0;
unsigned long lastRainbowMs  = 0;
uint32_t      rgbShown       = 0xFFFFFFFFUL;  // last packed colour written; invalid until first write

// USB host tracking (polled from TinyUSB in handleHostEnumeration())
bool          usbHostReady   = false;  // mounted and not suspended: HID reports can be delivered
bool          usbHostSeen    = false;  // a host enumerated us this uptime (latched — never cleared)
bool          usbWaitActive  = true;   // boot window: still learning whether a host is attached
unsigned long usbWaitStartMs = 0;
const unsigned long USB_ENUM_WAIT_MS = 4000;

// Serial line buffer (fixed-size to avoid heap fragmentation)
char          serialLineBuf[258];  // 256 data + margin + null
size_t        serialLineBufLen = 0;
bool          serialWasOpen  = false;

// ─── ESP-NOW common ────────────────────────────────────────────────────────
static const uint32_t ESPNOW_MAGIC   = 0x51545059;  // "QTPY" — kept so Zero, Matrix and QT Py units interoperate
static const uint8_t  ESPNOW_CHANNEL = 1;            // agreed broadcast channel
static const uint8_t  ESPNOW_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool espNowInitialized = false;


// ─── ESP-NOW packet types ──────────────────────────────────────────────────
#define ESPNOW_PKT_ANNOUNCE   0x01
#define ESPNOW_PKT_KEYSTROKE  0x02

// ─── Color helpers ─────────────────────────────────────────────────────────
struct RGB { uint8_t r, g, b; };

const RGB COLOR_GREEN  = {0,   255, 0  };
const RGB COLOR_BLUE   = {0,   0,   255};
const RGB COLOR_YELLOW = {255, 200, 0  };
const RGB COLOR_RED    = {255, 0,   0  };
const RGB COLOR_PURPLE = {160, 0,   255};
const RGB COLOR_WHITE  = {200, 200, 200};
const RGB COLOR_OFF    = {0,   0,   0  };

// Scales one colour channel to the "rgb" brightness setting, rounded so faint
// channels at low brightness don't all drop to 0.
uint8_t dimChannel(uint8_t v) {
  return (v * rgbPercent + 50) / 100;
}

// Set the onboard RGB LED to colour c, scaled to the "rgb" brightness setting.
// Only writes when the colour sent to the LED changes.
void showStatus(RGB c) {
  uint8_t r = dimChannel(c.r), g = dimChannel(c.g), b = dimChannel(c.b);
  uint32_t packed = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  if (packed == rgbShown) return;
  rgbShown = packed;
  rgbLedWriteOrdered(STATUS_RGB_PIN, STATUS_RGB_ORDER, r, g, b);
}

// ─── State management ──────────────────────────────────────────────────────
void setState(DeviceState s) {
  currentState = s;
  lastBlinkMs  = 0;
  blinkPhase   = false;
  LOG_PRINTF("LOG:state=%d ts=%lu\n", (int)s, millis());
}

// ─── HID output ────────────────────────────────────────────────────────────
// Map a key label (a–z, 0–9) to a keyboard report. No Shift: letters type lower
// case. Upper-case labels (relayed from older units) map to the same keys.
bool buildKeyReport(char key, hid_keyboard_report_t &report) {
  memset(&report, 0, sizeof(report));
  key = toLowerCase(key);
  if (key >= 'a' && key <= 'z') {
    report.keycode[0] = HID_KEY_A + (key - 'a');
  } else if (key >= '1' && key <= '9') {
    report.keycode[0] = HID_KEY_1 + (key - '1');
  } else if (key == '0') {
    report.keycode[0] = HID_KEY_0;
  } else {
    return false;
  }
  return true;
}

// Send a report holding key, or releasing all keys when key is 0. Returns true
// only if the report reached the host.
bool sendKeyReport(char key) {
  hid_keyboard_report_t report = {};
  if (key && !buildKeyReport(key, report)) return false;
  // A key-down that times out may still reach the host, so a release is owed from the attempt on.
  if (key) hidSentKey = key;
  if (!hid->SendReport(HID_REPORT_ID_KEYBOARD, &report, sizeof(report))) return false;
  hidSentKey = key;
  return true;
}

// Press a local key now. handleHidOutput() releases it when the trigger is up
// (hold) or after HID_HOLD_MS (tap). A key still down is released first so the
// host always sees a fresh key-down, even when the same key is pressed twice.
bool hidPress(char key, bool hold) {
  if (!usbHostReady) return false;
  relayKey        = 0;
  hidLocalKey     = key;
  hidLocalHold    = hold;
  hidLocalPressMs = millis();
  if (hidSentKey) sendKeyReport(0);
  return sendKeyReport(key);
}

void queueRelayTap(char key) {
  if (relayTapCount >= RELAY_TAP_QUEUE_LEN) {
    LOG_PRINTF("LOG:RELAY_TAP_DROP key=%c ts=%lu\n", key, millis());
    return;
  }
  relayTapQueue[(relayTapHead + relayTapCount) % RELAY_TAP_QUEUE_LEN] = key;
  relayTapCount++;
}

// Brings the host's key state to what it should be: the local press while it
// lasts, otherwise the current relayed tap, otherwise all keys up. Retried every
// loop until it lands: a lost release would leave the key auto-repeating on the host.
void handleHidOutput() {
  if (hidLocalKey && (hidLocalHold ? !triggerBtn.pressed
                                   : millis() - hidLocalPressMs >= HID_HOLD_MS)) {
    hidLocalKey = 0;
  }

  if (!hidLocalKey) {
    if (relayKey && hidSentKey == relayKey && millis() - relayPressMs >= HID_HOLD_MS) relayKey = 0;
    // Next tap only after a release, so repeated keys register as separate presses
    if (!relayKey && !hidSentKey && relayTapCount > 0 && usbHostReady) {
      relayKey     = relayTapQueue[relayTapHead];
      relayTapHead = (relayTapHead + 1) % RELAY_TAP_QUEUE_LEN;
      relayTapCount--;
      relayPressMs = millis();
    }
  }

  char want = hidLocalKey ? hidLocalKey : relayKey;
  if (want != hidSentKey && usbHostReady) sendKeyReport(want);
}

// ============================================================================
// WiFi / WebSocket mode (WIFI_ENABLED=1)
// ============================================================================
#if WIFI_ENABLED

// WiFi failure animation
int wifiFailBlinkCount = 0;

void startWiFi(const String &ssid, const String &pass);  // defined below, used by credential handlers

// WiFi
String        wifiSSID       = "";
String        wifiPass       = "";
unsigned long wifiStartMs    = 0;
const unsigned long WIFI_TIMEOUT_MS = 20000;
bool          wifiAutoStarted = false;

// ─── ESP-NOW credential broadcast (WiFi mode only) ────────────────────────
static const uint8_t ESPNOW_AES_KEY[16] = {
  0x4D, 0x79, 0x51, 0x54, 0x50, 0x59, 0x4B, 0x65,
  0x79, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
};

struct __attribute__((packed)) EspNowCredPayload {
  char    ssid[33];
  char    password[64];
  uint8_t channel;
  uint8_t checksum;
};

struct __attribute__((packed)) EspNowCredPacket {
  uint32_t magic;
  uint8_t  iv[16];
  uint8_t  encrypted[112];
};

volatile bool espNowCredsReady = false;
String        espNowRxSSID     = "";
String        espNowRxPass     = "";

// ─── ESP-NOW encryption helpers ──────────────────────────────────────────────
uint8_t computeChecksum(const char *ssid, const char *password) {
  uint8_t ck = 0;
  for (const char *p = ssid; *p; p++) ck ^= (uint8_t)*p;
  for (const char *p = password; *p; p++) ck ^= (uint8_t)*p;
  return ck;
}

bool encryptCredentials(const String &ssid, const String &pass, uint8_t channel, EspNowCredPacket &pkt) {
  EspNowCredPayload plain = {};
  strncpy(plain.ssid, ssid.c_str(), sizeof(plain.ssid) - 1);
  strncpy(plain.password, pass.c_str(), sizeof(plain.password) - 1);
  plain.channel = channel;
  plain.checksum = computeChecksum(plain.ssid, plain.password);

  uint8_t padded[112] = {};
  memcpy(padded, &plain, sizeof(plain));
  uint8_t padLen = 112 - sizeof(plain);
  memset(padded + sizeof(plain), padLen, padLen);

  pkt.magic = ESPNOW_MAGIC;
  uint32_t *ivWords = (uint32_t *)pkt.iv;
  for (int i = 0; i < 4; i++) ivWords[i] = esp_random();

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, ESPNOW_AES_KEY, 128);
  uint8_t ivCopy[16];
  memcpy(ivCopy, pkt.iv, 16);
  int ret = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 112, ivCopy, padded, pkt.encrypted);
  mbedtls_aes_free(&aes);
  return (ret == 0);
}

bool decryptCredentials(const EspNowCredPacket &pkt, EspNowCredPayload &plain) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, ESPNOW_AES_KEY, 128);
  uint8_t ivCopy[16];
  memcpy(ivCopy, pkt.iv, 16);
  uint8_t decrypted[112] = {};
  int ret = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 112, ivCopy, pkt.encrypted, decrypted);
  mbedtls_aes_free(&aes);
  if (ret != 0) return false;
  memcpy(&plain, decrypted, sizeof(plain));
  uint8_t expected = computeChecksum(plain.ssid, plain.password);
  return (plain.checksum == expected);
}

// ─── ESP-NOW credential receive callback (WiFi mode) ─────────────────────
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
#else
void onEspNowReceive(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len != sizeof(EspNowCredPacket)) return;
  const EspNowCredPacket *pkt = (const EspNowCredPacket *)data;
  if (pkt->magic != ESPNOW_MAGIC) return;

  EspNowCredPayload plain;
  if (!decryptCredentials(*pkt, plain)) {
    LOG_PRINTLN("LOG:ESPNOW_DECRYPT_FAIL");
    return;
  }

  if (WiFi.status() == WL_CONNECTED &&
      String(plain.ssid) == wifiSSID &&
      String(plain.password) == wifiPass) {
    LOG_PRINTLN("LOG:ESPNOW_RX_DUPLICATE");
    return;
  }

  espNowRxSSID = String(plain.ssid);
  espNowRxPass = String(plain.password);
  espNowCredsReady = true;
  LOG_PRINTF("LOG:ESPNOW_RX ssid=%s ch=%d ts=%lu\n", plain.ssid, plain.channel, millis());
}

// ─── ESP-NOW credential broadcast ──────────────────────────────────────────
void broadcastCredentials() {
  if (!espNowInitialized) return;
  if (wifiSSID.length() == 0) return;

  uint8_t apChannel = WiFi.channel();
  LOG_PRINTF("LOG:ESPNOW_BCAST_START ap_ch=%d ts=%lu\n", apChannel, millis());
  PROTO_PRINTF("ESPNOW_BROADCASTING:%s\n", wifiSSID.c_str());

  ws.closeAll();
  wsHttpServer.end();
  WiFi.disconnect(false);
  delay(50);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  delay(10);

  EspNowCredPacket pkt;
  if (!encryptCredentials(wifiSSID, wifiPass, apChannel, pkt)) {
    if ((bool)Cdc) Cdc.println("ERR:ESPNOW_ENCRYPT_FAILED");
    startWiFi(wifiSSID, wifiPass);
    return;
  }

  for (int i = 0; i < 3; i++) {
    esp_err_t result = esp_now_send(ESPNOW_BROADCAST, (uint8_t *)&pkt, sizeof(pkt));
    if (result != ESP_OK) {
      LOG_PRINTF("LOG:ESPNOW_SEND_FAIL attempt=%d err=%d\n", i, result);
    }
    if (i < 2) delay(50);
  }

  LOG_PRINTF("LOG:ESPNOW_BCAST_DONE ts=%lu\n", millis());
  PROTO_PRINTLN("ESPNOW_BROADCAST_DONE");
  startWiFi(wifiSSID, wifiPass);
}

// ─── ESP-NOW handler for credential receives (WiFi mode) ─────────────────
void handleEspNow() {
  if (espNowCredsReady) {
    espNowCredsReady = false;

    wifiSSID = espNowRxSSID;
    wifiPass = espNowRxPass;
    prefs.begin("remote", false);
    prefs.putString("wifi_ssid", wifiSSID);
    prefs.putString("wifi_pass", wifiPass);
    prefs.end();

    PROTO_PRINTF("ESPNOW_RECEIVED:%s\n", wifiSSID.c_str());
    startWiFi(wifiSSID, wifiPass);
  }
}

// ─── WebSocket event handler ────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket       *server,
               AsyncWebSocketClient *client,
               AwsEventType          type,
               void                 *arg,
               uint8_t              *data,
               size_t                len)
{
  switch (type) {
    case WS_EVT_CONNECT:
      if (client->client()) client->client()->setNoDelay(true);
      client->text("HELLO");
      LOG_PRINTF("LOG:WS_CONNECT id=%u ts=%lu\n", client->id(), millis());
      if (currentState == STATE_WS_WAITING) setState(STATE_WS_READY);
      break;
    case WS_EVT_DISCONNECT:
      LOG_PRINTF("LOG:WS_DISCONNECT id=%u ts=%lu\n", client->id(), millis());
      if (currentState == STATE_WS_READY && ws.count() <= 1) setState(STATE_WS_WAITING);
      break;
    case WS_EVT_ERROR:
      if ((bool)Cdc) {
        Cdc.printf("ERR:WS id=%u ts=%lu\n", client->id(), millis());
      }
      break;
    case WS_EVT_DATA: {
      char buf[64];
      size_t copyLen = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
      memcpy(buf, data, copyLen);
      buf[copyLen] = '\0';
      client->text("ECHO:" + String(buf));
      break;
    }
    default:
      break;
  }
}

// ─── Start WebSocket server ─────────────────────────────────────────────────
bool wsServerStarted = false;

void startWebSocket() {
  if (!wsServerStarted) {
    ws.onEvent(onWsEvent);
    wsHttpServer.addHandler(&ws);
    wsServerStarted = true;
  }
  wsHttpServer.begin();

  String url = "ws://" + WiFi.localIP().toString();
  PROTO_PRINTF("WS_STARTED:%s\n", url.c_str());
  LOG_PRINTF("LOG:WS_UP url=%s ts=%lu heap=%u\n",
             url.c_str(), millis(), ESP.getFreeHeap());
  setState(STATE_WS_WAITING);
}

// ─── WiFi connected ────────────────────────────────────────────────────────
void onWiFiConnected() {
  esp_wifi_set_ps(WIFI_PS_NONE);

  String ip = WiFi.localIP().toString();
  PROTO_PRINTF("WIFI_CONNECTED:%s\n", ip.c_str());
  LOG_PRINTF("LOG:WIFI_UP ip=%s rssi=%d ch=%d ts=%lu\n",
             ip.c_str(), WiFi.RSSI(), WiFi.channel(), millis());

  static String lastBroadcastSSID = "";
  static String lastBroadcastPass = "";
  if (wifiSSID != lastBroadcastSSID || wifiPass != lastBroadcastPass) {
    lastBroadcastSSID = wifiSSID;
    lastBroadcastPass = wifiPass;
    broadcastCredentials();
    return;
  }

  startWebSocket();
}

// ─── WiFi status to readable string ─────────────────────────────────────────
const char* wifiStatusStr(wl_status_t s) {
  switch (s) {
    case WL_IDLE_STATUS:     return "IDLE";
    case WL_NO_SSID_AVAIL:  return "NO_SSID";
    case WL_SCAN_COMPLETED:  return "SCAN_DONE";
    case WL_CONNECTED:       return "CONNECTED";
    case WL_CONNECT_FAILED:  return "CONNECT_FAIL";
    case WL_CONNECTION_LOST: return "CONN_LOST";
    case WL_DISCONNECTED:    return "DISCONNECTED";
    default:                 return "UNKNOWN";
  }
}

// ─── Start WiFi ────────────────────────────────────────────────────────────
void startWiFi(const String &ssid, const String &pass) {
  PROTO_PRINTLN("WIFI_CONNECTING");
  LOG_PRINTF("LOG:WIFI_BEGIN ssid=%s ts=%lu\n", ssid.c_str(), millis());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  delay(50);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());
  wifiStartMs = millis();
  setState(STATE_WIFI_CONNECTING);
}

// ─── WiFi polling (non-blocking) ────────────────────────────────────────────
void handleWiFi() {
  static unsigned long lastCheck = 0;
  static unsigned long lastDiagMs = 0;
  unsigned long now = millis();
  if (now - lastCheck < 50) return;
  lastCheck = now;

  if (currentState == STATE_WIFI_CONNECTING) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
      lastDiagMs = 0;
      onWiFiConnected();
      return;
    }
    if (now - lastDiagMs >= 3000) {
      lastDiagMs = now;
      LOG_PRINTF("LOG:WIFI_STATUS status=%s(%d) elapsed=%lus ts=%lu\n",
                 wifiStatusStr(st), (int)st,
                 (now - wifiStartMs) / 1000, millis());
    }
    if (now - wifiStartMs > WIFI_TIMEOUT_MS) {
      lastDiagMs = 0;
      PROTO_PRINTF("WIFI_FAILED:ssid=%s status=%s(%d)\n",
                   wifiSSID.c_str(), wifiStatusStr(st), (int)st);
      WiFi.disconnect(true);
      esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
      wifiFailBlinkCount = 0;
      setState(STATE_WIFI_FAILED);
    }
    return;
  }

  if (currentState == STATE_WIFI_FAILED && wifiFailBlinkCount >= 10) {
    setState(serialWasOpen ? STATE_SERIAL_CONNECTED : STATE_BOOT_OK);
    return;
  }

  if ((currentState == STATE_WS_READY || currentState == STATE_WS_WAITING) && WiFi.status() != WL_CONNECTED) {
    LOG_PRINTF("LOG:WIFI_DROP ts=%lu — reconnecting\n", millis());
    ws.closeAll();
    wsHttpServer.end();
    startWiFi(wifiSSID, wifiPass);
  }
}

// ─── WebSocket message sender ───────────────────────────────────────────────
void sendWsMessage(const char *msg) {
  if (currentState != STATE_WS_READY || ws.count() == 0) return;
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 20000) {
    if ((bool)Cdc) Cdc.printf("ERR:LOW_HEAP heap=%u\n", freeHeap);
    return;
  }
  ws.textAll(msg);
}

#endif // WIFI_ENABLED

// ============================================================================
// ESP-NOW relay mode (WIFI_ENABLED=0)
// ============================================================================
#if !WIFI_ENABLED

// ─── ESP-NOW relay packet structures ────────────────────────────────────────
struct __attribute__((packed)) EspNowAnnounce {
  uint32_t magic;     // ESPNOW_MAGIC
  uint8_t  type;      // ESPNOW_PKT_ANNOUNCE
  char     keyLabel;  // primary's configured HID key
  uint8_t  seq;       // incrementing sequence (detect primary change)
};
// sizeof = 7 bytes

struct __attribute__((packed)) EspNowKeystroke {
  uint32_t magic;     // ESPNOW_MAGIC
  uint8_t  type;      // ESPNOW_PKT_KEYSTROKE
  char     keyLabel;  // sender's configured HID key
  uint16_t seq;       // per-device sequence counter (dedup on retry)
};
// sizeof = 8 bytes

// ─── Relay state ────────────────────────────────────────────────────────────
// The primary role is usbHostSeen; remote duties run only on never-hosted units.
uint8_t       primaryMac[6]   = {0};
bool          hasPrimary      = false;
bool          primaryPeerRegistered = false;  // true if primary MAC registered as unicast peer

unsigned long lastAnnounceMs  = 0;
unsigned long lastPrimarySeenMs = 0;
const unsigned long ANNOUNCE_INTERVAL_MS = 5000;   // heartbeat every 5s
const unsigned long PRIMARY_TIMEOUT_MS   = 15000;  // 3 missed heartbeats = primary gone

uint8_t  announceSeq   = 0;
uint16_t keystrokeSeq  = 0;

// Non-blocking announce burst (replaces blocking delay loop)
uint8_t       announceBurstRemaining = 0;
unsigned long announceBurstNextMs    = 0;
const unsigned long ANNOUNCE_BURST_GAP_MS = 5;

// Send callback state — FreeRTOS queue for thread-safe status reporting
QueueHandle_t sendStatusQueue = NULL;  // depth 1, holds esp_now_send_status_t
bool          pendingRetry = false;
unsigned long retryAfterMs = 0;
EspNowKeystroke retryPacket;

// ─── Dedup table (primary side) ─────────────────────────────────────────────
// Track last sequence number per source MAC to reject duplicates from retries
struct RemotePeer {
  uint8_t  mac[6];
  char     keyLabel;
  uint16_t lastSeq;
  unsigned long lastMs;
};
static const int MAX_REMOTE_PEERS = 50;
RemotePeer remotePeers[MAX_REMOTE_PEERS];
int        remotePeerCount = 0;

// Own MAC address (cached at init for self-announce filtering)
uint8_t ownMacAddr[6] = {0};

// ─── FreeRTOS queues for ESP-NOW receive (replaces volatile flag pattern) ────
// Queues provide memory barriers + buffering for burst traffic from 50+ devices.
struct RelayRxItem {
  uint8_t  mac[6];
  char     keyLabel;
  uint16_t seq;
};

struct AnnounceRxItem {
  uint8_t mac[6];
  char    keyLabel;
  uint8_t seq;
};

QueueHandle_t relayRxQueue    = NULL;  // keystroke receives (depth 8)
QueueHandle_t announceRxQueue = NULL;  // announce receives (depth 4)

// ─── Find or create remote peer entry ───────────────────────────────────────
int findRemotePeer(const uint8_t *mac) {
  for (int i = 0; i < remotePeerCount; i++) {
    if (memcmp(remotePeers[i].mac, mac, 6) == 0) return i;
  }
  return -1;
}

int addRemotePeer(const uint8_t *mac, char keyLabel) {
  if (remotePeerCount >= MAX_REMOTE_PEERS) return -1;
  int idx = remotePeerCount++;
  memcpy(remotePeers[idx].mac, mac, 6);
  remotePeers[idx].keyLabel = keyLabel;
  remotePeers[idx].lastSeq = 0xFFFF;  // no valid seq yet
  remotePeers[idx].lastMs = millis();
  return idx;
}

// ─── ESP-NOW send callback (relay mode) ─────────────────────────────────────
void onEspNowSendRelay(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  xQueueOverwriteFromISR(sendStatusQueue, &status, NULL);
}

// ─── ESP-NOW receive callback (relay mode) ──────────────────────────────────
// Called from WiFi task context — must be fast, non-blocking.
// Enqueues items for loop() to process via FreeRTOS queues.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
void onEspNowReceiveRelay(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
#else
void onEspNowReceiveRelay(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len < 5) return;  // minimum: magic(4) + type(1)

  const uint32_t *magic = (const uint32_t *)data;
  if (*magic != ESPNOW_MAGIC) return;

  uint8_t pktType = data[4];

  if (pktType == ESPNOW_PKT_ANNOUNCE && len >= (int)sizeof(EspNowAnnounce)) {
    const EspNowAnnounce *ann = (const EspNowAnnounce *)data;
    if (memcmp(mac, ownMacAddr, 6) == 0) return;

    AnnounceRxItem item;
    memcpy(item.mac, mac, 6);
    item.keyLabel = ann->keyLabel;
    item.seq = ann->seq;
    xQueueSendFromISR(announceRxQueue, &item, NULL);
    return;
  }

  if (pktType == ESPNOW_PKT_KEYSTROKE && len >= (int)sizeof(EspNowKeystroke)) {
    const EspNowKeystroke *ks = (const EspNowKeystroke *)data;
    RelayRxItem item;
    memcpy(item.mac, mac, 6);
    item.keyLabel = ks->keyLabel;
    item.seq = ks->seq;
    xQueueSendFromISR(relayRxQueue, &item, NULL);
    return;
  }
}

// ─── Send ANNOUNCE broadcast ────────────────────────────────────────────────
void sendAnnounce() {
  if (!espNowInitialized) return;
  EspNowAnnounce pkt;
  pkt.magic = ESPNOW_MAGIC;
  pkt.type = ESPNOW_PKT_ANNOUNCE;
  pkt.keyLabel = hidKey;
  pkt.seq = announceSeq++;
  esp_now_send(ESPNOW_BROADCAST, (uint8_t *)&pkt, sizeof(pkt));
  LOG_PRINTF("LOG:ANNOUNCE seq=%d ts=%lu\n", pkt.seq, millis());
}

// ─── Send KEYSTROKE unicast to primary ──────────────────────────────────────
// Returns true if the packet was handed to the radio.
bool sendKeystroke() {
  if (!espNowInitialized || !hasPrimary || !primaryPeerRegistered) return false;
  keystrokeSeq++;
  EspNowKeystroke pkt;
  pkt.magic = ESPNOW_MAGIC;
  pkt.type = ESPNOW_PKT_KEYSTROKE;
  pkt.keyLabel = hidKey;
  pkt.seq = keystrokeSeq;

  esp_err_t err = esp_now_send(primaryMac, (uint8_t *)&pkt, sizeof(pkt));
  if (err != ESP_OK) {
    LOG_PRINTF("LOG:KS_SEND_ERR err=%d ts=%lu\n", err, millis());
  }

  // Store for potential retry (checked in handleRelay)
  retryPacket = pkt;
  pendingRetry = false;  // will be set true in handleRelay if send callback reports failure
  LOG_PRINTF("LOG:KS_SENT key=%c seq=%u ts=%lu\n", hidKey, keystrokeSeq, millis());
  return err == ESP_OK;
}

// ─── Register primary as unicast peer ───────────────────────────────────────
void registerPrimaryPeer(const uint8_t *mac) {
  // Remove old primary peer if different
  if (primaryPeerRegistered) {
    if (memcmp(primaryMac, mac, 6) == 0) return;  // same peer, no change
    esp_now_del_peer(primaryMac);
    primaryPeerRegistered = false;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;      // use current channel
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    memcpy(primaryMac, mac, 6);
    primaryPeerRegistered = true;
    LOG_PRINTF("LOG:PRIMARY_PEER_REG ts=%lu\n", millis());
  } else {
    LOG_PRINTLN("ERR:PRIMARY_PEER_REG_FAIL");
  }
}

// ─── Handle relay logic (called from loop) ──────────────────────────────────
void handleRelay() {
  unsigned long now = millis();
  // Primary duties need a host that can take keystrokes right now. While a hosted
  // unit's host is away it stays silent, so remotes time out and find another hub.
  bool hubActive = usbHostSeen && usbHostReady;

  // ── Primary duties ──
  if (hubActive) {
    // Non-blocking announce burst (3 rapid sends with 5ms gaps)
    if (announceBurstRemaining > 0 && now >= announceBurstNextMs) {
      sendAnnounce();
      announceBurstRemaining--;
      announceBurstNextMs = now + ANNOUNCE_BURST_GAP_MS;
      if (announceBurstRemaining == 0) {
        lastAnnounceMs = now;  // reset heartbeat timer
      }
    }

    // Broadcast ANNOUNCE heartbeat
    if (now - lastAnnounceMs >= ANNOUNCE_INTERVAL_MS) {
      lastAnnounceMs = now;
      sendAnnounce();
    }
  }

  // Process received KEYSTROKEs from remotes (drain all queued). Ones that arrive
  // while no host can take them are dropped, not replayed when the host returns.
  RelayRxItem rxItem;
  while (xQueueReceive(relayRxQueue, &rxItem, 0) == pdTRUE) {
    if (!hubActive) continue;
    char rxKey = rxItem.keyLabel;
    uint16_t rxSeq = rxItem.seq;

    // Dedup: find or create remote peer entry
    int idx = findRemotePeer(rxItem.mac);
    bool isNewPeer = (idx < 0);
    if (isNewPeer) {
      idx = addRemotePeer(rxItem.mac, rxKey);
      if (idx >= 0) {
        PROTO_PRINTF("PEER:%c\n", rxKey);
        PROTO_PRINTF("PEERS:%d\n", remotePeerCount);
      }
    }

    // Dedup check: reject if same seq as last from this peer
    bool isDuplicate = false;
    if (idx >= 0) {
      if (remotePeers[idx].lastSeq == rxSeq) {
        isDuplicate = true;
        LOG_PRINTF("LOG:KS_DEDUP key=%c seq=%u ts=%lu\n", rxKey, rxSeq, millis());
      } else {
        remotePeers[idx].lastSeq = rxSeq;
        remotePeers[idx].lastMs = now;
      }
    }

    if (!isDuplicate) {
      // Relay to browser via Serial
      PROTO_PRINTF("RELAY:%c\n", rxKey);
      // Typed by handleHidOutput(), behind any local trigger press
      queueRelayTap(rxKey);
      LOG_PRINTF("LOG:RELAY key=%c seq=%u ts=%lu\n", rxKey, rxSeq, millis());
    }
  }

  // Hosted units never send over ESP-NOW: other hubs' announces are irrelevant.
  if (usbHostSeen) {
    xQueueReset(announceRxQueue);
    return;
  }

  // ── Remote duties (never hosted this uptime) ──
  // Process ANNOUNCEs from primary (drain all queued)
  AnnounceRxItem annItem;
  while (xQueueReceive(announceRxQueue, &annItem, 0) == pdTRUE) {
    bool primaryChanged = !hasPrimary || memcmp(primaryMac, annItem.mac, 6) != 0;

    registerPrimaryPeer(annItem.mac);
    hasPrimary = true;
    lastPrimarySeenMs = now;

    if (primaryChanged) {
      LOG_PRINTF("LOG:PRIMARY_FOUND key=%c ts=%lu\n", annItem.keyLabel, millis());
      if (currentState == STATE_BOOT_OK) {
        setState(STATE_RELAY_READY);
      }
    }
  }

  // Primary timeout detection
  if (hasPrimary && (now - lastPrimarySeenMs > PRIMARY_TIMEOUT_MS)) {
    hasPrimary = false;
    if (primaryPeerRegistered) {
      esp_now_del_peer(primaryMac);
      primaryPeerRegistered = false;
    }
    LOG_PRINTLN("LOG:PRIMARY_LOST");
    if (currentState == STATE_RELAY_READY) {
      setState(STATE_BOOT_OK);
    }
  }

  // Retry failed keystroke send (single retry with random delay)
  if (pendingRetry && now >= retryAfterMs) {
    pendingRetry = false;
    if (hasPrimary && primaryPeerRegistered) {
      esp_now_send(primaryMac, (uint8_t *)&retryPacket, sizeof(retryPacket));
      LOG_PRINTF("LOG:KS_RETRY seq=%u ts=%lu\n", retryPacket.seq, millis());
    }
  }

  // ── Check send callback for keystroke retry scheduling ──
  // (runs for remotes only, after a keystroke was sent)
  esp_now_send_status_t sendStatus;
  if (!pendingRetry &&
      xQueueReceive(sendStatusQueue, &sendStatus, 0) == pdTRUE &&
      sendStatus != ESP_NOW_SEND_SUCCESS) {
    pendingRetry = true;
    retryAfterMs = now + 2 + (esp_random() % 7);
  }
}

#endif // !WIFI_ENABLED

// ============================================================================
// Common code — LED, Serial, Button, Setup, Loop
// ============================================================================

// ─── RGB status LED handler (non-blocking, called every loop) ──────────────
// Toggles blinkPhase every periodMs and returns it.
bool blinkEvery(unsigned long periodMs) {
  unsigned long now = millis();
  if (now - lastBlinkMs >= periodMs) {
    lastBlinkMs = now;
    blinkPhase  = !blinkPhase;
  }
  return blinkPhase;
}

// Colour wheel at full saturation and brightness; pos runs 0..1535 (six 256-step ramps).
RGB colorWheel(uint16_t pos) {
  uint8_t f = pos & 0xFF;
  switch (pos >> 8) {
    case 0:  return {255,           f,             0            };
    case 1:  return {(uint8_t)~f,   255,           0            };
    case 2:  return {0,             255,           f            };
    case 3:  return {0,             (uint8_t)~f,   255          };
    case 4:  return {f,             0,             255          };
    default: return {255,           0,             (uint8_t)~f  };
  }
}

// True while the RGB LED should show white/red press feedback. A trigger set to
// type on release has sent nothing while held, so it gets a short flash after
// the release instead. BOOT counts from its press edge (keyHoldTracked), not
// while still held through a restart. The release flash is cleared once over, so
// an old timestamp never comes back into range when millis() wraps.
bool pressFeedbackActive(unsigned long now) {
  if (releaseFeedback && now - releaseFeedbackMs >= PRESS_FEEDBACK_MS) releaseFeedback = false;
  bool trigger = triggerBtn.pressed && trigEdge == EDGE_PRESS;
  return trigger || keyHoldTracked || releaseFeedback;
}

// Shows the state colour, with press feedback on top while a button is held.
void handleStatusRgb() {
  unsigned long now = millis();

  // Settings mode: the rainbow, whatever else happens
  if (settingsMode) {
    if (now - lastRainbowMs < RAINBOW_FRAME_MS) return;
    lastRainbowMs = now;
    unsigned long t = (now - settingsStartMs) % RAINBOW_PERIOD_MS;
    showStatus(colorWheel((RAINBOW_START_POS + t * 1536 / RAINBOW_PERIOD_MS) % 1536));
    return;
  }

  // Button press feedback: white = delivered, red = nowhere to go. During the
  // boot window the press is still pending, so booting stays visible.
  if (pressFeedbackActive(now) && !usbWaitActive) {
    showStatus(pressDelivered ? COLOR_WHITE : COLOR_RED);
    return;
  }

  // Errors win over the boot window so a failed start shows at once
  if (currentState == STATE_ERROR) {
    showStatus(blinkEvery(250) ? COLOR_RED : COLOR_OFF);
    return;
  }

  // Boot window: still waiting to learn whether a host is attached
  if (usbWaitActive) {
    showStatus((now / 150) % 2 ? COLOR_YELLOW : COLOR_OFF);
    return;
  }

  switch (currentState) {
    case STATE_BOOT_OK:
      showStatus(blinkEvery(500) ? COLOR_BLUE : COLOR_OFF);
      break;

    case STATE_SERIAL_CONNECTED:
      showStatus(COLOR_GREEN);
      break;

#if WIFI_ENABLED
    case STATE_WIFI_CONNECTING:
      showStatus(blinkEvery(500) ? COLOR_BLUE : COLOR_OFF);
      break;

    case STATE_WS_WAITING:
      showStatus(blinkEvery(500) ? COLOR_PURPLE : COLOR_BLUE);
      break;

    case STATE_WS_READY:
      showStatus(COLOR_BLUE);
      break;

    case STATE_WIFI_FAILED:
      if (wifiFailBlinkCount < 10) {
        if (now - lastBlinkMs >= 250) {
          lastBlinkMs = now;
          blinkPhase  = !blinkPhase;
          if (!blinkPhase) wifiFailBlinkCount++;
        }
        showStatus(blinkPhase ? COLOR_RED : COLOR_OFF);
      }
      break;
#else
    case STATE_USB_HOST:
      showStatus(COLOR_GREEN);
      break;

    case STATE_HOST_AWAY:
      showStatus(blinkEvery(500) ? COLOR_GREEN : COLOR_OFF);
      break;

    case STATE_RELAY_READY:
      showStatus(COLOR_BLUE);
      break;
#endif

    case STATE_ERROR:   // handled above
      break;
  }
}

// ─── Serial line reader (non-blocking) ─────────────────────────────────────
// Collects newline-terminated lines from Cdc and hands each to onLine.
void readSerialLines(void (*onLine)(const char *)) {
  while (Cdc.available()) {
    char c = (char)Cdc.read();
    if (c == '\n' || c == '\r') {
      if (serialLineBufLen > 0) {
        serialLineBuf[serialLineBufLen] = '\0';
        onLine(serialLineBuf);
        serialLineBufLen = 0;
      }
    } else {
      if (serialLineBufLen < 256) {
        serialLineBuf[serialLineBufLen++] = c;
      } else {
        serialLineBufLen = 0;
        if ((bool)Cdc) Cdc.println("ERR:SERIAL_OVERFLOW");
      }
    }
  }
}

// ─── User settings ─────────────────────────────────────────────────────────
const SettingDef *findSetting(const char *name) {
  for (const SettingDef &s : SETTINGS) {
    if (strcmp(s.name, name) == 0) return &s;
  }
  return nullptr;
}

const SettingChoice *findChoice(const SettingDef &s, const char *name) {
  for (uint8_t i = 0; i < s.count; i++) {
    if (strcmp(s.choices[i].name, name) == 0) return &s.choices[i];
  }
  return nullptr;
}

const char *currentChoiceName(const SettingDef &s) {
  for (uint8_t i = 0; i < s.count; i++) {
    if (s.choices[i].value == *s.value) return s.choices[i].name;
  }
  return s.choices[0].name;
}

// Reads every setting from prefs (already open). A missing or unknown stored
// value falls back to the default.
void loadSettings() {
  for (const SettingDef &s : SETTINGS) {
    uint8_t stored = prefs.getUChar(s.nvsKey, s.choices[0].value);
    *s.value = s.choices[0].value;
    for (uint8_t i = 0; i < s.count; i++) {
      if (s.choices[i].value == stored) *s.value = stored;
    }
  }
}

// Saves and applies one setting. Returns false (setting unchanged) if NVS fails.
bool saveSetting(const SettingDef &s, uint8_t value) {
  prefs.begin("remote", false);
  size_t written = prefs.putUChar(s.nvsKey, value);
  prefs.end();
  if (written == 0) return false;
  *s.value = value;
  return true;
}

// The settings, then the selected key (read-only here; BOOT changes it).
void printSettings() {
  Cdc.print("SETTINGS:");
  for (size_t i = 0; i < COUNT_OF(SETTINGS); i++) {
    Cdc.printf("%s=%s,", SETTINGS[i].name, currentChoiceName(SETTINGS[i]));
  }
  Cdc.printf("key=%c\n", hidKey);
}

// ─── Settings protocol (settings mode) ─────────────────────────────────────
// The mode flag was cleared when settings mode started, so a restart is normal mode.
void restartToNormalMode() {
  if ((bool)Cdc) {
    Cdc.println("REBOOTING...");
    Cdc.flush();
  }
  delay(100);
  ESP.restart();
}

void parseSettingsLine(const char *line) {
  if (strcmp(line, "GET") == 0) {
    printSettings();
    return;
  }

  if (strncmp(line, "SET:", 4) == 0) {
    const char *eq = strchr(line + 4, '=');
    char name[16];
    size_t nameLen = eq ? (size_t)(eq - (line + 4)) : 0;
    if (!eq || nameLen == 0 || nameLen >= sizeof(name)) {
      Cdc.println("ERR:SET_PARSE — expected SET:name=value");
      return;
    }
    memcpy(name, line + 4, nameLen);
    name[nameLen] = '\0';
    const SettingDef *s = findSetting(name);
    if (!s) {
      Cdc.printf("ERR:UNKNOWN_SETTING:%s\n", name);
      return;
    }
    const SettingChoice *c = findChoice(*s, eq + 1);
    if (!c) {
      Cdc.printf("ERR:INVALID_VALUE:%s=%s\n", name, eq + 1);
      return;
    }
    if (!saveSetting(*s, c->value)) {
      Cdc.printf("ERR:SAVE_FAILED:%s\n", name);
      return;
    }
    Cdc.printf("OK:%s=%s\n", s->name, c->name);
    return;
  }

  if (strcmp(line, "DEFAULTS") == 0) {
    bool ok = true;
    for (const SettingDef &s : SETTINGS) ok &= saveSetting(s, s.choices[0].value);
    if (!ok) Cdc.println("ERR:SAVE_FAILED:defaults");
    printSettings();
    return;
  }

  if (strcmp(line, "REBOOT") == 0) {
    restartToNormalMode();
    return;
  }

  Cdc.printf("ERR:UNKNOWN_COMMAND:%s\n", line);
}

#if WEB_SERIAL_ENABLED
// ─── Serial line parser ─────────────────────────────────────────────────────
void parseSerialLine(const char *line) {
  LOG_PRINTF("LOG:RX [%s] ts=%lu\n", line, millis());

#if WIFI_ENABLED
  if (strncmp(line, "WIFI:", 5) == 0) {
    const char *sep = strchr(line + 5, ':');
    if (!sep) {
      if ((bool)Cdc) Cdc.println("ERR:WIFI_PARSE_NO_SSID");
      return;
    }
    // Construct Strings for WiFi creds (rare command, String is acceptable)
    char ssidBuf[34];
    size_t ssidLen = sep - (line + 5);
    if (ssidLen >= sizeof(ssidBuf)) ssidLen = sizeof(ssidBuf) - 1;
    memcpy(ssidBuf, line + 5, ssidLen);
    ssidBuf[ssidLen] = '\0';
    wifiSSID = String(ssidBuf);
    wifiPass = String(sep + 1);
    prefs.begin("remote", false);
    prefs.putString("wifi_ssid", wifiSSID);
    prefs.putString("wifi_pass", wifiPass);
    prefs.end();
    if (wifiAutoStarted && (currentState == STATE_WS_READY || currentState == STATE_WS_WAITING) && WiFi.status() == WL_CONNECTED) {
      LOG_PRINTLN("LOG:WIFI_ALREADY_UP — skipping reconnect");
      if ((bool)Cdc) {
        Cdc.printf("WIFI_CONNECTED:%s\n", WiFi.localIP().toString().c_str());
        Cdc.printf("WS_STARTED:ws://%s\n", WiFi.localIP().toString().c_str());
      }
      wifiAutoStarted = false;
      return;
    }
    if (wifiAutoStarted && currentState == STATE_WIFI_CONNECTING) {
      LOG_PRINTLN("LOG:WIFI_AUTO_IN_PROGRESS — waiting for auto-connect");
      wifiAutoStarted = false;
      return;
    }
    wifiAutoStarted = false;
    startWiFi(wifiSSID, wifiPass);
    return;
  }

  if (strcmp(line, "WIFI_CLEAR") == 0) {
    prefs.begin("remote", false);
    prefs.remove("wifi_ssid");
    prefs.remove("wifi_pass");
    prefs.end();
    wifiSSID = "";
    wifiPass = "";
    Cdc.println("WIFI_CLEARED");
    return;
  }
#endif // WIFI_ENABLED

  if (strncmp(line, "KEY:", 4) == 0 && strlen(line) == 5) {
    char newKey = line[4];
    if (isAlphaNumeric(newKey)) {
      newKey = toLowerCase(newKey);
      prefs.begin("remote", false);
      prefs.putUChar("hid_key", (uint8_t)newKey);
      prefs.end();
      Cdc.printf("KEY_SAVED:%c — send REBOOT to apply\n", newKey);
    } else {
      Cdc.println("ERR:KEY_INVALID — must be a single letter or digit");
    }
    return;
  }

  if (strcmp(line, "KEY_CLEAR") == 0) {
    prefs.begin("remote", false);
    prefs.remove("hid_key");
    prefs.end();
    Cdc.println("KEY_CLEARED — send REBOOT to apply");
    return;
  }

  if (strcmp(line, "DIAG") == 0) {
    Cdc.printf("DIAG:cpu_mhz=%d\n", getCpuFrequencyMhz());
    Cdc.printf("DIAG:heap_free=%u\n", ESP.getFreeHeap());
    wifi_ps_type_t psType;
    esp_wifi_get_ps(&psType);
    Cdc.printf("DIAG:wifi_ps=%s\n",
                   psType == WIFI_PS_NONE ? "NONE" :
                   psType == WIFI_PS_MIN_MODEM ? "MIN_MODEM" :
                   psType == WIFI_PS_MAX_MODEM ? "MAX_MODEM" : "UNKNOWN");
    Cdc.printf("DIAG:uptime_s=%lu\n", millis() / 1000);
    return;
  }

  if (strcmp(line, "REBOOT") == 0) {
    Cdc.println("REBOOTING...");
    delay(100);
    ESP.restart();
    return;
  }
}

// ─── Serial handler (non-blocking line reader) ─────────────────────────────
void handleSerial() {
  bool serialPortOpen = (bool)Cdc && usbHostReady;

  if (serialPortOpen && !serialWasOpen) {
    serialWasOpen = true;
    serialLineBufLen = 0;

#if !WIFI_ENABLED
    // Primary role is set by USB enumeration — transition state and send browser handshake
    if (currentState == STATE_BOOT_OK || currentState == STATE_USB_HOST || currentState == STATE_RELAY_READY) {
      setState(STATE_SERIAL_CONNECTED);
    }
    Cdc.println("READY");
    Cdc.printf("CONFIG:key=%c\n", hidKey);
    if (usbHostSeen) Cdc.println("PRIMARY");
    LOG_PRINTF("LOG:SERIAL_OPEN ts=%lu heap=%u\n", millis(), ESP.getFreeHeap());
#else
    if (currentState == STATE_BOOT_OK) {
      setState(STATE_SERIAL_CONNECTED);
    }
    Cdc.println("READY");
    Cdc.printf("CONFIG:key=%c\n", hidKey);
    LOG_PRINTF("LOG:SERIAL_OPEN ts=%lu heap=%u\n", millis(), ESP.getFreeHeap());
    if (currentState == STATE_WIFI_CONNECTING) {
      Cdc.println("WIFI_CONNECTING");
    } else if (currentState == STATE_WS_READY || currentState == STATE_WS_WAITING) {
      Cdc.printf("WIFI_CONNECTED:%s\n", WiFi.localIP().toString().c_str());
      Cdc.printf("WS_STARTED:ws://%s\n", WiFi.localIP().toString().c_str());
    }
#endif

  } else if (!serialPortOpen && serialWasOpen) {
    serialWasOpen = false;

#if !WIFI_ENABLED
    // An open port implies a host was seen — serial close reverts to the hosted state
    if (currentState == STATE_SERIAL_CONNECTED) {
      setState(usbHostReady ? STATE_USB_HOST : STATE_HOST_AWAY);
    }
    LOG_PRINTLN("LOG:SERIAL_CLOSED");
#else
    if (currentState == STATE_SERIAL_CONNECTED) {
      setState(STATE_BOOT_OK);
    }
#endif
  }

  readSerialLines(parseSerialLine);
}

#endif // WEB_SERIAL_ENABLED

// ─── Key delivery (trigger and BOOT presses) ─────────────────────────────────
// Hosted → HID keystroke, before anything else. Never hosted → ESP-NOW to the primary.
// The RGB LED shows white while the button is held if the key went out, red if
// it had nowhere to go. hold: keep the HID key down while the trigger is (else a tap).
void sendSelectedKey(bool hold) {
  bool routed = false;

  // 1. USB HID — first, ahead of every other channel. Preempts any relayed tap.
  if (usbHostReady) {
    routed = hidPress(hidKey, hold);
  }

  seqCounter++;
  LOG_PRINTF("LOG:BTN seq=%d hid=%d ts=%lu\n", seqCounter, (int)routed, millis());
  if (usbHostSeen && !routed && (bool)Cdc) Cdc.println("ERR:HID_NOT_DELIVERED");
  char btnMsg[2] = { hidKey, '\0' };

  // 2. Serial (CDC) notification
  PROTO_PRINTLN(btnMsg);
  PROTO_PRINTF("LOG:HEAP=%u seq=%d\n", ESP.getFreeHeap(), seqCounter);

#if WIFI_ENABLED
  // 3. WebSocket
  bool wsConnected = (currentState == STATE_WS_READY &&
                      WiFi.status() == WL_CONNECTED &&
                      ws.count() > 0);
  if (wsConnected) {
    sendWsMessage(btnMsg);
    routed = true;
  } else {
    LOG_PRINTF("LOG:WS_SKIP state=%d wifi=%d clients=%u msg=%s\n",
               (int)currentState, (int)WiFi.status(), (unsigned)ws.count(), btnMsg);
  }
#else
  // 3. ESP-NOW: only units never hosted this uptime send to the primary. A hosted
  //    unit whose host is away waits for it instead.
  if (!usbHostSeen && hasPrimary) {
    routed = sendKeystroke();
  }
#endif

  pressDelivered = routed;
}

// Send now, or keep the press until the boot USB wait knows the delivery path.
void keyPressed(bool hold) {
  if (usbWaitActive) {
    pendingPress = true;
    pendingHold  = hold;
  } else {
    sendSelectedKey(hold);
  }
}

// ─── Key-select mode ─────────────────────────────────────────────────────────
// Cycle order: a–z then 0–9.
char nextKeyChar(char c) {
  if (c >= 'a' && c < 'z') return c + 1;
  if (c == 'z')            return '0';
  if (c >= '0' && c < '9') return c + 1;
  return 'a';
}

// Persist the key. Called on every BOOT press and at every boot.
void saveHidKey() {
  prefs.begin("remote", false);
  size_t written = prefs.putUChar("hid_key", (uint8_t)hidKey);
  prefs.end();
  if (written == 0) {
    if ((bool)Cdc) Cdc.println("ERR:KEY_SAVE_FAILED");
    return;
  }
  LOG_PRINTF("LOG:KEYSEL_SAVED key=%c ts=%lu\n", hidKey, millis());
}

// ─── External status LED (non-blocking) ────────────────────────────────────
void writeStatusLed(uint32_t duty) {
  if (duty == ledLastDuty) return;
  ledLastDuty = duty;
  ledcWrite(STATUS_LED_PIN, STATUS_LED_INVERT ? STATUS_LED_MAX_DUTY - duty : duty);
}

void initStatusLed() {
  statusLedReady = ledcAttach(STATUS_LED_PIN, STATUS_LED_PWM_HZ, STATUS_LED_PWM_BITS);
  if (!statusLedReady) {
    if ((bool)Cdc) Cdc.println("ERR:STATUS_LED_PWM_INIT");
    return;
  }
  ledPhaseStartMs = millis();
  writeStatusLed(0);
}

// Duty for a pattern t ms into its cycle. Blinks start lit, breathing starts dark.
uint32_t ledPatternDuty(uint8_t pattern, unsigned long t) {
  switch (pattern) {
    case LED_FAST: return (t % LED_FAST_PERIOD_MS) < LED_FAST_PERIOD_MS / 2 ? STATUS_LED_MAX_DUTY : 0;
    case LED_SLOW: return (t % LED_SLOW_PERIOD_MS) < LED_SLOW_PERIOD_MS / 2 ? STATUS_LED_MAX_DUTY : 0;
    case LED_ON:   return (t % LED_FLICKER_PERIOD_MS) < LED_FLICKER_PERIOD_MS / 2 ? STATUS_LED_MAX_DUTY : 0;
    case LED_BREATHE: {
      // Raised-cosine over the whole period (no dark pause), squared for
      // perceptually even brightness.
      float x      = (float)(t % LED_BREATHE_PERIOD_MS) / LED_BREATHE_PERIOD_MS;
      float breath = 0.5f - 0.5f * cosf(2.0f * PI * x);
      return (uint32_t)(breath * breath * STATUS_LED_MAX_DUTY);
    }
    default:       return 0;
  }
}

// The "pressed" pattern while the trigger or BOOT is held, the "idle" pattern
// otherwise. A pattern change restarts its cycle.
void handleStatusLed() {
  if (!statusLedReady) return;
  unsigned long now = millis();

  uint8_t pattern = (triggerBtn.pressed || keyBtn.pressed) ? ledPressed : ledIdle;
  if (pattern != ledShownPattern) {
    ledShownPattern = pattern;
    ledPhaseStartMs = now;
  }
  writeStatusLed(ledPatternDuty(pattern, now - ledPhaseStartMs));
}

// ─── Button polling ────────────────────────────────────────────────────────
// Each button is read every BUTTON_POLL_MS; contact bounce settles between polls.
ButtonEdge pollButton(Button &btn, unsigned long now) {
  if (now - btn.lastPollMs < BUTTON_POLL_MS) return BTN_NONE;
  btn.lastPollMs = now;
  bool down = digitalRead(btn.pin) == LOW;
  if (down == btn.pressed) return BTN_NONE;
  btn.pressed = down;
  return down ? BTN_PRESSED : BTN_RELEASED;
}

// ─── Trigger button ────────────────────────────────────────────────────────
// edge=press: types on press, held while the button is if repeat is on
// (handleHidOutput() releases it), else a tap. edge=release: a tap on release.
enum TriggerAction { TRIG_NONE, TRIG_HOLD, TRIG_TAP };

TriggerAction triggerAction(ButtonEdge edge) {
  if (trigEdge == EDGE_RELEASE) return edge == BTN_RELEASED ? TRIG_TAP : TRIG_NONE;
  if (edge == BTN_PRESSED)      return trigRepeat ? TRIG_HOLD : TRIG_TAP;
  return TRIG_NONE;
}

void handleTriggerButton() {
  TriggerAction action = triggerAction(pollButton(triggerBtn, millis()));
  if (action == TRIG_NONE) return;
  keyPressed(action == TRIG_HOLD);
  if (trigEdge == EDGE_RELEASE) {
    releaseFeedback   = true;
    releaseFeedbackMs = millis();
  }
}

// ─── Settings mode switch ──────────────────────────────────────────────────
// Sets the NVS flag that setup() reads (and clears) at the next boot, then restarts.
void restartIntoSettingsMode() {
  prefs.begin("remote", false);
  size_t written = prefs.putUChar("settings", 1);
  prefs.end();
  if (written == 0) {
    if ((bool)Cdc) Cdc.println("ERR:SETTINGS_FLAG_SAVE_FAILED");
    return;
  }
  LOG_PRINTF("LOG:SETTINGS_RESTART ts=%lu\n", millis());
  ESP.restart();
}

// ─── BOOT button: key selection ────────────────────────────────────────────
// Each press steps to the next key, applies it at once, types it as a tap and
// saves it. Held for SETTINGS_HOLD_MS, the key steps back and the unit restarts
// into settings mode.
void handleKeyButton() {
  unsigned long now = millis();
  ButtonEdge edge = pollButton(keyBtn, now);

  if (edge == BTN_PRESSED) {
    keyBeforeHold  = hidKey;
    keyHoldStartMs = now;
    keyHoldTracked = true;
    hidKey = nextKeyChar(hidKey);
    LOG_PRINTF("LOG:KEYSEL key=%c ts=%lu\n", hidKey, now);
    PROTO_PRINTF("CONFIG:key=%c\n", hidKey);
    keyPressed(false);
    saveHidKey();
  } else if (edge == BTN_RELEASED) {
    keyHoldTracked = false;
  }

  if (keyHoldTracked && now - keyHoldStartMs >= SETTINGS_HOLD_MS) {
    keyHoldTracked = false;
    hidKey = keyBeforeHold;
    PROTO_PRINTF("CONFIG:key=%c\n", hidKey);
    saveHidKey();
    restartIntoSettingsMode();
  }
}

// ─── ESP-NOW initialization ─────────────────────────────────────────────────
void initEspNow() {
  if (WiFi.getMode() == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
  }
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    if ((bool)Cdc) Cdc.println("ERR:ESPNOW_INIT_FAILED");
    setState(STATE_ERROR);
    return;
  }

  // Register broadcast peer (needed for ANNOUNCE broadcasts)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, ESPNOW_BROADCAST, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (!esp_now_is_peer_exist(ESPNOW_BROADCAST)) {
    esp_now_add_peer(&peerInfo);
  }

  // Register callbacks
#if WIFI_ENABLED
  esp_now_register_recv_cb(onEspNowReceive);
#else
  // Cache own MAC for self-announce filtering in receive callback
  WiFi.macAddress(ownMacAddr);
  esp_now_register_recv_cb(onEspNowReceiveRelay);
  esp_now_register_send_cb(onEspNowSendRelay);
#endif

  espNowInitialized = true;

  // No battery on this board: keep the radio fully awake so the primary never
  // misses relayed keystrokes while duty-cycling RX.
  esp_wifi_set_ps(WIFI_PS_NONE);

  LOG_PRINTF("LOG:ESPNOW_INIT ch=%d ts=%lu\n", ESPNOW_CHANNEL, millis());
}

// ─── setup() ───────────────────────────────────────────────────────────────
void setup() {
  // 1. RGB status LED
  showStatus(COLOR_OFF);

  // 2. Buttons. BOOT can only be down at boot after a software restart (at power-up
  //    it selects download mode), i.e. still held from the press that restarted us:
  //    start it pressed so that press is not taken as a new one.
  pinMode(TRIGGER_BTN_PIN, INPUT_PULLUP);
  pinMode(KEY_BTN_PIN, INPUT_PULLUP);
  keyBtn.pressed = digitalRead(KEY_BTN_PIN) == LOW;

  // 3. Stored key, settings and mode flag. The flag is cleared at once, so any
  //    later restart, RESET or power cycle comes back in normal mode.
  prefs.begin("remote", false);
  hidKey = (char)prefs.getUChar("hid_key", DEFAULT_KEY);
  loadSettings();
  settingsMode = prefs.getUChar("settings", 0) == 1;
  if (settingsMode) prefs.remove("settings");
#if WIFI_ENABLED
  String wifiSSID_stored = prefs.getString("wifi_ssid", "");
  String wifiPass_stored = prefs.getString("wifi_pass", "");
#endif
  prefs.end();
  if (!isAlphaNumeric(hidKey)) hidKey = DEFAULT_KEY;
  hidKey = toLowerCase(hidKey);   // keys saved upper case by earlier firmware
  saveHidKey();

  // 4. External status LED (uses the settings)
  initStatusLed();

  // 5. USB device setup — ORDER IS CRITICAL: every interface must be registered
  //    before USB.begin(). Settings mode leaves the keyboard out.
  if (!settingsMode) {
    keyboard = new USBHIDKeyboard();
    hid      = new USBHID();
    keyboard->begin();
  }
  Cdc.begin(115200);
  USB.manufacturerName("Andrey");
  char productName[32];
  uint8_t mac[6];
  if (!settingsMode) {
    snprintf(productName, sizeof(productName), "Zero Remote [%c]", hidKey);
  } else if (esp_efuse_mac_get_default(mac) == ESP_OK) {
    // Key and the end of the MAC (the USB serial number) tell boards apart in the
    // browser's port chooser
    snprintf(productName, sizeof(productName), "USB Button [%c] (%02X:%02X:%02X)",
             hidKey, mac[3], mac[4], mac[5]);
  } else {
    snprintf(productName, sizeof(productName), "USB Button [%c]", hidKey);
  }
  USB.productName(productName);
  USB.begin();

  // A loop() stuck for 5 s (the core's task watchdog timeout) resets the unit
  // instead of leaving the button dead. Nothing in loop() blocks for more than a
  // few hundred ms (HID report 100 ms, CDC write 250 ms, NVS write).
  enableLoopWDT();

  // Settings mode: no keyboard, radio or host tracking — see loopSettingsMode()
  if (settingsMode) {
    settingsStartMs = millis();
    return;
  }

  // 6. Enumeration is tracked in loop() (handleHostEnumeration) — no blocking
  //    wait, so the trigger is live from the first loop pass.
  usbWaitStartMs = millis();

  // 7. Boot-OK state
  setState(STATE_BOOT_OK);

  // 8. Create FreeRTOS queues + Initialize ESP-NOW
#if !WIFI_ENABLED
  relayRxQueue    = xQueueCreate(8, sizeof(RelayRxItem));
  announceRxQueue = xQueueCreate(4, sizeof(AnnounceRxItem));
  sendStatusQueue = xQueueCreate(1, sizeof(esp_now_send_status_t));
#endif
  WiFi.mode(WIFI_STA);
  initEspNow();

#if WIFI_ENABLED
  // 9. Auto-connect to WiFi if credentials saved
  if (wifiSSID_stored.length() > 0) {
    wifiSSID = wifiSSID_stored;
    wifiPass = wifiPass_stored;
    wifiAutoStarted = true;
    WiFi.setAutoReconnect(true);
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    wifiStartMs = millis();
    setState(STATE_WIFI_CONNECTING);
  }
#endif
}

// ─── USB host tracking ─────────────────────────────────────────────────────
// TinyUSB state is polled directly. tud_mounted(): a host configured us.
// tud_suspended(): host asleep, or upstream gone — a computer unplugged from a
// self-powered hub looks like a suspend, not an unmount.
// The first mount latches usbHostSeen for the rest of the uptime: from then on
// the unit's own presses go to HID only, and it waits for an absent host to return.
void handleHostEnumeration() {
  bool mounted = tud_mounted();
  bool ready   = mounted && !tud_suspended();

  if (mounted && !usbHostSeen) {
    usbHostSeen = true;
#if !WIFI_ENABLED
    // Leave the remote role for good: hosted units never send over ESP-NOW.
    hasPrimary   = false;
    pendingRetry = false;
    if (primaryPeerRegistered) {
      esp_now_del_peer(primaryMac);
      primaryPeerRegistered = false;
    }
    PROTO_PRINTLN("READY");
    PROTO_PRINTF("CONFIG:key=%c\n", hidKey);
    PROTO_PRINTLN("PRIMARY");
#endif
    LOG_PRINTF("LOG:HOST_SEEN ts=%lu\n", millis());
  }

  if (ready != usbHostReady) {
    usbHostReady = ready;
    LOG_PRINTF("LOG:HOST_%s ts=%lu\n", ready ? "READY" : "AWAY", millis());
    if (!ready) relayTapCount = 0;   // don't replay relayed keys when the host returns
#if !WIFI_ENABLED
    if (ready) {
      if (currentState == STATE_BOOT_OK || currentState == STATE_RELAY_READY ||
          currentState == STATE_HOST_AWAY) {
        setState(STATE_USB_HOST);
      }
      announceBurstRemaining = 3;    // let remotes find this hub straight away
      announceBurstNextMs = millis();
    } else if (currentState == STATE_USB_HOST || currentState == STATE_SERIAL_CONNECTED) {
      setState(STATE_HOST_AWAY);
    }
#endif
  }

  if (usbWaitActive && (mounted || millis() - usbWaitStartMs >= USB_ENUM_WAIT_MS)) {
    usbWaitActive = false;
    if (pendingPress) {
      pendingPress = false;
      sendSelectedKey(pendingHold && triggerBtn.pressed);   // a hold only while still held
    }
  }
}

// ─── Settings mode trigger test ────────────────────────────────────────────
// The trigger doesn't type in settings mode. The settings page shows what it
// would type instead: the button's edges, and what the current settings make
// of them (a held key, which the computer repeats until TRIGGER:up, or a tap).
void reportTriggerTest(ButtonEdge edge) {
  if (edge == BTN_NONE || !(bool)Cdc) return;
  Cdc.println(edge == BTN_PRESSED ? "TRIGGER:down" : "TRIGGER:up");
  TriggerAction action = triggerAction(edge);
  if (action != TRIG_NONE) Cdc.println(action == TRIG_HOLD ? "TYPE:hold" : "TYPE:tap");
}

// ─── Settings mode loop ────────────────────────────────────────────────────
// The trigger only previews the status LED and reports to the settings page; a new BOOT press restarts into
// normal mode (the BOOT press that entered settings mode is still marked pressed).
void loopSettingsMode() {
  unsigned long now = millis();
  reportTriggerTest(pollButton(triggerBtn, now));
  if (pollButton(keyBtn, now) == BTN_PRESSED) restartToNormalMode();
  readSerialLines(parseSettingsLine);
  handleStatusRgb();
  handleStatusLed();
}

// ─── loop() ────────────────────────────────────────────────────────────────
// Host state is refreshed first and the trigger is read before any relay or
// display work, so a local press reaches HID ahead of everything else.
void loop() {
  if (settingsMode) {
    loopSettingsMode();
    return;
  }
  handleHostEnumeration();
  handleTriggerButton();
  handleHidOutput();
#if WIFI_ENABLED
  handleEspNow();
#else
  handleRelay();
#endif
  handleKeyButton();
  handleStatusRgb();
  handleStatusLed();
#if WEB_SERIAL_ENABLED
  handleSerial();
#endif
#if WIFI_ENABLED
  handleWiFi();
  if (currentState == STATE_WS_READY || currentState == STATE_WS_WAITING) {
    ws.cleanupClients(1);
  }
#endif
}
