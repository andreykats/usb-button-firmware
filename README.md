# USB Button

A port of the ESP32-S3-Matrix firmware on `main` (itself a port of
[esp32s3-com-poc](../../Sidetable/esp32s3-com-poc), the QT Py firmware) to the
[Waveshare ESP32-S3-Zero](https://docs.waveshare.com/ESP32-S3-Zero). Pressing an external
trigger button types a configurable key; the onboard BOOT button steps to the next key and
types it. The onboard RGB LED shows the device state.

## How it works

The delivery path is chosen automatically by whether a USB host enumerates the board
(the board has no VBUS sense pin, so enumeration is the only signal). Trigger and BOOT
presses take the same path:

| Plugged into | Behaviour |
|---|---|
| A computer | **USB HID keyboard**. The unit also becomes the ESP-NOW **primary** and types keystrokes relayed from other units; its own presses always go first. |
| USB power only (charger) | Keystroke is sent to the primary via **ESP-NOW** unicast; the primary types it on its computer. |

Once a computer has enumerated the unit, it stays a HID unit until it is power-cycled. If
the computer sleeps or is unplugged (e.g. from a self-powered hub), the unit waits for it
to come back instead of switching to ESP-NOW. A unit powered from a hub before the
computer is attached starts as an ESP-NOW remote and switches to HID when the computer
enumerates it.

Zero units use the same ESP-NOW packet format as the Matrix and QT Py firmware, so all
three can relay for each other.

Web Serial and WiFi/WebSocket are **disabled by default** (compile-time flags, below).

## Hardware

- **Board**: Waveshare ESP32-S3-Zero — ESP32-S3FH4R2 (4 MB flash, 2 MB quad PSRAM)
- **Trigger button**: external, GPIO1 → GND
- **Status LED**: external, transistor-switched 5 V, driven from GPIO2 (PWM)
- **Key button**: onboard BOOT (GPIO0)
- **Status display**: onboard WS2812B RGB LED on GPIO21

See **[HARDWARE.md](HARDWARE.md)** for the header pinout, wiring diagrams and part
recommendations. The schematic and pinout are in [resources/](resources/).

## Setup

### Arduino IDE board settings (esp32 core 3.3.5+)

| Setting | Value |
|---|---|
| Board | Waveshare ESP32-S3-Zero |
| PSRAM | Enabled |
| USB Mode | USB-OTG (TinyUSB) — **required for HID**; the board defaults to Hardware CDC |
| USB CDC On Boot | Enabled (keeps a CDC port for uploads/logs) |
| Upload Mode | USB-OTG CDC (TinyUSB) |

Setting *USB CDC On Boot* to Disabled makes the board a pure HID device; you then flash it
by holding BOOT and tapping RESET to enter download mode.

**Libraries:** none for the default build (the RGB LED uses the core's `rgbLedWriteOrdered()`).
WiFi mode needs the ESP32Async forks of `ESPAsyncWebServer` and `AsyncTCP`.

Open `firmware/firmware.ino` and upload.

### Command line

```bash
FQBN='esp32:esp32:waveshare_esp32_s3_zero:USBMode=default,CDCOnBoot=default,UploadMode=cdc,PSRAM=enabled'
arduino-cli compile -b "$FQBN" firmware
arduino-cli upload  -b "$FQBN" -p <port> firmware
```

This board's menu names the options differently from *ESP32S3 Dev Module*:
`USBMode=default` is TinyUSB and `CDCOnBoot=default` is Enabled.

On Apple Silicon without Rosetta, the Arduino-bundled `ctags` (x86_64 only) fails with
`bad CPU type in executable`. `ctags` only auto-generates function prototypes, and this
sketch declares every function before use, so skip that step:
`--build-property "tools.ctags.pattern=/usr/bin/true"`.

### Compile-time flags

| Flag | Default | Effect |
|---|---|---|
| `WEB_SERIAL_ENABLED` | 0 | Web Serial protocol for the browser app in [website/](website/). Needs *USB CDC On Boot: Enabled*. |
| `WIFI_ENABLED` | 0 | WebSocket over WiFi + encrypted ESP-NOW credential sharing. **Replaces** the ESP-NOW keystroke relay. |
| `ENABLE_VERBOSE_LOGS` | 0 | `LOG:` diagnostics over the CDC port. |

Set them at the top of the sketch, or with
`--build-property "compiler.cpp.extra_flags=-DWEB_SERIAL_ENABLED=1"`.

## Choosing the key

The key (A–Z, 0–9, default `Q`) is stored in flash.

1. Press BOOT — the key steps to the next one (A–Z, then 0–9) and is typed straight away,
   so you can see it in any text field. Without a computer, it is sent over ESP-NOW and
   typed by the primary.
2. Keep pressing until the key you want appears. The trigger types the new key at once.
3. Stop pressing: 3 s later the key is saved to flash (no reboot).

The USB product name (`Zero Remote [Q]`) updates at the next power-up. With
`WEB_SERIAL_ENABLED=1` the browser app can also set the key (`KEY:x`, then the device reboots).

## RGB LED status guide

| LED | Meaning |
|---|---|
| Yellow, fast blink | Booting: waiting for USB enumeration (up to 4 s) |
| Green, solid | Computer connected: HID + primary hub (also with Web Serial open) |
| Green, blinking | Computer was connected but is asleep or unplugged — waiting for it |
| Blue, solid | Power only, connected to the mesh through a primary |
| Blue, blinking | Power only, looking for a primary |
| Red, blinking | Error (ESP-NOW failed to start) — reboot to clear |
| White flash | Button press delivered (trigger or BOOT) |
| Red flash | Button press not delivered (power only with no primary, or computer asleep/unplugged) |

WiFi-mode colours are listed in the sketch header.

## Web app (optional)

Only useful with `WEB_SERIAL_ENABLED=1`:

```bash
python3 website/serve.py          # opens Chrome at http://127.0.0.1:8080
```
