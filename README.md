# USB Button — Web Serial interface

The settings page for USB Button (Advanced Type) boards, live at
**[button.drey.dev](https://button.drey.dev)**. It connects to a button over USB from the
browser, with no app or driver to install, and changes how its trigger and lights
behave.

The button's firmware is on the `ws-esp32s3-zero` (ESP32-S3-Zero) and `ws-esp32s3-matrix`
(ESP32-S3-Matrix) branches.

## Compatible browsers

The page uses the [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API),
which only some browsers have. Support as listed by [caniuse](https://caniuse.com/web-serial)
(checked 2026-10-26):

| Browser | Works | Notes |
|---|---|---|
| Google Chrome (desktop) | ✅ version 89 and later | Tested: Chrome 155 on macOS |
| Microsoft Edge (desktop) | ✅ version 89 and later | |
| Opera (desktop) | ✅ version 76 and later | |
| Firefox (desktop) | ✅ version 151 and later | Not tested with a button yet |
| Safari (Mac, iPhone, iPad) | ❌ | Apple doesn't support Web Serial |
| Chrome for Android | ❓ | Listed from version 156; not tested with a USB button |
| Firefox for Android, Samsung Internet | ❌ | |

In a browser without Web Serial the page says so instead of showing the settings.

## Using it

1. Plug the button into the computer.
2. Hold **LEFT BUTTON** for 3 seconds. The button restarts and its status light cycles through
   all colours (settings mode).
3. Open [button.drey.dev](https://button.drey.dev), click **Connect** and choose the
   **USB Button** entry, e.g. `USB Button [2] (EB:F4:58)`.
4. Change settings; each is saved on the button straight away.
5. Click **Disconnect**, or press LEFT BUTTON or RESET, to go back to normal use.

The key can also be changed without the page: in normal use, press and release **LEFT BUTTON**.
The button switches to the next key (a to z, then 0 to 9), types it and flashes white.
The page shows this too, next to the steps for connecting.

## Files

| File | What it is |
|---|---|
| `index.html` | The settings page (published) |
| `debug.html` | Debugger for firmware built with `WEB_SERIAL_ENABLED=1` (local use only) |
| `serve.py` | Local server: `python3 serve.py` opens Chrome at http://127.0.0.1:8080 |
| `.github/workflows/pages.yml` | Publishes `index.html` to GitHub Pages on every push that changes it |
