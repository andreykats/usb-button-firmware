# USB Button

A one-key USB keyboard you can build from a small microcontroller board, a push button
and an LED. Plug it into a computer, press the trigger button, and the computer receives
the selected key as if it were typed on a keyboard. It is a standard USB HID keyboard, so
it works on macOS, Windows, Linux, iOS/iPadOS and Android with no drivers or software.

Each supported board has its own firmware, kept in its own branch. Pick the branch for
your board: its README explains how to use the button, and its HARDWARE.md shows the
wiring.

## Firmware branches

| Branch | Board | Type |
|---|---|---|
| [`qtpy-samd21`](https://github.com/andreykats/usb-button/tree/qtpy-samd21) | [Adafruit QT Py SAMD21](https://learn.adafruit.com/adafruit-qt-py) | Basic |
| [`ws-esp32s3-zero`](https://github.com/andreykats/usb-button/tree/ws-esp32s3-zero) | [Waveshare ESP32-S3-Zero](https://docs.waveshare.com/ESP32-S3-Zero) | Advanced |
| [`ws-esp32s3-matrix`](https://github.com/andreykats/usb-button/tree/ws-esp32s3-matrix) | [Waveshare ESP32-S3-Matrix](https://docs.waveshare.com/ESP32-S3-Matrix) | Advanced |

**Basic** buttons only work plugged into a computer. **Advanced** buttons can also work
wirelessly: plugged into a charger, they send their key to a nearby button that is
plugged into a computer, and that button types it.

## Feature comparison

| Feature | QT Py SAMD21 | ESP32-S3-Zero | ESP32-S3-Matrix |
|---|---|---|---|
| Type | Basic | Advanced | Advanced |
| Mesh network | ❌ | ✅ | ✅ |
| Selectable keys | 36: a–z, 0–9 | 36: a–z, 0–9 | 36: a–z, 0–9 |
| Key selector | SELECT button | BOOT button or web browser | BOOT button or web browser |
| Customizable via web browser | ❌ | ✅ | ❌ |
| Shows the selected key | ❌ | ❌ | ✅ on the 8×8 LED display |
| Status light | RGB LED | RGB LED | 8×8 LED display |

All three firmware versions behave the same way, apart from the Zero's and Matrix's Mesh network features, the Zero's settings in the browser and the Matrix's display.

## Hub power

> ⚠️ **A powered USB hub is required** to connect buttons through a hub. Allow **2.5 W per
> button port**: for example, a 10-port hub needs a power supply of at least **25 W**.
