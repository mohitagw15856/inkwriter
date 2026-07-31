# InkWriter architecture

## Ecosystem study: what was reused vs rebuilt

Per the spec, existing community Bluetooth-keyboard work was surveyed first:

- **CrumBLE** (imshentastic/CrumBLE, a CrossInk fork, MIT) carries a complete
  BLE HID host in its freeink-sdk fork: `libs/network/BleKeyboardHost`,
  NimBLE-based, central role, bonded reconnect, key translation, and
  documented in its `docs/ble-keyboard-host.md`. This is exactly the
  hard-to-get-right layer (GATT HID subscription, bonding, auto-repeat,
  fixed-capacity queues).
- CrossPoint Reader itself has no BLE input support (its keyboard is an
  on-screen QWERTY).

**Reused**: `BleKeyboardHost` is vendored verbatim under
`lib/BleKeyboardHost/` (MIT, attribution in the files and THIRD_PARTY note
below) with its documented build flags, rather than writing a new untested
HID host. **Rebuilt**: everything editor-side (buffer, session stats,
screens) is InkWriter's own, portable and host-tested.

## The RAM trade-off: writing mode vs sync mode

The ESP32-C3 has ~380 KB of usable RAM and no PSRAM. Budget estimates:

| Consumer | Approx cost |
|---|---|
| Framebuffer (single-buffer mode) | 48 KB |
| Arduino core + FreeRTOS + heap slack | ~80 KB |
| NimBLE host + controller (central, 1 conn) | ~55-70 KB |
| Wi-Fi STA + lwIP | ~60-80 KB |
| TLS handshake (if https sync) | ~40 KB transient |

BLE + Wi-Fi + display simultaneously lands well past the ceiling once TLS
peaks are counted, and single-core radio coexistence would degrade keystroke
latency even where it fits. So InkWriter does not pretend:

- **Writing mode** (default): BLE HID + display + SD. Wi-Fi never
  initialised.
- **Sync mode** (explicit menu action): `BleHid.end()` fully deinitialises
  NimBLE and returns its RAM to the heap (the vendored host supports this
  precisely for memory-hungry phases), then Wi-Fi associates, drafts upload,
  Wi-Fi turns off, and `BleHid.begin()` restores the keyboard. The user
  sees "Stopping Bluetooth..." then "Connecting to Wi-Fi...": the mode
  switch is visible, not hidden.

TODO(hardware-test): measure free heap across the seam on real hardware;
the numbers above are pre-hardware estimates from ESP-IDF documentation and
the CrumBLE patch notes.

## Sync: conflict-safe naming, no merge logic

Every sync uploads every draft as `<draft>-<UTC stamp>.md` (one stamp per
run). Snapshots never collide and never overwrite server content; the
companion's `inkwriter export` picks the latest snapshot per draft per day
for the daily log. Merging is explicitly out of scope.

## E-ink typing loop

Keystrokes mutate the portable editor immediately; the screen repaints with
a fast (partial) refresh only after 150 ms of key silence, so a typing burst
costs one refresh, not one per key. Page scrolls and mode changes use a full
refresh. Honest latency expectations are documented in the README.

## Layering

```
lib/BleKeyboardHost/   vendored BLE HID host (CrumBLE freeink-sdk fork, MIT)
src/core/              portable: editor buffer, word counts, sprint, streak
src/device/            inkkit-facing: renderer, drafts, sync client, screens
companion/inkwriter/   serve (PUT receiver) + export (daily logs)
```
