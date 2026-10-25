# USB Button — CLAUDE.md

Branch `qtpy-samd21`: the basic (non-wireless) button on an Adafruit QT Py SAMD21 (QT Py M0).
Key behaviour, key set and status colours are kept identical to the `ws-esp32s3-zero`
branch, minus its wireless (blue) states. Change both branches together.

## Layout

```
firmware/firmware.ino      ← single-file Arduino sketch
README.md                  ← for end users only: what it does, key selection, lights, hub power
HARDWARE.md                ← only the pins and parts actually used — keep in sync with pin #defines
resources/adafruit-qt-py.pdf  ← Adafruit guide (pinout on page 18)
```

## Key facts

- Pins: select A0, trigger A3 (buttons to GND, internal pull-ups), external LED A10 (MO,
  TCC0 WO[2]), NeoPixel D11 (GRB; its power pin D12 is high by default).
- Keys a–z then 0–9, no Shift (lower case), default `'1'`. Key down on press, up once both
  buttons are released (held = host auto-repeat). Select steps the key and types it.
- HID reports go through `HID().SendReport(2, KeyReport)`, not `Keyboard.press()`, which
  hides send failures; success is a return of `sizeof(report) + 1`. `Keyboard.begin()`
  must stay: it registers the HID descriptor.
- The key is written on every select press and at every boot, then read back; a mismatch
  latches the error state (red blink). FlashStorage is zero-filled on every upload, which
  reads as invalid → default key.
- Host state: the core doesn't track suspend. `usbHostReady` = `USBDevice.configured()` and
  the SOF frame number (`USB->DEVICE.FNUM`) changed within 10 ms. Presses during the 4 s
  boot window are held until it ends.
- External LED: TCC0 is set up directly (10-bit, 48 MHz / 8 / 1024 ≈ 5.9 kHz) because
  `analogWrite()` is 8-bit at ~730 Hz. Breathing uses `BREATH_TABLE` (no soft-float at run
  time). The hardware is an NPN emitter follower from 5V with **no resistors** (LED ≈ 2.6 V,
  self-limiting): don't enable strong drive (`DRVSTR`) on the pin.
- Buttons are polled every 10 ms with no blocking delays. Keep `loop()` order: host
  tracking, trigger, HID, select, then the lights.

## Conventions

- Non-blocking `handle*()` functions called from `loop()`.
- Functions must be declared before use (ctags prototype generation is disabled, below).

## Build and upload (this Mac)

The Adafruit core's `arm-none-eabi-gcc` and `bossac` are x86_64-only and this Mac has no
Rosetta. Use the native ARM GNU toolchain in `~/.local/arm-gnu-toolchain/`; the Keyboard
library lives outside arduino-cli's search path.

```bash
TC=~/.local/arm-gnu-toolchain/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin/
arduino-cli compile -b adafruit:samd:adafruit_qtpy_m0 \
  --build-property "tools.ctags.pattern=/usr/bin/true" \
  --build-property "compiler.path=$TC" \
  --library ~/Library/Arduino15/libraries/Keyboard --output-dir /tmp/qtpy-build firmware
```

Upload through the UF2 bootloader instead of bossac: convert `firmware.ino.bin` to UF2
(base address 0x2000, SAMD21 family ID 0x68ed2b88, per github.com/microsoft/uf2), open the
port at 1200 baud (`stty -f /dev/cu.usbmodemXXXX 1200`) to enter the bootloader, then copy
the `.uf2` to the `QTPY_BOOT` drive.

Ask the user to do any physical hardware action (plugging in, pressing buttons).
