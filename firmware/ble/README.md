# firmware/ble: the Bluetooth radio and link layer

Not ours. These files are copies, kept as they are, from two projects:

- `stack/*`: the BLE stack (link layer, host, ATT, BLE-MIDI, pairing) from [fm1-base](https://github.com/charlesvestal/fm1-base) `bt/ble/`, written by Henri David (Optimist) and Charles Vestal. SPDX: GPL-3.0-only OR MPL-2.0 (we use it under the GPL).
- `chip/*`: the radio's registers and start-up from [felucca-bsp](https://github.com/hugelton/felucca-bsp) `chip/` (MPL-2.0; the radio drivers by Optimist, the calibration after JieLi's SDK, the start-up tables `ble_rf_tables.h` captured from M-VAVE's stock V15, Apache-2.0). See `LICENSES/` and each file's header.

`ble.c` is ours (Drumlab's glue). It is built as its own object and linked into the APP only: the update loader never contains any of this.
felucca-bsp commit 5b0879b, fm1-base as cloned 2026-11-07.
