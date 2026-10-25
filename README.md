# USB Button

A one-key USB keyboard. Plug it into a computer, press the trigger button, and the
computer receives the selected key as if it were typed on a keyboard.

It is a standard USB HID keyboard, so it works on macOS, Windows, Linux, iOS/iPadOS and
Android with no drivers or software. Phones and tablets need a USB-C port or a USB
adapter.

Holding the trigger works like holding a key on a keyboard: the computer starts
repeating the key after a short delay.

## Choosing the key

There are 36 keys to choose from: the letters a–z (typed in lower case) and the digits
0–9. The default key is `1`.

Press the select button to step to the next key: a, b, c … z, then 0, 1 … 9, then back
to a. Each press types the new key straight away, so you can see which one is selected
in any text field. The button remembers its key when unplugged.

## Lights

**Status light (on the board)**

| Light | Meaning |
|---|---|
| 🟡 Yellow, fast blink | Starting up (a few seconds after plugging in) |
| 🟢 Green, solid | Connected to a computer and ready |
| 🟢 Green, blinking | No computer: plugged into a charger, or the computer is asleep or unplugged |
| 🔴 Red, blinking | Fault: unplug and plug back in |
| ⚪ White while a button is held | The key press reached the computer |
| 🔴 Red while a button is held | The key press could not be delivered |

**Trigger light**

| Light | Meaning |
|---|---|
| ⚪ Slow breathing | Ready |
| ⚪ Fast blinking | A button is being held |
