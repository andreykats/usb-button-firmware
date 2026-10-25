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
 *   - WEB_SERIAL_ENABLED=1 — Web Serial protocol over USB CDC (browser app in
 *     website/). Requires Tools → USB CDC On Boot: Enabled.
 *   - WIFI_ENABLED=1       — WebSocket over WiFi + encrypted ESP-NOW credential
 *     sharing. Replaces the ESP-NOW keystroke relay (the two are exclusive).
 *
 * BUTTONS (both polled every BUTTON_POLL_MS; while a press is held the RGB LED
 *          shows white if the key was delivered, red if not):
 *   - Trigger (external, TRIGGER_BTN_PIN → GND) — types the selected key. Over
 *     USB HID the key goes down on press and up on release, so holding it makes
 *     the computer repeat the key. Over ESP-NOW the primary types a single tap.
 *   - BOOT (GPIO0) — steps to the next key (a–z, then 0–9), applies it at once,
 *     saves it to NVS and types it like the trigger. The key is also written at
 *     every boot. The USB product name ("Zero Remote [x]") updates at the next
 *     power-up.
 *   Keys are sent without Shift: letters type lower case. Default key: '1'.
 *
 * STATUS LED (external, STATUS_LED_PIN → transistor switching 5V to an LED):
 *   - Idle: continuous breathing, one fade-in/fade-out per LED_IDLE_PERIOD_MS.
 *   - Trigger or BOOT held: fast blinking for as long as it is held, then back to idle.
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
 *   USB CDC On Boot : Enabled                  ← keeps a CDC port for uploads/logs;
 *                                                "Disabled" = HID-only device, flash
 *                                                via BOOT+RESET download mode
 *   Upload Mode     : USB-OTG CDC (TinyUSB)
 *
 * HARDWARE PINS (from ESP32-S3-Zero schematic, resources/):
 *   GPIO 0   — BOOT button (active LOW, 10K external pull-up) — key select
 *   GPIO 1   — status LED transistor drive (PWM, HIGH = LED on)    — STATUS_LED_PIN
 *   GPIO 13  — trigger button (external, to GND, internal pull-up) — TRIGGER_BTN_PIN
 *   GPIO 21  — onboard WS2812B RGB LED data (RGB colour order)     — STATUS_RGB_PIN
 *   There is no VBUS sense pin: host vs. power-only is detected by USB
 *   enumeration.
 *
 * SERIAL PROTOCOL (WEB_SERIAL_ENABLED=1, 115200 baud, newline-terminated):
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
 * RGB STATUS LED (full brightness):
 *   Blinking yellow (150ms) — booting: waiting to learn whether a computer is attached
 *   Solid green             — computer connected: HID + primary hub (also Web Serial)
 *   Blinking green  (500ms) — computer was connected but is asleep/unplugged: waiting
 *   Solid blue              — no computer, connected to the mesh via a primary
 *   Blinking blue   (500ms) — no computer, looking for a primary
 *   Blinking red    (250ms) — error (ESP-NOW failed to start); held until reboot
 *   White while held        — button press delivered (trigger or BOOT)
 *   Red while held          — button press could not be delivered
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

// 1 = emit verbose LOG:* lines over CDC Serial. 0 = suppress verbose logs.
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
#include <Preferences.h>         // NVS flash — persists key across reboots/reflash

#if WIFI_ENABLED
#include <mbedtls/aes.h>          // AES-128-CBC encryption (hardware-accelerated)
#include <ESPAsyncWebServer.h>    // ESP32Async version — only for WiFi/WS mode
#endif

// ─── Board-setting guards ────────────────────────────────────────────────────
#if ARDUINO_USB_MODE
#error "Set Tools → USB Mode: USB-OTG (TinyUSB). USB HID does not work in Hardware CDC and JTAG mode."
#endif
#if WEB_SERIAL_ENABLED && !ARDUINO_USB_CDC_ON_BOOT
#error "WEB_SERIAL_ENABLED=1 requires Tools → USB CDC On Boot: Enabled."
#endif

// Hold duration of a relayed key tap before release (handled asynchronously in loop()).
const unsigned long HID_HOLD_MS = 10;

#define LOG_PRINTF(...) do { if (ENABLE_VERBOSE_LOGS && (bool)Serial) Serial.printf(__VA_ARGS__); } while (0)
#define LOG_PRINTLN(x)  do { if (ENABLE_VERBOSE_LOGS && (bool)Serial) Serial.println(x); } while (0)

// Web Serial protocol output — compiled out unless WEB_SERIAL_ENABLED=1
#define PROTO_PRINTF(...) do { if (WEB_SERIAL_ENABLED && (bool)Serial) Serial.printf(__VA_ARGS__); } while (0)
#define PROTO_PRINTLN(x)  do { if (WEB_SERIAL_ENABLED && (bool)Serial) Serial.println(x); } while (0)

// ─── USB Objects ───────────────────────────────────────────────────────────
// Keyboard registers the HID keyboard interface; reports are sent through HID
// directly because Keyboard.press() discards the delivery result. HID must be
// declared after Keyboard so the keyboard protocol is the one registered.
USBHIDKeyboard Keyboard;
USBHID         HID;

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
const unsigned long LED_IDLE_PERIOD_MS   = 3000;  // one breath (fade up + down) every 3 s while idle
const unsigned long LED_FAST_PERIOD_MS   = 100;   // while the trigger is held: 50 ms on / 50 ms off (10 Hz)

// ─── Onboard RGB status LED (WS2812B) ──────────────────────────────────────
// Driven by the core's rgbLedWriteOrdered() (RMT). The S3-Zero's LED takes RGB
// order, not the usual WS2812B GRB (matches the core's waveshare_esp32_s3_zero
// variant). If red and green look swapped on hardware, change STATUS_RGB_ORDER.
#define STATUS_RGB_PIN         21
#define STATUS_RGB_ORDER       LED_COLOR_ORDER_RGB

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

// HID output: the host sees one key at a time. A local press holds its key until
// both buttons are up and cuts a relayed tap short; relayed taps queue behind it.
char          hidLocalKey    = 0;       // key held for a local button; 0 = none
char          hidSentKey     = 0;       // key in the last report sent; 0 = all up. A release is owed while set
char          relayKey       = 0;       // relayed tap in progress; 0 = none
unsigned long relayPressMs   = 0;
const int     RELAY_TAP_QUEUE_LEN = 8;
char          relayTapQueue[RELAY_TAP_QUEUE_LEN];
int           relayTapHead   = 0;
int           relayTapCount  = 0;

// Status LED
bool          statusLedReady   = false;
bool          ledFastActive    = false;
unsigned long ledPhaseStartMs  = 0;       // start of the current fast or idle cycle
uint32_t      ledLastDuty      = 0xFFFFFFFFUL;

// RGB status LED: blink phase, press feedback, write cache
unsigned long lastBlinkMs    = 0;
bool          blinkPhase     = false;
bool          pressDelivered = false;   // white (delivered) vs red (not delivered) while a button is held
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

// Set the onboard RGB LED to colour c. Only writes when the colour changes.
void showStatus(RGB c) {
  uint32_t packed = ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
  if (packed == rgbShown) return;
  rgbShown = packed;
  rgbLedWriteOrdered(STATUS_RGB_PIN, STATUS_RGB_ORDER, c.r, c.g, c.b);
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
  if (!HID.SendReport(HID_REPORT_ID_KEYBOARD, &report, sizeof(report))) return false;
  hidSentKey = key;
  return true;
}

// Press a local key now; handleHidOutput() releases it once both buttons are up.
// A key still down is released first so the host always sees a fresh key-down,
// even when the same key is pressed twice.
bool hidPress(char key) {
  if (!usbHostReady) return false;
  relayKey    = 0;
  hidLocalKey = key;
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

// Brings the host's key state to what it should be: the local key while a button
// is held, otherwise the current relayed tap, otherwise all keys up. Retried every
// loop until it lands: a lost release would leave the key auto-repeating on the host.
void handleHidOutput() {
  if (hidLocalKey && !triggerBtn.pressed && !keyBtn.pressed) hidLocalKey = 0;

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
    if ((bool)Serial) Serial.println("ERR:ESPNOW_ENCRYPT_FAILED");
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
      if ((bool)Serial) {
        Serial.printf("ERR:WS id=%u ts=%lu\n", client->id(), millis());
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
    if ((bool)Serial) Serial.printf("ERR:LOW_HEAP heap=%u\n", freeHeap);
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

// Shows the state colour, with press feedback on top while a button is held.
void handleStatusRgb() {
  unsigned long now = millis();

  // Button press feedback: white = delivered, red = nowhere to go. During the
  // boot window the press is still pending, so booting stays visible.
  if ((triggerBtn.pressed || keyBtn.pressed) && !usbWaitActive) {
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

#if WEB_SERIAL_ENABLED
// ─── Serial line parser ─────────────────────────────────────────────────────
void parseSerialLine(const char *line) {
  LOG_PRINTF("LOG:RX [%s] ts=%lu\n", line, millis());

#if WIFI_ENABLED
  if (strncmp(line, "WIFI:", 5) == 0) {
    const char *sep = strchr(line + 5, ':');
    if (!sep) {
      if ((bool)Serial) Serial.println("ERR:WIFI_PARSE_NO_SSID");
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
      if ((bool)Serial) {
        Serial.printf("WIFI_CONNECTED:%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("WS_STARTED:ws://%s\n", WiFi.localIP().toString().c_str());
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
    Serial.println("WIFI_CLEARED");
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
      Serial.printf("KEY_SAVED:%c — send REBOOT to apply\n", newKey);
    } else {
      Serial.println("ERR:KEY_INVALID — must be a single letter or digit");
    }
    return;
  }

  if (strcmp(line, "KEY_CLEAR") == 0) {
    prefs.begin("remote", false);
    prefs.remove("hid_key");
    prefs.end();
    Serial.println("KEY_CLEARED — send REBOOT to apply");
    return;
  }

  if (strcmp(line, "DIAG") == 0) {
    Serial.printf("DIAG:cpu_mhz=%d\n", getCpuFrequencyMhz());
    Serial.printf("DIAG:heap_free=%u\n", ESP.getFreeHeap());
    wifi_ps_type_t psType;
    esp_wifi_get_ps(&psType);
    Serial.printf("DIAG:wifi_ps=%s\n",
                   psType == WIFI_PS_NONE ? "NONE" :
                   psType == WIFI_PS_MIN_MODEM ? "MIN_MODEM" :
                   psType == WIFI_PS_MAX_MODEM ? "MAX_MODEM" : "UNKNOWN");
    Serial.printf("DIAG:uptime_s=%lu\n", millis() / 1000);
    return;
  }

  if (strcmp(line, "REBOOT") == 0) {
    Serial.println("REBOOTING...");
    delay(100);
    ESP.restart();
    return;
  }
}

// ─── Serial handler (non-blocking line reader) ─────────────────────────────
void handleSerial() {
  bool serialPortOpen = (bool)Serial && usbHostReady;

  if (serialPortOpen && !serialWasOpen) {
    serialWasOpen = true;
    serialLineBufLen = 0;

#if !WIFI_ENABLED
    // Primary role is set by USB enumeration — transition state and send browser handshake
    if (currentState == STATE_BOOT_OK || currentState == STATE_USB_HOST || currentState == STATE_RELAY_READY) {
      setState(STATE_SERIAL_CONNECTED);
    }
    Serial.println("READY");
    Serial.printf("CONFIG:key=%c\n", hidKey);
    if (usbHostSeen) Serial.println("PRIMARY");
    LOG_PRINTF("LOG:SERIAL_OPEN ts=%lu heap=%u\n", millis(), ESP.getFreeHeap());
#else
    if (currentState == STATE_BOOT_OK) {
      setState(STATE_SERIAL_CONNECTED);
    }
    Serial.println("READY");
    Serial.printf("CONFIG:key=%c\n", hidKey);
    LOG_PRINTF("LOG:SERIAL_OPEN ts=%lu heap=%u\n", millis(), ESP.getFreeHeap());
    if (currentState == STATE_WIFI_CONNECTING) {
      Serial.println("WIFI_CONNECTING");
    } else if (currentState == STATE_WS_READY || currentState == STATE_WS_WAITING) {
      Serial.printf("WIFI_CONNECTED:%s\n", WiFi.localIP().toString().c_str());
      Serial.printf("WS_STARTED:ws://%s\n", WiFi.localIP().toString().c_str());
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

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLineBufLen > 0) {
        serialLineBuf[serialLineBufLen] = '\0';
        parseSerialLine(serialLineBuf);
        serialLineBufLen = 0;
      }
    } else {
      if (serialLineBufLen < 256) {
        serialLineBuf[serialLineBufLen++] = c;
      } else {
        serialLineBufLen = 0;
        if ((bool)Serial) Serial.println("ERR:SERIAL_OVERFLOW");
      }
    }
  }
}

#endif // WEB_SERIAL_ENABLED

// ─── Key delivery (trigger and BOOT presses) ─────────────────────────────────
// Hosted → HID keystroke, before anything else. Never hosted → ESP-NOW to the primary.
// The RGB LED shows white while the button is held if the key went out, red if
// it had nowhere to go.
void sendSelectedKey() {
  bool routed = false;

  // 1. USB HID — first, ahead of every other channel. Preempts any relayed tap.
  if (usbHostReady) {
    routed = hidPress(hidKey);
  }

  seqCounter++;
  LOG_PRINTF("LOG:BTN seq=%d hid=%d ts=%lu\n", seqCounter, (int)routed, millis());
  if (usbHostSeen && !routed && (bool)Serial) Serial.println("ERR:HID_NOT_DELIVERED");
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

// Send now, or hold the press until the boot USB wait knows the delivery path.
void keyPressed() {
  if (usbWaitActive) pendingPress = true;
  else               sendSelectedKey();
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
    if ((bool)Serial) Serial.println("ERR:KEY_SAVE_FAILED");
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
    if ((bool)Serial) Serial.println("ERR:STATUS_LED_PWM_INIT");
    return;
  }
  ledPhaseStartMs = millis();
  writeStatusLed(0);
}

// Fast blink while the trigger or BOOT is held, idle breathing otherwise. Each
// switch restarts the cycle, so the blink starts lit and the breathing starts dark.
void handleStatusLed() {
  if (!statusLedReady) return;
  unsigned long now = millis();

  bool fast = triggerBtn.pressed || keyBtn.pressed;
  if (fast != ledFastActive) {
    ledFastActive   = fast;
    ledPhaseStartMs = now;
  }
  unsigned long t = now - ledPhaseStartMs;

  if (ledFastActive) {
    writeStatusLed((t % LED_FAST_PERIOD_MS) < LED_FAST_PERIOD_MS / 2 ? STATUS_LED_MAX_DUTY : 0);
    return;
  }

  // Idle: raised-cosine breathing over the whole LED_IDLE_PERIOD_MS (no dark
  // pause), squared for perceptually even brightness. Starts dark.
  float x      = (float)(t % LED_IDLE_PERIOD_MS) / LED_IDLE_PERIOD_MS;
  float breath = 0.5f - 0.5f * cosf(2.0f * PI * x);
  writeStatusLed((uint32_t)(breath * breath * STATUS_LED_MAX_DUTY));
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
// Key goes down on press; handleHidOutput() releases it once the button is up.
void handleTriggerButton() {
  if (pollButton(triggerBtn, millis()) == BTN_PRESSED) keyPressed();
}

// ─── BOOT button: key selection ────────────────────────────────────────────
// Each press steps to the next key, applies it at once, types it like a
// trigger press and saves it.
void handleKeyButton() {
  if (pollButton(keyBtn, millis()) != BTN_PRESSED) return;
  hidKey = nextKeyChar(hidKey);
  LOG_PRINTF("LOG:KEYSEL key=%c ts=%lu\n", hidKey, millis());
  PROTO_PRINTF("CONFIG:key=%c\n", hidKey);
  keyPressed();
  saveHidKey();
}

// ─── ESP-NOW initialization ─────────────────────────────────────────────────
void initEspNow() {
  if (WiFi.getMode() == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
  }
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    if ((bool)Serial) Serial.println("ERR:ESPNOW_INIT_FAILED");
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

  // 2. Buttons + external status LED
  pinMode(TRIGGER_BTN_PIN, INPUT_PULLUP);
  pinMode(KEY_BTN_PIN, INPUT_PULLUP);
  initStatusLed();

  // 3. USB composite device setup — ORDER IS CRITICAL
  prefs.begin("remote", true);
  hidKey = (char)prefs.getUChar("hid_key", DEFAULT_KEY);
#if WIFI_ENABLED
  String wifiSSID_stored = prefs.getString("wifi_ssid", "");
  String wifiPass_stored = prefs.getString("wifi_pass", "");
#endif
  prefs.end();
  if (!isAlphaNumeric(hidKey)) hidKey = DEFAULT_KEY;
  hidKey = toLowerCase(hidKey);   // keys saved upper case by earlier firmware
  saveHidKey();

  Keyboard.begin();
  Serial.begin(115200);
  USB.manufacturerName("Andrey");
  char productName[32];
  snprintf(productName, sizeof(productName), "Zero Remote [%c]", hidKey);
  USB.productName(productName);
  USB.begin();

  // 4. Enumeration is tracked in loop() (handleHostEnumeration) — no blocking
  //    wait, so the trigger is live from the first loop pass.
  usbWaitStartMs = millis();

  // 5. Boot-OK state
  setState(STATE_BOOT_OK);

  // 6. Create FreeRTOS queues + Initialize ESP-NOW
#if !WIFI_ENABLED
  relayRxQueue    = xQueueCreate(8, sizeof(RelayRxItem));
  announceRxQueue = xQueueCreate(4, sizeof(AnnounceRxItem));
  sendStatusQueue = xQueueCreate(1, sizeof(esp_now_send_status_t));
#endif
  WiFi.mode(WIFI_STA);
  initEspNow();

#if WIFI_ENABLED
  // 7. Auto-connect to WiFi if credentials saved
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
      sendSelectedKey();
    }
  }
}

// ─── loop() ────────────────────────────────────────────────────────────────
// Host state is refreshed first and the trigger is read before any relay or
// display work, so a local press reaches HID ahead of everything else.
void loop() {
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
