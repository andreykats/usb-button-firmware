# TODO

Deferred items from the long-uptime audit (2026-10-26). All in `firmware/firmware.ino`,
ESP-NOW relay mode (`WIFI_ENABLED=0`). Ordered by how likely each one is to show up.

## Plausible, depending on how remotes are used

- [ ] **Remote's first press after its reboot can be dropped.** The primary's dedup in
  `handleRelay()` rejects any keystroke whose seq equals the last one from that MAC,
  with no time limit. `keystrokeSeq` restarts at every boot, so a remote that sent
  exactly one press, rebooted and pressed again loses that press (primary stayed up).
  Fix: count a match as a duplicate only within `DEDUP_WINDOW_MS` (1 s) of
  `remotePeers[idx].lastMs` (already stored, never read), and skip the check for a
  peer just added. **Needs the retry cap below first**, or retries arriving >1 s
  later would be typed twice.

## Remote, low impact

- [ ] **Keystroke retries never stop while the primary is unreachable.** Meant to be
  one retry, but the retry's own failed send status schedules another, every 2–8 ms,
  until the primary times out (15 s). Primary dedup hides it today; it's radio noise.
  Fix: a `retryUsed` flag set when the retry is scheduled and cleared in
  `sendKeystroke()`; drain `sendStatusQueue` (`xQueueReset`) before each new send so
  a stale status isn't blamed on it.
- [ ] **Peer table never evicts.** `addRemotePeer()` returns -1 once 50 remotes have
  been seen in one uptime; later remotes get no dedup (a retry may type twice).
  Fix: when full, reuse the entry with the oldest `lastMs`.

## Practically never

- [ ] **Two deadlines aren't wrap-safe.** `now >= announceBurstNextMs` and
  `now >= retryAfterMs` in `handleRelay()`; a deadline set in the last few ms before
  `millis()` wraps (49.7 days) waits another 49.7 days. Use
  `(long)(now - deadline) >= 0`.
- [ ] **A relayed key in progress is typed when the host wakes.** If the host sleeps
  within ~10 ms of a relayed tap, `relayKey` survives and is typed on resume. Clear
  `relayKey` with `relayTapCount` in `handleHostEnumeration()` when the host goes
  away (a key already down still gets its release via `hidSentKey`).
- [ ] **ESP-NOW queues aren't checked after `xQueueCreate()`.** A NULL queue would
  crash in the callbacks. Move creation into `initEspNow()`, go to `STATE_ERROR` on
  failure, and return early from `handleRelay()` when `!espNowInitialized`.
- [ ] **Callbacks use the `FromISR` queue calls.** ESP-NOW callbacks run in the WiFi
  task, not an ISR: use `xQueueSend(q, &item, 0)` / `xQueueOverwrite()`. No
  behaviour change.

## Design choices to revisit

- [ ] ESP-NOW init failure stays red (`STATE_ERROR`) until a reboot — could restart
  itself instead.
- [ ] Trigger has no debounce beyond the 10 ms poll: a noise spike on a long trigger
  wire could type a key. Two matching reads would fix it at +10 ms latency.
- [ ] Status LED pattern phase jumps once per 49.7 days (`now - ledPhaseStartMs`
  wraps). Cosmetic.
- [ ] `WIFI_ENABLED=1` build not audited (e.g. `String`s written from the ESP-NOW
  callback in the WiFi task while `loop()` reads them).
