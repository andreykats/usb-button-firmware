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
P1-1 (5V) ── R_led ── LED (anode → cathode) ── C
                                                 NPN
P1-9 (GPIO2) ── 1 kΩ ──┬────────────────────── B
                       │
                     10 kΩ
                       │
P1-2 (GND) ────────────┴────────────────────── E
```

### Parts

| Part | Recommendation | Why |
|---|---|---|
| Transistor | **2N3904** or **BC337** (TO-92 NPN). 2N2222A also works. | Cheap, common, fully on with 3.3 V drive through 1 kΩ. BC337 handles up to 800 mA if you later drive more LEDs. |
| Base resistor | **1 kΩ** | About 2.6 mA base current. Plenty for a 15 mA LED. |
| Base pull-down | **10 kΩ**, base to GND | Keeps the LED off while GPIO2 floats during boot and flashing. |
| LED | 5 mm through-hole, any colour | |
| LED resistor | Red / yellow / orange: **180 Ω**. Green / blue / white: **120 Ω**. | Sized for about 4.7 V supply and about 13–15 mA. ¼ W is fine. |

Other LED currents: `R_led = (4.7 V − LED forward voltage) / current`.

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
| Status LED | about 15 mA |

The total stays well within a 500 mA USB port or charger. Keep `MATRIX_BRIGHTNESS` low:
Waveshare warns that high brightness overheats the board.

## Reminders

- Don't hold BOOT while plugging in or pressing RESET. That puts the board into firmware-download mode.
- RESET is the only other onboard button. It restarts the board and can't be read by the firmware.
