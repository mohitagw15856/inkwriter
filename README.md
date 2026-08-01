# InkWriter

**Distraction-free writing appliance for the Xteink X4/X3 with a Bluetooth
keyboard.** Pair a BLE keyboard, open a draft, and write on paper-like ink:
word counts, session goals, sprints and streaks stay subtle; everything else
stays out of the way.

Status: builds in CI, not yet verified on device.

Compatible with the CrossPoint ecosystem: same Xteink X4/X3 hardware,
device layer entirely from
[inkkit](https://github.com/mohitagw15856/inkkit) (pinned in
`platformio.ini`), drafts in plain Markdown under `/inkwriter/drafts` on
the SD card. The BLE HID host is reused from the CrumBLE freeink-sdk fork
(MIT), not reinvented; see ARCHITECTURE.md.

## Features

- BLE keyboard pairing (scan, connect, bonded auto-reconnect)
- Plain Markdown drafts, autosaved every 5 seconds
- Word count, session goal with an 8-segment progress bar, sprint timer,
  daily writing streak
- Focus mode: only the current paragraph on screen (Tab toggles)
- Explicit writing mode vs sync mode: Wi-Fi sync tears BLE down first and
  restores it after (the ~380 KB RAM ceiling makes them exclusive; see
  ARCHITECTURE.md for the budget)
- Conflict-safe sync: every upload is a timestamped snapshot; nothing is
  ever merged or overwritten

## Supported keyboards

Any **BLE** (Bluetooth Low Energy) keyboard exposing the standard HID
service (0x1812) should work: typical low-profile travel keyboards
(Logitech K380 in BLE mode, Keychron K-series in BLE mode, Anne Pro 2,
NuPhy Air series) and BLE page-turner remotes. Two honest caveats:

- **No Bluetooth Classic.** The ESP32-C3 has no BR/EDR radio; older
  Classic-only keyboards cannot connect at all.
- **US layout mapping.** Keycodes translate through a US-layout map;
  other layouts type but punctuation may land shifted.

Pairing: Menu -> Pair keyboard, put the keyboard in pairing mode, pick it
from the list. Bonds persist; the keyboard reconnects on wake.

## Latency expectations (read this before judging e-ink typing)

E-ink is not an LCD. InkWriter batches keystrokes and repaints with a fast
partial refresh after ~150 ms of key silence, so glyphs appear in bursts a
beat behind your fingers (roughly 300-500 ms behind at speed, to be
measured on hardware). Page turns use a full refresh (~700 ms class) to
clear ghosting. Touch typists report this feels like a typewriter cadence:
fine for drafting, wrong for editing-heavy work. If you need per-keystroke
echo, this is not the machine.

## Install and flash

```sh
pio run -e xteink_x4              # or -e xteink_x3; run one at a time
pio run -e xteink_x4 -t upload
```

Optional `/inkwriter/config.txt` on the SD card:

```
goal 500
sprint_minutes 15
ssid YourNetwork
pass YourPassword
url  http://192.168.1.10:8377
user webdav-user-if-any
password webdav-pass-if-any
```

## Companion

```sh
pip install -e "companion[test]"
inkwriter serve --root ~/drafts          # receives device uploads (HTTP PUT)
inkwriter export --root ~/drafts --out ~/logs   # daily combined writing log
```

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - reuse-vs-rebuild study, RAM budget, mode design
- [docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md) - on-device checklist
- [docs/INKKIT_GAPS.md](docs/INKKIT_GAPS.md) - device-layer gaps for inkkit
- [CONTRIBUTING.md](CONTRIBUTING.md)

## Licence

MIT, see [LICENSE](LICENSE). `lib/BleKeyboardHost` is vendored from the
CrumBLE freeink-sdk fork (MIT); the 5x7 text renderer is shared ecosystem
code originating in InkQuest (same author, MIT).
