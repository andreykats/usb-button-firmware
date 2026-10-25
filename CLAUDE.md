# USB Button — CLAUDE.md

Branch `ws-esp32s3-zero`: port of the `main` branch firmware (Waveshare ESP32-S3-Matrix,
itself a port of `~/Sidetable/esp32s3-com-poc`, the QT Py firmware) to the
Waveshare ESP32-S3-Zero.
README.md is for end users (behaviour, key selection, lights). Board settings, compile-time
flags and the Web Serial protocol are documented in the sketch header.

## Layout

```
firmware/firmware.ino   ← single-file Arduino sketch
website/                ← Web Serial browser app (only with WEB_SERIAL_ENABLED=1)
resources/              ← schematics, pinout, datasheets, Waveshare demo code (reference only)
HARDWARE.md             ← pinout, wiring, parts — keep in sync with pin #defines
```

## Key facts

- Pins (from resources/ESP32-S3-Zero-Sch.pdf): BOOT button GPIO0 (only onboard user
  button), WS2812B RGB LED GPIO21 in **RGB** colour order (`STATUS_RGB_ORDER`). The 5V pad
  is VBUS directly (no diode, unlike the Matrix).
- Buttons (polled every `BUTTON_POLL_MS` = 10 ms): external trigger on `TRIGGER_BTN_PIN`
  (GPIO13, to GND) types the selected key. BOOT steps the key (a–z, 0–9), applies it at
  once and types it via the same path (`keyPressed()` → `sendSelectedKey()`), then saves it
  to NVS. The key is also saved at every boot. Default `'1'`. The USB product name updates
  at next power-up.
- Key set and behaviour are kept identical to the `qtpy-samd21` branch: lower-case
  letters, no Shift; key down on press, up on release (held = host auto-repeat).
- RGB LED (`handleStatusRgb()`, full brightness): state colour, replaced by white
  (delivered) / red (not delivered, `pressDelivered`) while a button is held. Never shows
  the selected key. `STATE_ERROR` (ESP-NOW init failure) blinks red until reboot and is never left.
- External status LED on `STATUS_LED_PIN` (GPIO1, LEDC PWM → transistor → 5V LED):
  idle continuous 3 s breathing, fast blink while the trigger or BOOT is held (`handleStatusLed()`).
- No VBUS sense pin. Host state is polled from TinyUSB in `handleHostEnumeration()`:
  `usbHostReady` = mounted && !suspended; `usbHostSeen` latches on first mount and is
  never cleared. Host sleep / unplug from a self-powered hub shows up as suspend, not unmount.
- Hosted (`usbHostSeen`) → HID + ESP-NOW primary; own presses never go over ESP-NOW,
  even while the host is away. Never hosted → ESP-NOW remote (`sendKeystroke()`).
- HID reports go through `HID.SendReport()` (`sendKeyReport()` / `hidPress()` /
  `handleHidOutput()`), not `Keyboard.press()`, which hides send failures. One key at a
  time: the local key is held while either button is down and cuts a relayed tap short;
  relayed keystrokes (`HID_HOLD_MS` taps) queue behind it. ESP-NOW carries presses only,
  so a remote's hold never repeats on the primary. Keep `loop()` order: host tracking,
  trigger, HID, then the rest.
- `ESPNOW_MAGIC` stays "QTPY" for interoperability with QT Py and Matrix units.
- WiFi power save is `WIFI_PS_NONE` (no battery; the primary must not miss relayed packets).

## Conventions

- Non-blocking `handle*()` functions called from `loop()`.
- Web Serial protocol output goes through `PROTO_PRINTF/PROTO_PRINTLN`
  (compiled out unless `WEB_SERIAL_ENABLED=1`). Diagnostics use `LOG_PRINTF`.
  `ERR:` lines print whenever a CDC terminal is open.
- Functions must be declared before use. Arduino's ctags prototype generation is
  unavailable on this Mac (x86_64 binary, no Rosetta), so builds use
  `--build-property "tools.ctags.pattern=/usr/bin/true"`.

## Build

The board's menu defaults to Hardware CDC USB mode, so `USBMode=default` (TinyUSB) is
required; `CDCOnBoot=default` means Enabled on this board.

```bash
FQBN='esp32:esp32:waveshare_esp32_s3_zero:USBMode=default,CDCOnBoot=default,UploadMode=cdc,PSRAM=enabled'
arduino-cli compile -b "$FQBN" --build-property "tools.ctags.pattern=/usr/bin/true" firmware
```

Web app (only with `WEB_SERIAL_ENABLED=1`): `python3 website/serve.py` opens Chrome at
http://127.0.0.1:8080.

Ask the user to do any physical hardware action (plugging in, pressing buttons).
