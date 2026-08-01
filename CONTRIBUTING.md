# Contributing to InkWriter

Thanks for helping. Quick facts:

- The device layer comes exclusively from
  [inkkit](https://github.com/mohitagw15856/inkkit), pinned in
  `platformio.ini`. Do not add freeink-sdk submodules or symlinks; if the
  device layer is missing something, record it in `docs/INKKIT_GAPS.md`.
- Run pio environments one at a time (`pio run -e xteink_x4`, then
  `-e xteink_x3`); concurrent pio invocations race on `~/.platformio`.
- Before opening a PR, all of these must pass locally:

```sh
pio run -e xteink_x4
pio run -e xteink_x3
./test/host/run.sh
cd companion && pip install -e ".[test]" && pytest
```

- Portable logic goes in `src/core/` (Arduino-free, host-tested); anything
  touching inkkit goes in `src/device/` and carries `TODO(hardware-test)`
  markers for behaviour that needs a physical device.
- British English in docs. No em dashes. Conventional commit subjects
  (`feat:`, `fix:`, `docs:` ...).
- Work on a branch and open a PR; CI must be green before review.
