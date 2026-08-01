# inkkit gaps

InkWriter depends on inkkit v0.1.0-rc1. Gaps found:

## 1. No Wi-Fi/HTTP helper (second app to need it, after DailyDrop)

The sync client re-implements the associate/timeout/request/off sequence
DailyDrop wrote for fetching, in the upload direction. An `inkkit::net`
module (connect, GET-to-file, PUT-from-buffer, result enum) would remove
both copies.

## 2. No BLE surface at all

inkkit has no Bluetooth story. The ecosystem now has a proven MIT BLE HID
host (CrumBLE's freeink-sdk fork, vendored here under lib/BleKeyboardHost).
inkkit should either adopt it or document it as the blessed companion
library, including the `end()`-returns-RAM contract that InkWriter's mode
switch depends on.

## 3. No text or font engine (fifth app)

Same 5x7 renderer copy as DailyDrop/InkVerse/InkQuest. A writing appliance
especially wants proportional type and larger sizes; the ecosystem
`.cpfont` system remains the obvious inkkit addition.

## 4. No key-value settings helper (second app)

config.txt parsing duplicated again (see InkVerse gap 3).
