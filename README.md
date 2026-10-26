# USB Button (Advanced Type)

A one-key USB keyboard. Plug it into a computer, press the trigger button, and the
computer receives the selected key as if it were typed on a keyboard.

It is a standard USB HID keyboard, so it works on macOS, Windows, Linux, iOS/iPadOS and
Android with no drivers or software. Phones and tablets need a USB-C port or a USB
adapter.

Holding the trigger works like holding a key on a keyboard: the computer starts
repeating the key after a short delay. You can change this in the [settings](#settings).

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
0, 1 … 9, then back to a. Each press types the new key once, straight away, so you can
see which one is selected in any text field. The button remembers its key when unplugged.

You can also pick the key directly on the [settings page](#settings).

Don't hold BOOT while plugging the button in: that puts the board into firmware-update
mode. Unplug it and plug it back in to recover.

## Settings

You can change how the trigger and its light behave from the settings page,
[button.drey.dev](https://button.drey.dev), in Chrome, Edge, Opera or Firefox on a
computer. Safari doesn't support it.

1. Plug the button into the computer.
2. Hold **BOOT** for 3 seconds. The button restarts and its status light cycles through
   all colours. The selected key doesn't change.
3. Open [button.drey.dev](https://button.drey.dev), click **Connect** and choose the
   **USB Button** entry. It shows the button's key and an ID that tells buttons apart,
   e.g. `USB Button [2] (EB:F4:58)`.

| Setting | Choices (default first) |
|---|---|
| Key | a–z, 0–9 (default `1`; **Restore defaults** keeps the key) |
| Trigger types the key | When pressed, when released |
| Trigger held down | Repeats the key, types it once (a key typed on release is always typed once) |
| Trigger light while a button is pressed | Fast blink, on, slow blink, off |
| Trigger light when idle | Breathing, fast blink, slow blink, off |
| Status light brightness | Normal, high, low |

Each change is saved on the button straight away. While the button is in settings mode
it doesn't type: pressing the trigger shows its light, and the settings page shows what
your computer would receive.

To go back to normal use, click **Disconnect**, press **BOOT** or **RESET**, or unplug
the button and plug it back in.

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
| 🌈 Cycling through all colours | Settings mode |

If the trigger is set to type when released, white or red shows briefly after you let go.

**Trigger light**

| Light | Meaning |
|---|---|
| ⚪ Slow breathing | Ready |
| ⚪ Fast blinking | A button is being held |

These are the defaults; both can be changed in the [settings](#settings).
