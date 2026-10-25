# USB Button

A one-key USB keyboard on an [Adafruit QT Py SAMD21](https://learn.adafruit.com/adafruit-qt-py).
Press the trigger button and the computer receives the selected key, as if it were typed
on a keyboard. No drivers or software are needed on the computer.

## How it works

| Button | Pin | What it does |
|---|---|---|
| Trigger | A3 | Types the selected key. The key goes down on press and up on release, so holding the button makes the computer repeat the key. |
| Select | A0 | Steps to the next key (a–z, then 0–9) and types it straight away so you can see it. |

- Letters type lower case. The default key is `1`.
- The selected key is saved to flash, so it survives unplugging.
- Both buttons are read every 10 ms.

## Status lights

**Onboard NeoPixel**

| Colour | Meaning |
|---|---|
| Yellow, fast blink | Starting up: waiting for a computer (up to 4 s) |
| Green, solid | Computer connected |
| Green, blinking | No computer: power only, or the computer is asleep or unplugged |
| Red, blinking | Error: saving the key to flash failed (reboot to clear) |
| White while held | The key press was delivered |
| Red while held | The key press could not be delivered |

**External LED (A10):** breathes slowly (one fade every 3 s) when idle, and blinks fast
while a button is held.

## Hardware

See [HARDWARE.md](HARDWARE.md) for the pins and wiring.

## Building

Open `firmware/firmware.ino` in the Arduino IDE and upload with:

- **Board:** Adafruit QT Py M0 (SAMD21), from the Adafruit SAMD boards package
- **USB Stack:** Arduino
- **Libraries:** Adafruit NeoPixel, FlashStorage, Keyboard

If the upload fails, double-tap the board's RST button. A `QTPY_BOOT` drive appears and
the upload can be retried.
