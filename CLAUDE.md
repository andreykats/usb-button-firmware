# USB Button — CLAUDE.md

Branch `ws-esp32s3-zero`: port of the `main` branch firmware (Waveshare ESP32-S3-Matrix,
itself a port of `~/Sidetable/esp32s3-com-poc`, the QT Py firmware) to the
Waveshare ESP32-S3-Zero.
See README.md for user-facing behaviour, board settings and the status colour table.

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
- Buttons: external trigger on `TRIGGER_BTN_PIN` (GPIO1, to GND) types the selected key.
  BOOT steps the key, applies it at once and types it via the same path
  (`keyPressed()` → `sendSelectedKey()`); the NVS write happens 3 s after the last press.
  The USB product name updates at next power-up.
- RGB LED (`handleStatusRgb()`): state colour, overlaid by a short white (delivered) /
  red (not delivered) flash after each press (`flashStatus()`). `STATE_ERROR` (ESP-NOW
  init failure) blinks red until reboot and is never left.
- External status LED on `STATUS_LED_PIN` (GPIO2, LEDC PWM → transistor → 5V LED):
  idle soft pulse every 3 s, fast blink 3 s after a trigger press (`handleStatusLed()`).
- No VBUS sense pin. Host state is polled from TinyUSB in `handleHostEnumeration()`:
  `usbHostReady` = mounted && !suspended; `usbHostSeen` latches on first mount and is
  never cleared. Host sleep / unplug from a self-powered hub shows up as suspend, not unmount.
- Hosted (`usbHostSeen`) → HID + ESP-NOW primary; own presses never go over ESP-NOW,
  even while the host is away. Never hosted → ESP-NOW remote (`sendKeystroke()`).
- HID reports go through `HID.SendReport()` (`hidTap()` / `handleHidOutput()`), not
  `Keyboard.press()`, which hides send failures. One tap at a time; local presses
  preempt queued relay taps. Keep `loop()` order: host tracking, trigger, HID, then the rest.
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

Ask the user to do any physical hardware action (plugging in, pressing buttons).
