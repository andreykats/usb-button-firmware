# Hardware — pin layout and parts

Wiring guide for building a USB Button: a Waveshare ESP32-S3-Zero plus an
external trigger button and a transistor-driven status LED.

## Pins used

| Function | GPIO | Header pad | Notes |
|---|---|---|---|
| Trigger button | **GPIO13** | right side, "13" | Button to GND, internal pull-up (`TRIGGER_BTN_PIN`) |
| Status LED drive | **GPIO1** | left side, "1" | PWM to the transistor base/gate (`STATUS_LED_PIN`) |
| 5 V (LED supply) | — | left side, "5V" | USB VBUS directly — no diode, ≈ 5.0 V |
| GND | — | left side, "GND" | Button and transistor emitter/source |
| Key select | GPIO0 | — | Onboard **BOOT** button, nothing to wire |
| RGB status LED | GPIO21 | — | Onboard WS2812B, RGB colour order (`STATUS_RGB_PIN`) |

The LED stage uses the top pads of the left header (5V, GND, GPIO1). The trigger is on
pad 13, the third pad of the right header, directly opposite 3V3.

## Header layout (from the schematic and Waveshare pinout)

Top view, USB-C at the top:

| Left side | | Right side |
|---|---|---|
| 5V | | TX (GPIO43, via 499 Ω) |
| GND | | RX (GPIO44) |
| 3V3 (out) | | **13 — trigger** |
| **1 — status LED** | | 12 |
| 2 | | 11 |
| 3 | | 10 |
| 4 | | 9 |
| 5 | | 8 |
| 6 | | 7 |

Further pads: 14, 15, 16 at the lower right edge; 17, 18, 38, 39, 40, 41, 42, 45 on the
underside. See [resources/ESP32-S3-Zero-Pinout.png](resources/ESP32-S3-Zero-Pinout.png) and
[resources/ESP32-S3-Zero-Sch.pdf](resources/ESP32-S3-Zero-Sch.pdf).
**Check them against the silkscreen labels on the board before soldering.**

To use other pins, change `TRIGGER_BTN_PIN` / `STATUS_LED_PIN` at the top of the sketch. Avoid:
- **GPIO3 and GPIO45**: strapping pins, which affect boot.
- **TX/RX**: UART0, which carries boot messages.
- **GPIO21**: the onboard RGB LED.
- Any pin listed in *Pins used* above.

GPIO4–GPIO18 and GPIO38–GPIO42 are free. GPIO33–37 (octal PSRAM) and GPIO19/20 (USB) are
not broken out.

## Trigger button

```
"13" (GPIO13) ──── button ──── GND
```

- **Part:** any normally-open momentary switch, e.g. a 6 / 12 mm tactile switch, a
  panel-mount pushbutton, or a 24 / 30 mm arcade button.
- **Resistors:** none needed. The firmware enables the internal pull-up and debounces for 50 ms.
- **Long leads (over about 30 cm):** add a 10 kΩ pull-up from GPIO13 to 3V3 for better noise immunity.

## Status LED

The GPIO can't drive a 5 V LED directly, so a transistor switches the LED's ground side
(a low-side switch). GPIO HIGH turns the LED on.

```
5V ── 68 Ω ── LED (anode → cathode) ── C
                                        NPN 2N2222
"1" (GPIO1) ── 1 kΩ ──┬─────────────── B
                      │
                    10 kΩ
                      │
GND ──────────────────┴─────────────── E
```

### Parts

| Part | Chosen part | Notes |
|---|---|---|
| Transistor | **2N2222** NPN, TO-92 (60 V, 0.8 A, 0.5 W) | Far more headroom than one LED needs. 2N3904 or BC337 are drop-in alternatives. |
| LED | **White 5 mm water-clear**, Vf 3.0–3.4 V, 60 000 mcd, 30°, 320 mW max | See current notes below. |
| LED resistor | **68 Ω**, ¼ W (≈ 25 mA) | **Required.** The Matrix build used 47 Ω; see below. |
| Base resistor | **1 kΩ** | About 2.6 mA base current. Saturates the transistor up to about 30 mA. |
| Base pull-down | **10 kΩ**, base to GND | Keeps the LED off while GPIO1 floats during boot and flashing. |

**Why 68 Ω, not 47 Ω.** On the ESP32-S3-Matrix the 5 V pin sits behind a Schottky diode
(≈ 4.7 V). On the Zero, the 5V pad is USB VBUS directly: ≈ 5.0 V, and up to 5.25 V on some
ports. With 47 Ω that gives about 32–45 mA, past the 20–30 mA a 5 mm LED is happiest at and
past what the 1 kΩ base resistor fully saturates. 68 Ω gives about 22–31 mA.

**Why the LED resistor is required.** Without it, nothing sets the current except the
transistor's gain and the USB supply. The LED would pull hundreds of mA, burn out, and heat
the transistor.

**Brightness varies between units.** There is under 2 V of headroom between the supply and
the LED, so the current depends on the LED's exact Vf and the USB supply. That's normal for
white LEDs on 5 V.

**Transistor pinout.** Generic "2N2222" TO-92 parts come in two pinouts:
- PN2222A: E-B-C.
- P2N2222A: C-B-E.

Both are read with the flat face toward you and the legs down. Check your parts with the seller's
datasheet or a multimeter's diode test before soldering. Base-to-emitter and base-to-collector both read
about 0.6–0.7 V.

Other LEDs: `R_led = (5.0 V − LED Vf − 0.1 V) / current`.

### MOSFET alternative

A logic-level N-channel MOSFET is wired the same way:
- **Parts:** AO3400A or IRLML2502 (SOT-23).
- **Gate:** 100 Ω from GPIO1.
- **Gate pull-down:** 100 kΩ gate to GND.
- **Source:** to GND.
- **Drain:** to the LED cathode.

Avoid the **2N7000**: it isn't fully on at 3.3 V gate drive.

If you build a high-side stage instead (PNP or P-MOSFET, LED on when the GPIO is LOW), set `STATUS_LED_INVERT 1` in the sketch.

### LED behaviour

- **Idle:** continuous breathing, one fade in and out every 3 s, no dark pause.
- **While the trigger or BOOT is held:** blinks fast (10 Hz); back to breathing on release.
- **Tuning:** adjust the `LED_*` constants at the top of the sketch.

## Onboard RGB LED

One WS2812B on GPIO21, powered from 3V3. It takes colours in **RGB** order, not the usual
WS2812B GRB. If red and green look swapped, change `STATUS_RGB_ORDER` in the sketch.
Brightness is set by `STATUS_RGB_BRIGHTNESS`.

## Power budget

The board runs from USB 5 V. Onboard, an 800 mA LDO (ME6217) makes 3.3 V.

| Load | Typical draw |
|---|---|
| ESP32-S3 with radio on | 100–200 mA |
| Onboard RGB LED (brightness 40) | a few mA |
| Status LED | 22–31 mA while lit (less on average, because it pulses) |

The total stays well within a 500 mA USB port or charger.

## Reminders

- Don't hold BOOT while plugging in or pressing RESET. That puts the board into firmware-download mode.
- RESET is the only other onboard button. It restarts the board and can't be read by the firmware.
