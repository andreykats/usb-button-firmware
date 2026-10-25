# Hardware — pin layout and parts

Waveshare ESP32-S3-Matrix with an external trigger button and a transistor-driven status LED.

## Pins used

| Function | GPIO | Header pin (P1) | Notes |
|---|---|---|---|
| Trigger button | GPIO1 | 10 | Button to GND, internal pull-up (`TRIGGER_BTN_PIN`) |
| Status LED drive | GPIO2 | 9 | PWM to the 2N2222 base (`STATUS_LED_PIN`) |
| 5 V | — | 1 | USB 5 V after the onboard Schottky diode, ≈ 4.7 V |
| GND | — | 2 | Button and transistor emitter |
| Key select | GPIO0 | — | Onboard BOOT button |
| RGB matrix | GPIO14 | — | Onboard 8×8 WS2812B (`MATRIX_PIN`) |

The P1 numbers come from the schematic ([resources/ESP32-S3-Matrix-Sch.pdf](resources/ESP32-S3-Matrix-Sch.pdf)).
Check them against the silkscreen labels on the board before soldering.

Don't hold BOOT while plugging in or pressing RESET: that enters firmware-download mode.

## Trigger button

```
P1-10 (GPIO1) ──── trigger button ──── P1-2 (GND)
```

Any normally-open momentary switch. No resistors: the firmware enables the internal pull-up.

## Status LED

The transistor is an emitter follower feeding the LED from 5V. GPIO HIGH turns the LED on.
The LED gets about 2.6 V (the GPIO's 3.3 V minus the transistor's 0.7 V), which also limits
its current without resistors.

```
              P1-1 (5V)
                 │
         ┌───────┴───────┐
         │   Collector   │
P1-9 ────┤ Base          │  2N2222 (NPN)
(GPIO2)  │   Emitter     │
         └───────┬───────┘
                 │
         ┌───────┴───────┐
         │ LED + anode   │  long leg
         │ LED − cathode │  short leg
         └───────┬───────┘
                 │
              P1-2 (GND)
```

| Part | Value |
|---|---|
| Transistor | 2N2222 NPN, TO-92 |
| LED | CHANZON 8 mm white (straw hat), Vf 3.0–3.2 V, 250 mA |

**Transistor pinout.** Generic "2N2222" TO-92 parts come in two pinouts, read with the flat
face toward you and the legs down: PN2222A is E-B-C, P2N2222A is C-B-E. Check yours before
soldering.
