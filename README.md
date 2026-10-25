# ESP32-S3 Matrix Button

A port of [esp32s3-com-poc](../../Sidetable/esp32s3-com-poc) (QT Py firmware) to the
[Waveshare ESP32-S3-Matrix](https://docs.waveshare.com/ESP32-S3-Matrix). Pressing an
external trigger button sends a configurable keystroke; the onboard BOOT button picks the
key. The selected key is shown on the 8×8 RGB matrix, drawn in a colour that reflects the
device state.

## How it works

The delivery path is chosen automatically by whether a USB host enumerates the board
(the board has no VBUS sense pin, so enumeration is the only signal):

| Plugged into | Behaviour |
|---|---|
| A computer | **USB HID keyboard**. The unit also becomes the ESP-NOW **primary** and types keystrokes relayed from other units; its own trigger always goes first. |
| USB power only (charger) | Keystroke is sent to the primary via **ESP-NOW** unicast; the primary types it on its computer. |

Once a computer has enumerated the unit, it stays a HID unit until it is power-cycled. If
the computer sleeps or is unplugged (e.g. from a self-powered hub), the unit waits for it
to come back instead of switching to ESP-NOW. A unit powered from a hub before the
computer is attached starts as an ESP-NOW remote and switches to HID when the computer
enumerates it.

Matrix units use the same ESP-NOW packet format as the QT Py firmware, so the two can
relay for each other.

Web Serial and WiFi/WebSocket are **disabled by default** (compile-time flags, below).

## Hardware

- **Board**: Waveshare ESP32-S3-Matrix — ESP32-S3FH4R2 (4 MB flash, 2 MB quad PSRAM)
- **Trigger button**: external, GPIO1 → GND
- **Status LED**: external, transistor-switched 5 V, driven from GPIO2 (PWM)
- **Key button**: onboard BOOT (GPIO0)
- **Display**: onboard 8×8 WS2812B matrix on GPIO14

See **[HARDWARE.md](HARDWARE.md)** for the header pinout, wiring diagrams and part
recommendations. Schematic, datasheets and Waveshare demo code are in [resources/](resources/).

> Keep `MATRIX_BRIGHTNESS` low — Waveshare warns that high brightness overheats the board.

## Setup

### Arduino IDE board settings (esp32 core 3.3.x)

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash Size | 4MB (32Mb) |
| PSRAM | QSPI PSRAM |
| USB Mode | USB-OTG (TinyUSB) — **required for HID** |
| USB CDC On Boot | Enabled (keeps a CDC port for uploads/logs) |
| Upload Mode | USB-OTG CDC (TinyUSB) |

Setting *USB CDC On Boot* to Disabled makes the board a pure HID device; you then flash it
by holding BOOT and tapping RESET to enter download mode.

**Libraries:** `Adafruit NeoMatrix` (pulls in Adafruit GFX + Adafruit NeoPixel).
WiFi mode additionally needs the ESP32Async forks of `ESPAsyncWebServer` and `AsyncTCP`.

Open `firmware/firmware.ino` and upload.

### Command line

```bash
arduino-cli compile -b 'esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,UploadMode=cdc,FlashSize=4M,PSRAM=enabled' firmware
arduino-cli upload  -b 'esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,UploadMode=cdc,FlashSize=4M,PSRAM=enabled' -p <port> firmware
```

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

1. Press BOOT — the display steps to the next key, blinking orange.
2. Keep pressing to step through A–Z, 0–9.
3. Stop pressing: 3 s later the key is applied and saved (no reboot).

The trigger keeps sending the previous key until the new one is applied. The USB product
name (`Matrix Remote [Q]`) updates at the next power-up. With `WEB_SERIAL_ENABLED=1` the
browser app can also set the key (`KEY:x`, then the device reboots).

## Matrix status guide

| Key colour | Meaning |
|---|---|
| Yellow, fast blink | Waiting for USB enumeration (first 4 s after boot) |
| Green, solid | Computer connected: HID + primary hub |
| Yellow, solid | Computer was connected but is asleep or unplugged — waiting for it |
| Green, blinking | Power only, no primary found yet |
| Purple, solid | Power only, relaying through a primary |
| Blue, solid | Web Serial connected (`WEB_SERIAL_ENABLED=1`) |
| White (trigger held) | Keystroke delivered |
| Red (trigger held) | Not delivered (power only with no primary, or computer asleep/unplugged) |
| Orange, blinking | Choosing a key (BOOT presses) |

WiFi-mode colours are listed in the sketch header.

## Web app (optional)

Only useful with `WEB_SERIAL_ENABLED=1`:

```bash
python3 website/serve.py          # opens Chrome at http://127.0.0.1:8080
```
