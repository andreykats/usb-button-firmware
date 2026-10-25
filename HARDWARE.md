# Hardware — pin layout and parts

Waveshare ESP32-S3-Zero with an external trigger button and a transistor-driven status LED.

## Pins used

Top view, USB-C at the top.

| Function | GPIO | Header pad | Notes |
|---|---|---|---|
| Trigger button | GPIO13 | right side, 3rd pad ("13") | Button to GND, internal pull-up (`TRIGGER_BTN_PIN`) |
| Status LED drive | GPIO1 | left side, 4th pad ("1") | PWM to the 2N2222 base (`STATUS_LED_PIN`) |
| 5 V | — | left side, 1st pad ("5V") | USB VBUS directly, ≈ 5.0 V |
| GND | — | left side, 2nd pad ("GND") | Button and transistor emitter |
| Key select | GPIO0 | — | Onboard BOOT button |
| RGB status LED | GPIO21 | — | Onboard WS2812B, RGB colour order (`STATUS_RGB_PIN`) |

Don't hold BOOT while plugging in or pressing RESET: that enters firmware-download mode.

## Trigger button

```
"13" ──── trigger button ──── GND
```

Any normally-open momentary switch. No resistors: the firmware enables the internal pull-up.

## Status LED

The transistor is an emitter follower feeding the LED from 5V. GPIO HIGH turns the LED on.
The LED gets about 2.6 V (the GPIO's 3.3 V minus the transistor's 0.7 V), which also limits
its current without resistors.

```
                "5V"
                 │
         ┌───────┴───────┐
         │   Collector   │
"1" ─────┤ Base          │  2N2222 (NPN)
         │   Emitter     │
         └───────┬───────┘
                 │
         ┌───────┴───────┐
         │ LED + anode   │  long leg
         │ LED − cathode │  short leg
         └───────┬───────┘
                 │
               "GND"
```

| Part | Value |
|---|---|
| Transistor | 2N2222 NPN, TO-92 |
| LED | CHANZON 8 mm white (straw hat), Vf 3.0–3.2 V, 250 mA |

**Transistor pinout.** Generic "2N2222" TO-92 parts come in two pinouts, read with the flat
face toward you and the legs down: PN2222A is E-B-C, P2N2222A is C-B-E. Check yours before
soldering.
