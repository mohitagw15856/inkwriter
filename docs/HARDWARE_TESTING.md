# Hardware testing checklist

The editor/session core is host-tested (32 checks) and the companion is
pytest-covered. Device items, each a TODO(hardware-test):

## Build and flash

```sh
pio run -e xteink_x4            # run envs one at a time
pio run -e xteink_x4 -t upload
```

## Items to verify on device

- Boot order and the writing-mode default (BLE up, Wi-Fi never touched).
- BLE pairing flow with real keyboards (see README supported-keyboards):
  scan list, connect, bonded auto-reconnect after sleep/wake.
- Keystroke-to-glyph latency and the 150 ms repaint debounce; full vs fast
  refresh feel on page scrolls; ghosting after long sessions.
- Mode seam: free heap before BleHid.end(), after it, during Wi-Fi TLS
  handshake, and after returning to writing mode. This is the load-bearing
  claim in ARCHITECTURE.md.
- Sync against (a) `inkwriter serve`, (b) a WebDAV server (basic auth), and
  (c) an https endpoint; snapshot naming lands as expected.
- Autosave every 5 s of dirty state; power-hold saves before sleeping.
- Streak survives across days with the RTC; degraded (no clock) behaviour.
- Focus mode paragraph isolation while typing at the document edges.
