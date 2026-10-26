# USB Button Web Serial interface — CLAUDE.md

Branch `web-serial-interface`: the browser side of USB Button settings mode. The firmware
is on `ws-esp32s3-zero` (Waveshare ESP32-S3-Zero) and `ws-esp32s3-matrix` (Waveshare
ESP32-S3-Matrix, the same sketch with its own hardware layer), "Advanced" units only;
the Zero's sketch header is the source of truth for the settings protocol. README.md is for people using
or working on the page (compatible browsers, how to connect).

## Layout

```
index.html    ← settings page: single static file, inline CSS/JS, no build step (published)
debug.html    ← debugger for the firmware's normal-mode WEB_SERIAL_ENABLED=1 protocol (local only)
serve.py      ← localhost server; python3 -I serve.py opens Chrome at http://127.0.0.1:8080
.github/workflows/pages.yml ← publishes index.html to GitHub Pages
```

## Deploy

- Every push to `web-serial-interface` that changes `index.html` or the workflow publishes
  `index.html` (only that file) to https://button.drey.dev via GitHub Pages (Actions).
- After every change to `index.html`: commit and push `web-serial-interface` without
  asking (standing permission from the user), then confirm the run succeeded
  (`gh run list -R andreykats/usb-button-firmware -b web-serial-interface`) and that
  `curl -s https://button.drey.dev/ | shasum` matches the local file.
- The custom domain is set in the repo's Pages settings (no CNAME file with Actions
  deploys); DNS: CNAME `button.drey.dev` → `andreykats.github.io`; HTTPS enforced. Only
  `main` and `web-serial-interface` may deploy (github-pages environment rule).
- Web Serial needs a secure context: HTTPS, or localhost via serve.py.

## Connection

- Web Serial: `navigator.serial.requestPort()` filtered to the Advanced boards: Zero
  303A:822B and Matrix 303A:81FB (each the same in normal and settings mode), opened at
  115200. Never open at 1200 baud: that resets the
  board into its bootloader.
- `port.getInfo()` exposes only VID/PID; anything else the page shows (key, settings)
  must come from the firmware over the protocol. Chrome's chooser shows the USB product
  name, e.g. `USB Button [2] (EB:F4:58)`, but the page cannot read it.
- Previously granted ports connect automatically (`getPorts()` and the `connect` event)
  in quiet mode: a button in normal use doesn't answer `GET`, which is not an error.

## Settings protocol (newline-terminated text)

Page → board: `GET`, `SET:name=value`, `DEFAULTS`, `REBOOT` (restart into normal use;
the page's **Disconnect** button).
`SET:key=x` changes the key (a–z, 0–9; the "Key" panel, built from `KEY_ROWS`).
`DEFAULTS` leaves the key alone. Firmware older than this answers it with
`ERR:UNKNOWN_SETTING:key`, which the page shows as "update the button's firmware".
Board → page: `SETTINGS:name=value,…,key=x` (GET, DEFAULTS),
`OK:name=value`, `ERR:…`, `REBOOTING...`, and for the trigger test `TRIGGER:down|up`
and `TYPE:hold|tap`.

## Adding a setting

The firmware branch's CLAUDE.md sends agents here whenever a customizable feature is
added. For each new firmware setting (an entry in `SETTINGS[]` on `ws-esp32s3-zero`):

1. Add a `<fieldset data-setting="NAME">` with one radio per choice: `name="NAME"`,
   `value` = the firmware's choice name. Order the tiles as they read best (brightness
   is High / Normal / Low); the default is whatever the firmware lists first.
2. Add `NAME` to `SETTING_NAMES` in the script.
3. Settings an older firmware doesn't report are hidden (`showReportedSettings()`), so the
   page can be published before every board is reflashed.
4. Render it (headless Chrome screenshot), commit, push, and check the deploy (above).
5. Update README.md here if the change affects how the page is used.

## Conventions

- Copy is user-facing: plain verbs, sentence case; name controls by what they do.
- Lamp previews use the firmware's timings (breathe 3 s, fast 100 ms, slow 1 s); keep
  them in sync with the `LED_*_PERIOD_MS` constants.
- Respect `prefers-reduced-motion`; keep keyboard focus visible.
