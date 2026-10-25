/*
 * ESP32-S3 Matrix Button
 * =========================================================
 * Port of the ESP32-S3 COM POC (QT Py firmware) to the Waveshare ESP32-S3-Matrix.
 * Pressing the BOOT button fires a configurable keystroke; the selected key is
 * shown on the 8x8 RGB matrix, coloured by device state.
 *
 * DELIVERY PATHS (chosen automatically by USB enumeration):
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
 * BUTTONS:
 *   - Trigger (external, TRIGGER_BTN_PIN → GND) — sends the keystroke.
 *   - BOOT (GPIO0)                              — key selection: each press steps
 *     to the next key (A–Z, 0–9), shown blinking orange. 3 s after the last press
 *     the key is applied and saved to NVS. The USB product name
 *     ("Matrix Remote [X]") updates at the next power-up.
 *
 * STATUS LED (external, STATUS_LED_PIN → transistor switching 5V to an LED):
 *   - Idle: one soft fade-in/fade-out pulse every LED_IDLE_PERIOD_MS.
 *   - Trigger pressed: fast blinking for LED_FAST_DURATION_MS, then back to idle.
 *
 * REQUIRED LIBRARIES (install via Arduino Library Manager):
 *   - Adafruit NeoMatrix (pulls in Adafruit GFX Library + Adafruit NeoPixel)
 *   - ESPAsyncWebServer by ESP32Async (only needed when WIFI_ENABLED=1)
 *   - AsyncTCP by ESP32Async         (only needed when WIFI_ENABLED=1)
 *
 * Already built-in (no install needed):
 *   - USB (USBHIDKeyboard, USBCDC)  — part of the esp32 board package
 *   - WiFi                          — part of the esp32 board package
 *   - esp_now                       — part of the esp32 board package
 *
 * BOARD SETTINGS (Tools menu in Arduino IDE, esp32 core 3.3.x):
 *   Board           : ESP32S3 Dev Module
 *   Flash Size      : 4MB (32Mb)          ← ESP32-S3FH4R2
 *   PSRAM           : QSPI PSRAM          ← 2MB quad PSRAM (not OPI)
 *   USB Mode        : USB-OTG (TinyUSB)   ← CRITICAL — HID needs TinyUSB
 *   USB CDC On Boot : Enabled             ← keeps a CDC port for uploads/logs;
 *                                           "Disabled" = HID-only device, flash
 *                                           via BOOT+RESET download mode
 *   Upload Mode     : USB-OTG CDC (TinyUSB)
 *
 * HARDWARE PINS (from ESP32-S3-Matrix schematic, resources/):
 *   GPIO 0   — BOOT button (Key2, active LOW, 10K external pull-up) — key select
 *   GPIO 1   — trigger button (external, to GND, internal pull-up) — TRIGGER_BTN_PIN
 *   GPIO 2   — status LED transistor drive (PWM, HIGH = LED on)    — STATUS_LED_PIN
 *   GPIO 14  — WS2812B 8x8 matrix data (via MOSFET level shifter, 5V LEDs)
 *   GPIO 11/12 — QMI8658 IMU SDA/SCL, GPIO 10/13 — IMU INT1/INT2 (unused)
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
 *     KEY_CLEAR\n           Reset HID key to default 'Q'
 *     REBOOT\n              Reboot device
 *     WIFI:ssid:password\n  Send credentials (WIFI_ENABLED=1 only)
 *     WIFI_CLEAR\n          Erase saved WiFi credentials (WIFI_ENABLED=1 only)
 *
 * MATRIX STATUS (the selected key is drawn in the state colour):
 *   Fast yellow blink       — USB enumeration in progress at boot
 *   Blinking green  (500ms) — power only, no primary found yet
 *   Solid green             — USB host present: HID + primary hub
 *   Solid yellow            — hosted, host asleep/unplugged: waiting for it
 *   Solid purple            — power only, relaying via primary
 *   Solid blue              — Web Serial connected (WEB_SERIAL_ENABLED=1)
 *   White (held)            — trigger pressed, keystroke delivered
 *   Red (held)              — trigger pressed, no delivery path
 *   Blinking orange         — choosing a key (BOOT presses)
 *   Blinking blue   (500ms) — connecting to WiFi (WIFI_ENABLED=1)
 *   Alternating purple/blue — WiFi up, waiting for WS client (WIFI_ENABLED=1)
 *   Solid blue              — WiFi + WebSocket connected (WIFI_ENABLED=1)
 *   Blinking red 10x(250ms) — WiFi failed (WIFI_ENABLED=1)
 *   Blinking red    (250ms) — unrecoverable error
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
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
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

// HID press hold duration before release (handled asynchronously in loop()).
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
// Trigger: external momentary switch between this pin and GND. GPIO1 is a header
// pin with no strapping/USB/IMU/matrix role; avoid strapping pins 0, 3, 45, 46.
#define TRIGGER_BTN_PIN  1    // Active LOW, internal pull-up
#define KEY_BTN_PIN      0    // BOOT (Key2), active LOW, 10K external pull-up
#define DEBOUNCE_MS      50
const unsigned long KEY_SELECT_COMMIT_MS = 3000;  // apply + save this long after the last BOOT press

// ─── External status LED (transistor-switched 5V) ──────────────────────────
// GPIO drives the transistor base/gate; HIGH = LED on (low-side NPN / N-MOSFET).
// Set STATUS_LED_INVERT 1 for a high-side PNP / P-MOSFET stage (LOW = LED on).
#define STATUS_LED_PIN     2
#define STATUS_LED_INVERT  0
const uint32_t      STATUS_LED_PWM_HZ    = 5000;
const uint8_t       STATUS_LED_PWM_BITS  = 10;
const uint32_t      STATUS_LED_MAX_DUTY  = (1UL << STATUS_LED_PWM_BITS) - 1;
const unsigned long LED_IDLE_PERIOD_MS   = 3000;  // one pulse every 3 s while idle
const unsigned long LED_IDLE_PULSE_MS    = 1200;  // fade up + down duration of that pulse
const unsigned long LED_FAST_DURATION_MS = 3000;  // fast pulsing after a trigger press
const unsigned long LED_FAST_PERIOD_MS   = 200;   // 100 ms on / 100 ms off

// ─── 8x8 RGB matrix ────────────────────────────────────────────────────────
// Layout and colour order match Waveshare's Font demo (resources/ESP32-S3-Matrix-Demo).
// Keep brightness low — Waveshare warns high brightness overheats the board.
#define MATRIX_PIN         14
#define MATRIX_BRIGHTNESS  40
Adafruit_NeoMatrix matrix(8, 8, MATRIX_PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);
// Classic GFX font is 5x7 (+1 col spacing): centre it in the 8x8 grid.
const int16_t GLYPH_X = 1;
const int16_t GLYPH_Y = 0;

// ─── WebSocket server (only for WiFi mode) ─────────────────────────────────
#if WIFI_ENABLED
AsyncWebServer wsHttpServer(80);
AsyncWebSocket ws("/");
#endif

// ─── State machine ─────────────────────────────────────────────────────────
enum DeviceState {
  STATE_BOOT_OK,           // Blink green — no USB host (charger/power only, or no USB)
  STATE_SERIAL_CONNECTED,  // Solid green — browser connected via Web Serial (= primary, serial open)
#if WIFI_ENABLED
  STATE_WIFI_CONNECTING,   // Blink blue   — waiting for WiFi association
  STATE_WS_WAITING,        // Alternate purple/blue — WiFi up, waiting for WS client
  STATE_WS_READY,          // Solid blue   — WiFi + WebSocket client connected
  STATE_WIFI_FAILED,       // Blink red 10x — WiFi failed, then revert to green
#else
  STATE_USB_HOST,          // Solid green — USB host enumerated, primary role active, no serial yet
  STATE_HOST_AWAY,         // Solid yellow — hosted, host asleep/unplugged, waiting for it
  STATE_RELAY_READY,       // Solid purple — remote device, knows primary, ready to relay
#endif
  STATE_ERROR              // Blink red   — unrecoverable error
};
DeviceState currentState = STATE_BOOT_OK;

// ─── Configurable HID key (persisted in NVS flash) ─────────────────────────
char           hidKey        = 'Q';
Preferences    prefs;

// ─── Globals ───────────────────────────────────────────────────────────────
// Button
unsigned long lastBtnTime    = 0;
bool          btnWasPressed  = false;
int           seqCounter     = 0;
bool          lastPressRouted = false;  // last press reached HID or the ESP-NOW primary
bool          pendingTriggerPress = false;  // pressed during the boot USB wait; sent once the path is known

// HID output: one key tap at a time. The local trigger preempts; relayed taps queue.
bool          hidKeyDown     = false;   // a key-down report may be on the host; release still owed
unsigned long hidPressMs     = 0;
const int     RELAY_TAP_QUEUE_LEN = 8;
char          relayTapQueue[RELAY_TAP_QUEUE_LEN];
int           relayTapHead   = 0;
int           relayTapCount  = 0;
unsigned long lastKeyBtnTime = 0;
bool          keyBtnWasPressed = false;

// Status LED
bool          statusLedReady   = false;
bool          ledFastActive    = false;
unsigned long ledPhaseStartMs  = 0;       // start of the current fast or idle cycle
uint32_t      ledLastDuty      = 0xFFFFFFFFUL;

// Key selection (BOOT presses)
bool          keySelectActive = false;
char          keySelectChar   = 'Q';
unsigned long keySelectLastMs = 0;

// Display blink + redraw cache
unsigned long lastBlinkMs    = 0;
bool          blinkPhase     = false;
char          shownChar      = 0;
uint16_t      shownColor     = 0xFFFF;
bool          shownValid     = false;

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
static const uint32_t ESPNOW_MAGIC   = 0x51545059;  // "QTPY" — kept so Matrix units interoperate with QT Py units
static const uint8_t  ESPNOW_CHANNEL = 1;            // agreed broadcast channel
static const uint8_t  ESPNOW_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool espNowInitialized = false;


// ─── ESP-NOW packet types ──────────────────────────────────────────────────
#define ESPNOW_PKT_ANNOUNCE   0x01
#define ESPNOW_PKT_KEYSTROKE  0x02

// ─── Color helpers ─────────────────────────────────────────────────────────
// Full-scale colours; overall output is limited by MATRIX_BRIGHTNESS.
struct RGB { uint8_t r, g, b; };

const RGB COLOR_GREEN  = {0,   255, 0  };
const RGB COLOR_BLUE   = {0,   0,   255};
const RGB COLOR_ORANGE = {255, 80,  0  };
const RGB COLOR_YELLOW = {255, 200, 0  };
const RGB COLOR_RED    = {255, 0,   0  };
const RGB COLOR_PURPLE = {160, 0,   255};
const RGB COLOR_WHITE  = {200, 200, 200};
const RGB COLOR_OFF    = {0,   0,   0  };

// Draw one character centred on the matrix in colour c (COLOR_OFF = blank).
// Only pushes to the LEDs when the character or colour actually changes.
void showGlyph(char ch, RGB c) {
  uint16_t color = matrix.Color(c.r, c.g, c.b);
  if (color == 0) ch = 0;
  if (shownValid && ch == shownChar && color == shownColor) return;
  shownValid = true;
  shownChar  = ch;
  shownColor = color;
  matrix.fillScreen(0);
  if (ch) matrix.drawChar(GLYPH_X, GLYPH_Y, ch, color, 0, 1);
  matrix.show();
}

// ─── State management ──────────────────────────────────────────────────────
void setState(DeviceState s) {
  currentState = s;
  lastBlinkMs  = 0;
  blinkPhase   = false;
  LOG_PRINTF("LOG:state=%d ts=%lu\n", (int)s, millis());
}

// ─── HID output ────────────────────────────────────────────────────────────
// Map a key label (A–Z, 0–9) to a keyboard report. Letters carry Left Shift,
// matching what Keyboard.press() sent for upper-case characters.
bool buildKeyReport(char key, hid_keyboard_report_t &report) {
  memset(&report, 0, sizeof(report));
  key = toUpperCase(key);
  if (key >= 'A' && key <= 'Z') {
    report.modifier   = KEYBOARD_MODIFIER_LEFTSHIFT;
    report.keycode[0] = HID_KEY_A + (key - 'A');
  } else if (key >= '1' && key <= '9') {
    report.keycode[0] = HID_KEY_1 + (key - '1');
  } else if (key == '0') {
    report.keycode[0] = HID_KEY_0;
  } else {
    return false;
  }
  return true;
}

bool sendHidRelease() {
  hid_keyboard_report_t report = {};
  if (!HID.SendReport(HID_REPORT_ID_KEYBOARD, &report, sizeof(report))) return false;
  hidKeyDown = false;
  return true;
}

// Press one key now; handleHidOutput() releases it after HID_HOLD_MS. A key still
// down is released first so the host always sees a fresh key-down, even when the
// same key is pressed twice. Returns true only if the report reached the host.
bool hidTap(char key) {
  hid_keyboard_report_t report;
  if (!usbHostReady || !buildKeyReport(key, report)) return false;
  if (hidKeyDown) sendHidRelease();
  // A send that times out may still reach the host, so a release is always owed.
  hidKeyDown = true;
  hidPressMs = millis();
  return HID.SendReport(HID_REPORT_ID_KEYBOARD, &report, sizeof(report));
}

void queueRelayTap(char key) {
  if (relayTapCount >= RELAY_TAP_QUEUE_LEN) {
    LOG_PRINTF("LOG:RELAY_TAP_DROP key=%c ts=%lu\n", key, millis());
    return;
  }
  relayTapQueue[(relayTapHead + relayTapCount) % RELAY_TAP_QUEUE_LEN] = key;
  relayTapCount++;
}

void handleHidOutput() {
  if (hidKeyDown) {
    // Retried every loop until it lands: a lost release would leave the key
    // auto-repeating on the host.
    if (millis() - hidPressMs >= HID_HOLD_MS) sendHidRelease();
    return;
  }
  if (relayTapCount > 0) {
    char key = relayTapQueue[relayTapHead];
    relayTapHead = (relayTapHead + 1) % RELAY_TAP_QUEUE_LEN;
    relayTapCount--;
    hidTap(key);
  }
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

// ─── Display handler (non-blocking, called every loop) ─────────────────────
// Draws the selected key in the colour of the current state.
void handleDisplay() {
  unsigned long now = millis();

  // Boot window: still waiting to learn whether a host is attached
  if (usbWaitActive) {
    showGlyph(hidKey, (now / 150) % 2 ? COLOR_YELLOW : COLOR_OFF);
    return;
  }

  // Trigger held: white if the press was delivered, red if it had nowhere to go
  if (btnWasPressed) {
    showGlyph(hidKey, lastPressRouted ? COLOR_WHITE : COLOR_RED);
    return;
  }

  // Choosing a key: candidate blinks orange until it is committed
  if (keySelectActive) {
    if (now - lastBlinkMs >= 300) {
      lastBlinkMs = now;
      blinkPhase  = !blinkPhase;
    }
    showGlyph(keySelectChar, blinkPhase ? COLOR_ORANGE : COLOR_OFF);
    return;
  }

  switch (currentState) {
    case STATE_BOOT_OK:
      if (now - lastBlinkMs >= 500) {
        lastBlinkMs = now;
        blinkPhase  = !blinkPhase;
      }
      showGlyph(hidKey, blinkPhase ? COLOR_GREEN : COLOR_OFF);
      break;

    case STATE_SERIAL_CONNECTED:
      showGlyph(hidKey, COLOR_BLUE);
      break;

#if WIFI_ENABLED
    case STATE_WIFI_CONNECTING:
      if (now - lastBlinkMs >= 500) {
        lastBlinkMs = now;
        blinkPhase  = !blinkPhase;
      }
      showGlyph(hidKey, blinkPhase ? COLOR_BLUE : COLOR_OFF);
      break;

    case STATE_WS_WAITING:
      if (now - lastBlinkMs >= 500) {
        lastBlinkMs = now;
        blinkPhase  = !blinkPhase;
      }
      showGlyph(hidKey, blinkPhase ? COLOR_PURPLE : COLOR_BLUE);
      break;

    case STATE_WS_READY:
      showGlyph(hidKey, COLOR_BLUE);
      break;

    case STATE_WIFI_FAILED:
      if (wifiFailBlinkCount < 10) {
        if (now - lastBlinkMs >= 250) {
          lastBlinkMs = now;
          blinkPhase  = !blinkPhase;
          if (!blinkPhase) wifiFailBlinkCount++;
        }
        showGlyph(hidKey, blinkPhase ? COLOR_RED : COLOR_OFF);
      }
      break;
#else
    case STATE_USB_HOST:
      showGlyph(hidKey, COLOR_GREEN);
      break;

    case STATE_HOST_AWAY:
      showGlyph(hidKey, COLOR_YELLOW);
      break;

    case STATE_RELAY_READY:
      showGlyph(hidKey, COLOR_PURPLE);
      break;
#endif

    case STATE_ERROR:
      if (now - lastBlinkMs >= 250) {
        lastBlinkMs = now;
        blinkPhase  = !blinkPhase;
      }
      showGlyph(hidKey, blinkPhase ? COLOR_RED : COLOR_OFF);
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
      newKey = toUpperCase(newKey);
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

// ─── Button press action ───────────────────────────────────────────────────
// Hosted → HID keystroke, before anything else. Never hosted → ESP-NOW to the primary.
void buttonPressed() {
  bool routed = false;

  // 1. USB HID — first, ahead of every other channel. Preempts any relayed tap.
  if (usbHostReady) {
    routed = hidTap(hidKey);
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

  lastPressRouted = routed;
}

// ─── Key-select mode ─────────────────────────────────────────────────────────
// Cycle order: A–Z then 0–9.
char nextKeyChar(char c) {
  if (c >= 'A' && c < 'Z') return c + 1;
  if (c == 'Z')            return '0';
  if (c >= '0' && c < '9') return c + 1;
  return 'A';
}

// Apply the chosen key now (HID + ESP-NOW use hidKey directly) and persist it.
// Only one NVS write per selection, not one per press.
void commitKeySelect() {
  keySelectActive = false;
  if (keySelectChar == hidKey) {
    LOG_PRINTLN("LOG:KEYSEL_UNCHANGED");
    return;
  }
  hidKey = keySelectChar;
  prefs.begin("remote", false);
  prefs.putUChar("hid_key", (uint8_t)hidKey);
  prefs.end();
  LOG_PRINTF("LOG:KEYSEL_SAVED key=%c ts=%lu\n", hidKey, millis());
  PROTO_PRINTF("CONFIG:key=%c\n", hidKey);
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

// Called on every trigger press; a press during fast mode restarts its timer.
void startStatusLedFast() {
  ledFastActive   = true;
  ledPhaseStartMs = millis();
}

void handleStatusLed() {
  if (!statusLedReady) return;
  unsigned long now = millis();

  if (ledFastActive) {
    unsigned long t = now - ledPhaseStartMs;
    if (t < LED_FAST_DURATION_MS) {
      writeStatusLed((t % LED_FAST_PERIOD_MS) < LED_FAST_PERIOD_MS / 2 ? STATUS_LED_MAX_DUTY : 0);
      return;
    }
    ledFastActive   = false;
    ledPhaseStartMs = now;   // restart the idle cycle from a dark phase
  }

  // Idle: triangle fade over LED_IDLE_PULSE_MS, squared for perceptually even
  // brightness, then dark for the rest of LED_IDLE_PERIOD_MS.
  unsigned long t = (now - ledPhaseStartMs) % LED_IDLE_PERIOD_MS;
  uint32_t duty = 0;
  if (t < LED_IDLE_PULSE_MS) {
    float x   = (float)t / LED_IDLE_PULSE_MS;
    float tri = 1.0f - fabsf(2.0f * x - 1.0f);
    duty = (uint32_t)(tri * tri * STATUS_LED_MAX_DUTY);
  }
  writeStatusLed(duty);
}

// ─── Trigger button (50ms debounce) ────────────────────────────────────────
// Keystroke fires on press for lowest latency. During the boot USB wait the press
// is held and sent by handleHostEnumeration() once the delivery path is known.
void handleTriggerButton() {
  bool rawReading  = digitalRead(TRIGGER_BTN_PIN);
  unsigned long now = millis();

  if (rawReading == LOW && !btnWasPressed) {
    if (now - lastBtnTime >= DEBOUNCE_MS) {
      lastBtnTime   = now;
      btnWasPressed = true;
      if (usbWaitActive) pendingTriggerPress = true;
      else               buttonPressed();
      startStatusLedFast();
    }
  } else if (rawReading == HIGH && btnWasPressed) {
    lastBtnTime   = now;
    btnWasPressed = false;
  }
}

// ─── BOOT button: key selection (50ms debounce) ────────────────────────────
// Each press steps to the next key; the choice is committed after
// KEY_SELECT_COMMIT_MS without further presses.
void handleKeyButton() {
  bool rawReading  = digitalRead(KEY_BTN_PIN);
  unsigned long now = millis();

  if (rawReading == LOW && !keyBtnWasPressed) {
    if (now - lastKeyBtnTime >= DEBOUNCE_MS) {
      lastKeyBtnTime   = now;
      keyBtnWasPressed = true;
      if (!keySelectActive) {
        keySelectActive = true;
        keySelectChar   = hidKey;
      }
      keySelectChar   = nextKeyChar(keySelectChar);
      keySelectLastMs = now;
      blinkPhase      = true;   // show the new key immediately
      lastBlinkMs     = now;
      LOG_PRINTF("LOG:KEYSEL key=%c ts=%lu\n", keySelectChar, now);
    }
  } else if (rawReading == HIGH && keyBtnWasPressed) {
    lastKeyBtnTime   = now;
    keyBtnWasPressed = false;
  }

  if (keySelectActive && !keyBtnWasPressed && now - keySelectLastMs >= KEY_SELECT_COMMIT_MS) {
    commitKeySelect();
  }
}

// ─── ESP-NOW initialization ─────────────────────────────────────────────────
void initEspNow() {
  if (WiFi.getMode() == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
  }
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    if ((bool)Serial) Serial.println("ERR:ESPNOW_INIT_FAILED");
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
  // 1. LED matrix
  matrix.begin();
  matrix.setBrightness(MATRIX_BRIGHTNESS);
  matrix.setTextWrap(false);
  showGlyph(0, COLOR_OFF);

  // 2. Buttons + external status LED
  pinMode(TRIGGER_BTN_PIN, INPUT_PULLUP);
  pinMode(KEY_BTN_PIN, INPUT_PULLUP);
  initStatusLed();

  // 3. USB composite device setup — ORDER IS CRITICAL
  prefs.begin("remote", true);
  hidKey = (char)prefs.getUChar("hid_key", 'Q');
#if WIFI_ENABLED
  String wifiSSID_stored = prefs.getString("wifi_ssid", "");
  String wifiPass_stored = prefs.getString("wifi_pass", "");
#endif
  prefs.end();
  if (!isAlphaNumeric(hidKey)) hidKey = 'Q';
  hidKey = toUpperCase(hidKey);

  Keyboard.begin();
  Serial.begin(115200);
  USB.manufacturerName("Andrey");
  char productName[32];
  snprintf(productName, sizeof(productName), "Matrix Remote [%c]", hidKey);
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
    if (pendingTriggerPress) {
      pendingTriggerPress = false;
      buttonPressed();
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
  handleDisplay();
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
