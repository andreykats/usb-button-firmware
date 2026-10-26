# Fleet — ESP32-S3-Zero boards

Which firmware each board runs. Boards are identified by USB serial number (the MAC),
which the fleet tool logs. Update this file after every flash; the raw per-attempt log is
`~/.local/usb-button-tools/flash-log-zero.csv` (outside the repo).

## Current firmware

| Board | Serial (MAC) | Firmware | Flashed | Saved key / settings |
|---|---|---|---|---|
| board-01 | a0:f2:62:eb:fc:d4 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-02 | a0:f2:62:ea:5c:dc | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-03 | a0:f2:62:eb:82:c0 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-04 | ac:27:6e:af:83:34 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-05 | ac:27:6e:af:8b:2c | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-06 | a0:f2:62:eb:7d:58 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-07 | d4:05:92:44:a3:c4 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-08 | ac:27:6e:af:f8:b0 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-09 | a0:f2:62:ea:ca:14 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-10 | a0:f2:62:ea:c5:c4 | `e680468` | 2026-10-26 | erased: key 1, defaults |
| board-11 | a0:f2:62:eb:f4:58 | `e680468` (IDE upload) | 2026-10-26 | kept: desk test board |

"Saved key / settings" is the state right after the flash: an erase resets the key to `1`
and every setting to its default; a plain upload keeps what was saved (NVS).

## Firmware images

| Firmware | What changed | Image sha256 (fleet tool) |
|---|---|---|
| `b6aebdf` | First fleet image: settings mode and settings page | `b637ae77cfc67359` |
| `e680468` | Press light no longer sticks after 24.8 days; loop watchdog; key settable from the settings page; BOOT steps the key on release | `44fec048c5891c2f` |

Image directories are `~/.local/usb-button-tools/zero-<commit>/`.

## History

| Date | Boards | Firmware | How | Notes |
|---|---|---|---|---|
| 2026-10-25 | 01–10 | `b6aebdf` | fleet tool, erase | board-01 failed once, verified after replug |
| 2026-10-25 | — | `b6aebdf` | fleet tool | Rejected: a board that powers up (red fault blink) but never enumerates, not even in ROM download mode |
| 2026-10-26 | 11 | `b6aebdf` | fleet tool, erase | |
| 2026-10-26 | 11 | development builds, then `e680468` | IDE upload (`arduino-cli`), no erase | Desk test board |
| 2026-10-26 | 01–10 | `e680468` | fleet tool, erase | All ok first try |
