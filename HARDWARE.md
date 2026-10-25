# Hardware — pin layout and parts

Adafruit QT Py SAMD21 with an external key select button, an external trigger button and a
transistor-driven status LED.

## Pins used

Top view, USB-C at the top.

| Function | Pin | Header pad | Notes |
|---|---|---|---|
| Key select button | A0 | left side, 1st pad ("A0") | Button to GND, internal pull-up (`SELECT_BTN_PIN`) |
| Trigger button | A3 | left side, 4th pad ("A3") | Button to GND, internal pull-up (`TRIGGER_BTN_PIN`) |
| Status LED drive | A10 | right side, 4th pad ("MO") | PWM to the 2N2222 base (`STATUS_LED_PIN`) |
| 5 V | — | right side, 1st pad ("5V") | USB 5 V behind a protection diode, ≈ 4.7 V |
| GND | — | right side, 2nd pad ("GND") | Buttons and transistor emitter |
| RGB status LED | D11 | — | Onboard NeoPixel (`STATUS_RGB_PIN`) |

## Buttons

```
"A0" ──── key select button ──── GND
"A3" ──── trigger button    ──── GND
```

Any normally-open momentary switch. No resistors: the firmware enables the internal pull-ups.

## Status LED

The transistor is an emitter follower feeding the LED from 5V. Pin HIGH turns the LED on.
The LED gets about 2.6 V (the pin's 3.3 V minus the transistor's 0.7 V), which also limits
its current without resistors.

```
"5V" ─── C
          NPN 2N2222
"MO" ─── B
          E ── LED (anode → cathode) ── "GND"
```

| Part | Value |
|---|---|
| Transistor | 2N2222 NPN, TO-92 |
| LED | CHANZON 8 mm white (straw hat), Vf 3.0–3.2 V, 250 mA |

**Transistor pinout.** Generic "2N2222" TO-92 parts come in two pinouts, read with the flat
face toward you and the legs down: PN2222A is E-B-C, P2N2222A is C-B-E. Check yours before
soldering.
