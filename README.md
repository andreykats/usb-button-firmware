# USB Button (Advanced Type)

A one-key USB keyboard. Plug it into a computer, press the trigger button, and the
computer receives the selected key as if it were typed on a keyboard.

It is a standard USB HID keyboard, so it works on macOS, Windows, Linux, iOS/iPadOS and
Android with no drivers or software. Phones and tablets need a USB-C port or a USB
adapter.

Holding the trigger works like holding a key on a keyboard: the computer starts
repeating the key after a short delay.

## Wireless buttons

A button doesn't have to be plugged into a computer. Plugged into a charger, it sends
its key wirelessly to a nearby button that *is* plugged into a computer, and that button
types it. Several wireless buttons can share one connected button.

- The connected button's own presses always go first.
- A wireless press is typed as a single key press: holding the trigger does not repeat.
- A button that was plugged into a computer stays a computer button until it is
  unplugged, even if the computer goes to sleep.

## Choosing the key

There are 36 keys to choose from: the letters a–z (typed in lower case) and the digits
0–9. The default key is `1`.

Press the small **BOOT** button on the board to step to the next key: a, b, c … z, then
0, 1 … 9, then back to a. Each press types the new key straight away, so you can see
which one is selected in any text field. The button remembers its key when unplugged.

Don't hold BOOT while plugging the button in: that puts the board into firmware-update
mode. Unplug it and plug it back in to recover.

## Hardware requirements

> ⚠️ **A powered USB hub is required.** To connect buttons through a hub, use one with
> its own power supply. Unpowered (bus-powered) hubs are not supported.

Allow **2.5 W per button port** (5 V, 500 mA). The hub's power supply must cover every
port in use: for example, a 10-port hub with a button on each port needs a power supply
of at least **25 W**.

A wireless button can use any USB charger rated 5 V, 500 mA or more.

## Lights

**Status light (on the board)**

| Light | Meaning |
|---|---|
| 🟡 Yellow, fast blink | Starting up (a few seconds after plugging in) |
| 🟢 Green, solid | Connected to a computer and ready; also types keys from wireless buttons |
| 🟢 Green, blinking | The computer is asleep or unplugged: waiting for it to come back |
| 🔵 Blue, solid | Wireless: linked to a connected button and ready |
| 🔵 Blue, blinking | Wireless: looking for a connected button |
| 🔴 Red, blinking | Fault: unplug and plug back in |
| ⚪ White while a button is held | The key press was delivered |
| 🔴 Red while a button is held | The key press could not be delivered |

**Trigger light**

| Light | Meaning |
|---|---|
| ⚪ Slow breathing | Ready |
| ⚪ Fast blinking | A button is being held |
