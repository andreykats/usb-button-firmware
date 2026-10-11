# ESP32-S3 Matrix Button — CLAUDE.md

Port of `~/Sidetable/esp32s3-com-poc` (QT Py firmware) to the Waveshare ESP32-S3-Matrix.
See README.md for user-facing behaviour, board settings and the status colour table.

## Layout

```
matrix-firmware/matrix-firmware.ino   ← single-file Arduino sketch
website/                              ← Web Serial browser app (only with WEB_SERIAL_ENABLED=1)
resources/                            ← Waveshare schematic, datasheets, demo code (reference only)
HARDWARE.md                           ← pinout, wiring, parts — keep in sync with pin #defines
```

## Key facts

- Pins (from resources/ESP32-S3-Matrix-Sch.pdf): BOOT button GPIO0 (only onboard user
  button), WS2812B 8x8 matrix GPIO14, QMI8658 SDA 11 / SCL 12 / INT1 10 / INT2 13.
- Buttons: external trigger on `TRIGGER_BTN_PIN` (GPIO1, to GND) sends the keystroke;
  BOOT steps the key, committed (applied + one NVS write) 3 s after the last press —
  no reboot, USB product name updates at next power-up.
- External status LED on `STATUS_LED_PIN` (GPIO2, LEDC PWM → transistor → 5V LED):
  idle soft pulse every 3 s, fast blink 3 s after a trigger press (`handleStatusLed()`).
- No VBUS sense pin. Host state is polled from TinyUSB in `handleHostEnumeration()`:
  `usbHostReady` = mounted && !suspended; `usbHostSeen` latches on first mount and is
  never cleared. Host sleep / unplug from a self-powered hub shows up as suspend, not unmount.
- Hosted (`usbHostSeen`) → HID + ESP-NOW primary; own presses never go over ESP-NOW,
  even while the host is away. Never hosted → ESP-NOW remote (`sendKeystroke()`).
- HID reports go through `HID.SendReport()` (`hidTap()` / `handleHidOutput()`), not
  `Keyboard.press()`, which hides send failures. One tap at a time; the local trigger
  preempts queued relay taps. Keep `loop()` order: host tracking, trigger, HID, then the rest.
- `ESPNOW_MAGIC` stays "QTPY" for interoperability with QT Py units.
- Matrix layout/colour order copied from Waveshare's Font demo
  (`TOP+RIGHT+COLUMNS+PROGRESSIVE`, `NEO_GRB`). The Game demo uses `NEO_RGB`;
  if colours or glyph orientation look wrong on hardware, adjust the `matrix(...)` flags.
- Waveshare warns high LED brightness overheats the board — keep `MATRIX_BRIGHTNESS` low.
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

```bash
FQBN='esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,UploadMode=cdc,FlashSize=4M,PSRAM=enabled'
arduino-cli compile -b "$FQBN" --build-property "tools.ctags.pattern=/usr/bin/true" matrix-firmware
```

Ask the user to do any physical hardware action (plugging in, pressing buttons).
