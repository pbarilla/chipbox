# Licensing

Chipbox is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in
`LICENSE`). You may use, study, change and share it. If you give someone a firmware built from it, give them its
complete source under the same licence too.

## Who made what

|                                                                                                                                                                                           |                                                                                                                                                                                                   |
| ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| The platform: `firmware/hal/`, `firmware/loader/`, `crt0.S`, `app.ld`, `firmware/src/` (`usb.c`, `ota.c`, `recovery.c`, `lcd.c`, `gfx.c`, `libc.c`, `bootguard.h`, `uac_fir.h`), `tools/` | Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ([Felucca](https://github.com/hugelton/Felucca)), with the changes of [SLOOP](https://github.com/isod89/sloop-fm1) (isod89) |
| The browser installer `site/js/fm1ota.js` and `site/js/fm1pkg.js` (unchanged), and what `site/js/install.js` follows | Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ([Felucca](https://github.com/hugelton/Felucca), GPL-3.0-only) |
| The wiring `platform.c`, the host mode `tests/host.c`, the docs, from Dinghy                                                                                                              | zvenson                                                                                                                                                                                           |
| Chipbox itself: `chipbox.c`, `chips.c`, `seq.c`, `proj.c`, `rack.c`, `chipview.c`, `menu.c`, `picker.c`, `tests/` scripts, `site/`                                                        | pbarilla                                                                                                                                                                                          |

## Third-party material

| What                                                                                                                                                                 | Licence     | Where                                           |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------- | ----------------------------------------------- |
| Terminus font 8x16 (ter-u16n), rendered into `firmware/gen/felucca_font.h`                                                                                           | SIL OFL 1.1 | `assets/fonts/Terminus-LICENSE.txt`             |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0  | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |
| Press Start 2P (the site), `site/fonts/` | SIL OFL 1.1 | `site/fonts/OFL.txt` |
| The Bluetooth radio and link layer, `firmware/ble/` (copies, see its `README.md`; `ble.c` is ours): the stack from [fm1-base](https://github.com/charlesvestal/fm1-base) (Henri David and Charles Vestal) and the radio drivers from [felucca-bsp](https://github.com/hugelton/felucca-bsp) | GPL-3.0-only OR MPL-2.0 (used under the GPL); MPL-2.0 and Apache-2.0 | `firmware/ble/LICENSES/` |
| The SID's combined waveform tables, `firmware/gen/sid_tables.h` (read from [reSID](https://github.com/daglem/reSID) by `tools/gen_sid_tables.py`), Dag Lem | GPL-2.0-or-later (used under the GPL) | header of `firmware/gen/sid_tables.h` |
| The YM2413 instrument table (`OPLL_ROM`) and key scale levels in `tools/gen_chip_tables.py`, written into `firmware/gen/chip_tables.h`, from [emu2413](https://github.com/digital-sound-antiques/emu2413) (Mitsutaka Okazaki) | MIT | `assets/licenses/emu2413-LICENSE.txt` (the copyright notice and permission text), `tools/gen_chip_tables.py` |

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments; "M-VAVE" and "FM-1" belong to their
owners. Console and chip names (NES, Game Boy, Mega Drive, Commodore 64 and the rest) belong to their makers. Chipbox is not
made or endorsed by any of them. The other chip cores (ymfm, Nuked, libvgm, NSFPlay, MAME, VICE and others) were only run to measure
our sound in `tests/`; none of their code or tables is in this tree.
