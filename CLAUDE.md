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
README.md               ← for end users only: what it does, key selection, lights, hub power
HARDWARE.md             ← only the pins and parts actually used — keep in sync with pin #defines
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
- Key set, behaviour and status colours are kept identical to the `qtpy-samd21` branch:
  lower-case letters, no Shift; key down on press, up on release (held = host auto-repeat).
  Change both branches together.
- RGB LED (`handleStatusRgb()`, full brightness): state colour, replaced by white
  (delivered) / red (not delivered, `pressDelivered`) while a button is held. Never shows
  the selected key. `STATE_ERROR` (ESP-NOW init failure) blinks red until reboot and is never left.
- External status LED on `STATUS_LED_PIN` (GPIO1, LEDC PWM): idle continuous 3 s breathing,
  fast blink while the trigger or BOOT is held (`handleStatusLed()`). The hardware is an NPN
  emitter follower from 5V with **no resistors** (LED ≈ 2.6 V, self-limiting).
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

## Fleet flashing (this Mac)

`~/.local/usb-button-tools/flash_fleet.py` (outside the repo; also has a `qtpy` profile for
the `qtpy-samd21` branch). esptool 5.x from the esp32 core is native arm64. Build with
`--output-dir`, then copy `firmware.ino.bootloader.bin`, `.partitions.bin`, `.bin` and the
core's `tools/partitions/boot_app0.bin` into `zero-<commit>/` as `bootloader.bin`,
`partitions.bin`, `app.bin`, `boot_app0.bin`.

```bash
python3 -I flash_fleet.py zero zero-<commit> --watch --until N
```

Each board not yet logged ok is flashed as `board-NN`, verified (same USB location back as
303A:822B with a HID interface) and logged by USB serial (the MAC) to `flash-log-zero.csv`.

- Boards in USB-Serial/JTAG mode (303A:1001: factory firmware or ROM) are reset into
  download mode by esptool. The running sketch (303A:822B) gets a 1200-baud touch first.
  No serial port at all: hold BOOT while plugging in.
- `write-flash --erase-all`: NVS starts empty, so every board starts on the default key.
- `--after watchdog-reset`, not `hard-reset`: over USB-Serial/JTAG `hard-reset` is only a
  core reset, and the sketch then enumerated with no interfaces (no HID, no CDC) until
  power-cycled.
- The product string read `ESP32-S3-Zero` / `ESP32_S3_Zero`, not `Zero Remote [1]`, on the
  first boot and after one power cycle (2026-10-25).

Ask the user to do any physical hardware action (plugging in, pressing buttons).
