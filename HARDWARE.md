# Hardware — pin layout and parts

Wiring guide for building an ESP32-S3 Matrix Button: a Waveshare ESP32-S3-Matrix plus an
external trigger button and a transistor-driven status LED.

## Pins used

| Function | GPIO | Header pin (P1) | Notes |
|---|---|---|---|
| Trigger button | **GPIO1** | 10 | Button to GND, internal pull-up (`TRIGGER_BTN_PIN`) |
| Status LED drive | **GPIO2** | 9 | PWM to the transistor base/gate (`STATUS_LED_PIN`) |
| 5 V (LED supply) | — | 1 | USB 5 V after the onboard Schottky diode, ≈ 4.7 V |
| GND | — | 2 | Button and transistor emitter/source |
| Key select | GPIO0 | — | Onboard **BOOT** button, nothing to wire |
| RGB matrix | GPIO14 | — | Onboard 8×8 WS2812B |
| IMU (QMI8658) | 11 / 12 / 10 / 13 | — | Onboard SDA / SCL / INT1 / INT2 (unused) |

GPIO1 and GPIO2 sit next to each other on the header, near 5 V and GND.

## Header P1 (from the schematic)

| P1 | Signal | | P1 | Signal |
|---|---|---|---|---|
| 1 | 5V | | 11 | RX (U0RXD) |
| 2 | GND | | 12 | TX (U0TXD, via 499 Ω) |
| 3 | 3V3 | | 13 | IO40 |
| 4 | IO7 | | 14 | IO39 |
| 5 | IO6 | | 15 | IO38 |
| 6 | IO5 | | 16 | IO37 |
| 7 | IO4 | | 17 | IO36 |
| 8 | IO3 | | 18 | IO35 |
| 9 | **IO2 — status LED** | | 19 | IO34 |
| 10 | **IO1 — trigger** | | 20 | IO33 |

These P1 numbers come from the schematic's netlist ([resources/ESP32-S3-Matrix-Sch.pdf](resources/ESP32-S3-Matrix-Sch.pdf)).
**Check them against the silkscreen labels on the board before soldering.**

To use other pins, change `TRIGGER_BTN_PIN` / `STATUS_LED_PIN` at the top of the sketch. Avoid:
- **IO3**: a strapping pin, which affects boot.
- **RX/TX**: UART0, which carries boot messages.
- Any pin listed in *Pins used* above.

IO4–IO7 and IO33–IO40 are free. The board's quad PSRAM doesn't use IO33–IO37.

## Trigger button

```
P1-10 (GPIO1) ──── button ──── P1-2 (GND)
```

- **Part:** any normally-open momentary switch, e.g. a 6 / 12 mm tactile switch, a
  panel-mount pushbutton, or a 24 / 30 mm arcade button.
- **Resistors:** none needed. The firmware enables the internal pull-up and debounces for 50 ms.
- **Long leads (over about 30 cm):** add a 10 kΩ pull-up from GPIO1 to 3V3 (P1-3) for better noise immunity.

## Status LED

The GPIO can't drive a 5 V LED directly, so a transistor switches the LED's ground side
(a low-side switch). GPIO HIGH turns the LED on.

```
P1-1 (5V) ── 47 Ω ── LED (anode → cathode) ── C
                                                NPN 2N2222
P1-9 (GPIO2) ── 1 kΩ ──┬────────────────────── B
                       │
                     10 kΩ
                       │
P1-2 (GND) ────────────┴────────────────────── E
```

### Parts

| Part | Chosen part | Notes |
|---|---|---|
| Transistor | **2N2222** NPN, TO-92 (60 V, 0.8 A, 0.5 W) | Far more headroom than one LED needs. 2N3904 or BC337 are drop-in alternatives. |
| LED | **White 5 mm water-clear**, Vf 3.0–3.4 V, 60 000 mcd, 30°, 320 mW max | See current notes below. |
| LED resistor | **47 Ω**, ¼ W (≈ 30 mA) | **Required.** Use 68 Ω (≈ 20 mA) if 47 Ω is brighter than needed. |
| Base resistor | **1 kΩ** | About 2.6 mA base current. Saturates the transistor up to about 30 mA. |
| Base pull-down | **10 kΩ**, base to GND | Keeps the LED off while GPIO2 floats during boot and flashing. |

**Why the LED resistor is required.** The 5 V pin gives about 4.7 V, but the LED only drops 3.0–3.4 V.
Without a resistor, nothing sets the current except the transistor's gain and the USB supply. The LED would
pull hundreds of mA, burn out, and heat the transistor. It could also sag the 5 V rail that also feeds the
matrix.

**LED current.** The listing's 120 mA can't be continuous: at 3.4 V that is 408 mW, more than the
LED's own 320 mW rating. A 5 mm epoxy LED is happiest at 20–30 mA. 30 mA (about 100 mW) is already
very bright at 60 000 mcd. Don't go above about 60 mA (27 Ω resistor, 470 Ω base resistor).

**Brightness varies between units.** There is only about 1.3 V of headroom between the supply and the
LED. With 47 Ω, the current lands anywhere from about 20 to 38 mA, depending on the LED's exact Vf and the
USB supply. That's normal for white LEDs on 5 V.

**Transistor pinout.** Generic "2N2222" TO-92 parts come in two pinouts:
- PN2222A: E-B-C.
- P2N2222A: C-B-E.

Both are read with the flat face toward you and the legs down. Check your parts with the seller's
datasheet or a multimeter's diode test before soldering. Base-to-emitter and base-to-collector both read
about 0.6–0.7 V.

Other LEDs: `R_led = (4.7 V − LED Vf − 0.1 V) / current`.

### MOSFET alternative

A logic-level N-channel MOSFET is wired the same way:
- **Parts:** AO3400A or IRLML2502 (SOT-23).
- **Gate:** 100 Ω from GPIO2.
- **Gate pull-down:** 100 kΩ gate to GND.
- **Source:** to GND.
- **Drain:** to the LED cathode.

Avoid the **2N7000**: it isn't fully on at 3.3 V gate drive.

If you build a high-side stage instead (PNP or P-MOSFET, LED on when the GPIO is LOW), set `STATUS_LED_INVERT 1` in the sketch.

### LED behaviour

- **Idle:** soft fade in and out every 3 s.
- **After a trigger press:** blinks fast (5 Hz) for 3 s.
- **Tuning:** adjust the `LED_*` constants at the top of the sketch.

## Power budget

The board runs from USB 5 V. Onboard, a Schottky diode feeds the 5 V rail and an 800 mA LDO makes 3.3 V.

| Load | Typical draw |
|---|---|
| ESP32-S3 with radio on | 100–200 mA |
| Matrix showing one character (brightness 40) | under 100 mA |
| Status LED | 20–38 mA while lit (less on average, because it pulses) |

The total stays well within a 500 mA USB port or charger. Keep `MATRIX_BRIGHTNESS` low:
Waveshare warns that high brightness overheats the board.

## Reminders

- Don't hold BOOT while plugging in or pressing RESET. That puts the board into firmware-download mode.
- RESET is the only other onboard button. It restarts the board and can't be read by the firmware.
