# USB Button — CLAUDE.md

Branch `ws-esp32s3-matrix`: firmware for the Waveshare ESP32-S3-Matrix, kept in step with
`ws-esp32s3-zero` (itself matching `qtpy-samd21`; originally a port of
`~/Sidetable/esp32s3-com-poc`, the QT Py firmware). The sketch is the Zero's sketch with
the Matrix hardware layer on top (pins, 8×8 display, USB ID, names): port Zero changes by
re-applying that layer rather than editing the two apart.
README.md is for end users (behaviour, key selection, lights). Board settings, compile-time
flags and the Web Serial protocol are documented in the sketch header.

## Layout

```
firmware/firmware.ino   ← single-file Arduino sketch
resources/              ← schematic, datasheets, Waveshare demo code (reference only)
README.md               ← for end users only: what it does, key selection, lights, hub power
HARDWARE.md             ← only the pins and parts actually used — keep in sync with pin #defines
```

The settings page (https://button.drey.dev) and the debug page live on their own branch,
`web-serial-interface` (see its CLAUDE.md). Deferred fixes shared with the Zero are in
`TODO.md` on `ws-esp32s3-zero`.

## Customizable features → update `web-serial-interface` too

Settings and the settings protocol are shared with `ws-esp32s3-zero`: change them there
first (its CLAUDE.md), then port here. The page finds this board by its USB ID 303A:81FB
(`DEVICE_FILTERS` in `index.html`). Also update the settings table in README.md here.

## Key facts

- Pins (from resources/ESP32-S3-Matrix-Sch.pdf): BOOT button GPIO0 (only onboard user
  button), WS2812B 8x8 matrix GPIO14, QMI8658 SDA 11 / SCL 12 / INT1 10 / INT2 13 (unused,
  so GPIO13 is not free as on the Zero). The 5V pad is VBUS after a Schottky diode (≈ 4.7 V).
- USB ID 303A:81FB, set in `setup()` (`USB.VID()` / `USB.PID()`): Waveshare's ID for this
  board, from the core's `ws_esp32_s3_matrix` profile. The variant header's own 303A:1001
  is the ID of any ESP32-S3 in ROM download mode.
- Buttons (polled every `BUTTON_POLL_MS` = 10 ms): external trigger on `TRIGGER_BTN_PIN`
  (GPIO1, to GND) types the selected key on press or release (setting `edge`); on press
  with `repeat` on it is held while the button is (host auto-repeat), else a
  `HID_HOLD_MS` tap. BOOT steps the key (a–z, 0–9) on release (only of a press seen
  in this boot, and released before `SETTINGS_HOLD_MS`), applies it at once, types it
  as a tap via the same path (`keyPressed()` → `sendSelectedKey()`), then saves it to
  NVS. On release, not press, so a hold into settings mode types and changes nothing.
  The key is also saved at every boot. Default `'1'`. The USB product name
  ("Matrix Remote [x]") updates at the next restart or power-up.
- Settings mode: BOOT held `SETTINGS_HOLD_MS` (3 s) sets NVS flag `settings` and calls
  `ESP.restart()`. `setup()` reads and clears the flag, so any later restart, RESET or
  power cycle is normal mode. Settings mode registers CDC only (named "USB Button [key]
  (EB:F4:58)", the last 3 MAC bytes, as on the Zero), no ESP-NOW, the key drawn cycling
  the rainbow from purple, serves the settings protocol (`parseSettingsLine()`), reports
  trigger edges for the page's test (`reportTriggerTest()`), and a new BOOT press
  restarts. `ESP.restart()` is a core reset, which does not re-sample strapping pins
  (TRM Table 7-1): BOOT held through it does not enter download mode. BOOT held at boot
  is taken as already pressed.
- Settings (`SETTINGS[]` table, NVS, applied live): `edge` press/release, `repeat`
  on/off, `pressed` fast/on/slow/off, `idle` breathe/fast/slow/off, `rgb` normal/high/low
  = matrix brightness 10/30/1 % (`rgbPercent`, scaled to 0–255 in `showStatus()`).
  First choice is the default. "On" is a 125 Hz blink (`LED_FLICKER_PERIOD_MS`), not
  steady. The key is set with `SET:key=x` too (`setKeyFromSettings()`), but isn't in
  `SETTINGS[]`: BOOT also steps it, and `DEFAULTS` leaves it alone.
- Waveshare warns high LED brightness overheats the board — keep the `rgb` choices low.
- Key set, behaviour and status colours are kept identical to the `ws-esp32s3-zero` and
  `qtpy-samd21` branches: lower-case letters, no Shift. Change all branches together.
- Matrix (`handleDisplay()` → `showStatus()`): the selected key drawn in the state colour,
  replaced by white (delivered) / red (not delivered, `pressDelivered`) while the trigger
  is held (BOOT and edge=release: `PRESS_FEEDBACK_MS` after release, `releaseFeedback`).
  Same colours as the Zero's RGB LED. `STATE_ERROR` (ESP-NOW init failure) blinks red
  until reboot and is never left.
- Matrix layout copied from Waveshare's Font demo (`TOP+RIGHT+COLUMNS+PROGRESSIVE`),
  plus `MATRIX_ROTATION` = 3 (90° CCW), which makes glyphs upright on the hardware.
  Colour order `NEO_RGB`, as in Waveshare's Color and Game demos: the Font demo's
  `NEO_GRB` showed green as red on hardware.
- Key glyphs are the sketch's own 6-wide x 8-tall table, full height (`KEY_GLYPHS`:
  capitals, then digits), not a GFX font. Every glyph's lit area has even width and height, so `drawCentredChar()`
  centres it exactly on the 8x8 grid; only glyphs that would otherwise be odd (1, I, T,
  Y) get 2-pixel strokes. Keep that rule when editing a glyph.
- External status LED on `STATUS_LED_PIN` (GPIO2, LEDC PWM): `idle` pattern (default 3 s
  breathing), `pressed` pattern while the trigger or BOOT is held (default fast blink)
  (`handleStatusLed()`). The hardware is an NPN emitter follower from 5V with **no
  resistors** (LED ≈ 2.6 V, self-limiting).
- No VBUS sense pin. Host state is polled from TinyUSB in `handleHostEnumeration()`:
  `usbHostReady` = mounted && !suspended; `usbHostSeen` latches on first mount and is
  never cleared. Host sleep / unplug from a self-powered hub shows up as suspend, not unmount.
- Hosted (`usbHostSeen`) → HID + ESP-NOW primary; own presses never go over ESP-NOW,
  even while the host is away. Never hosted → ESP-NOW remote (`sendKeystroke()`).
- HID reports go through `hid->SendReport()` (`sendKeyReport()` / `hidPress()` /
  `handleHidOutput()`), not `Keyboard.press()`, which hides send failures. One key at a
  time: a local press (hold or tap) cuts a relayed tap short; relayed keystrokes
  (`HID_HOLD_MS` taps) queue behind it. ESP-NOW carries presses only,
  so a remote's hold never repeats on the primary. Keep `loop()` order: host tracking,
  trigger, HID, then the rest.
- `ESPNOW_MAGIC` stays "QTPY" for interoperability with QT Py and Zero units.
- Long uptimes: the loop task is on the core's task watchdog (`enableLoopWDT()`, 5 s,
  resets), so nothing in `loop()` may block for seconds. Timers compare `now - start`
  (or `(long)(now - deadline) >= 0`), never `now >= deadline`; a stored timestamp
  that is only valid for a while gets a flag that is cleared when it expires, since
  `millis()` wraps after 49.7 days.
- WiFi power save is `WIFI_PS_NONE` (no battery; the primary must not miss relayed packets).

## Conventions

- Non-blocking `handle*()` functions called from `loop()`.
- All CDC output goes through `Cdc` (never `Serial`, which is UART0 with CDC On Boot
  disabled). Debug Web Serial protocol output goes through `PROTO_PRINTF/PROTO_PRINTLN`
  (compiled out unless `WEB_SERIAL_ENABLED=1`); the settings protocol is always built in.
  Diagnostics use `LOG_PRINTF`. `ERR:` lines print whenever a CDC terminal is open.
- Functions must be declared before use. Arduino's ctags prototype generation is
  unavailable on this Mac (x86_64 binary, no Rosetta), so builds use
  `--build-property "tools.ctags.pattern=/usr/bin/true"`.

## Build

Needs the Adafruit NeoMatrix library (with Adafruit GFX + Adafruit NeoPixel). Board
profile `ws_esp32_s3_matrix` (Waveshare ESP32-S3-Matrix). Its menu defaults to Hardware
CDC USB mode, so `USBMode=default` (TinyUSB) is required. `CDCOnBoot=default` means
Disabled here (on the Zero's profile it is `cdc`): with it Enabled the core calls
`USB.begin()` before `setup()`, so interfaces, USB ID and product name set in `setup()`
are ignored. The sketch creates `USBCDC Cdc(0)` itself (the 1200-baud upload touch still
works).

```bash
FQBN='esp32:esp32:ws_esp32_s3_matrix:USBMode=default,CDCOnBoot=default,UploadMode=cdc,PSRAM=enabled'
arduino-cli compile -b "$FQBN" --build-property "tools.ctags.pattern=/usr/bin/true" firmware
```

Settings page and debugger: branch `web-serial-interface` (`python3 -I serve.py` there).

Ask the user to do any physical hardware action (plugging in, pressing buttons).
