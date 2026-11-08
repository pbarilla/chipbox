# Brothers Link

Brothers Link is Bluetooth sync for FM-1 firmwares. It is Bluetooth Low Energy underneath, but it is not standard BLE-MIDI (section 1). One FM-1 leads, the others follow: tempo, start and stop, bar phase, kick positions (sidechain), pattern, punch-in effects, key and swing. No pairing, no connection.

This is what Chipbox (`FM-1_337`), Smileyface (`FM-1_303`) and Drumlab (`FM-1_032`) speak. Why the name: see the READMEs. The code is `firmware/ble/ble.c`, byte for byte the same file in all three repos. This page is read from that code. Anything I could not check in the code or on a bench is marked **unverified**.

For a short, step by step version to hand to a coding agent: [BROTHERS-LINK-FOR-AGENTS.md](BROTHERS-LINK-FOR-AGENTS.md). The eight punch-in effects, with recipes to build them: [PUNCH-IN-FX.md](PUNCH-IN-FX.md).

## 1. What it is, and why it is not BLE-MIDI

- A **connectionless broadcast**. The leader sends non-connectable advertising packets (`ADV_NONCONN_IND`) with a manufacturer-data payload. Followers scan passively and rebuild a clock from it. Nobody connects, nobody pairs, any number of followers can listen.
- It is **not BLE-MIDI**. No GATT, no MIDI messages, no timestamps in BLE-MIDI's format. A phone or DAW cannot use it, and it cannot receive notes. The followers turn the broadcast into MIDI-style real-time bytes `0xF8` clock, `0xFA` start, `0xFC` stop for their own sequencer (`bt_rt()`, section 7).
- Why not BLE-MIDI: the radio stack we use does one connection at a time (`ble_ll.c`: "one connection"), so a leader could not serve two followers, and a connection's events (7.5 to 11.25 ms) would add jitter to a clock. A broadcast has one sender and no link state.
- The stack also contains a BLE-MIDI peripheral. `ble.c` stubs its MIDI hooks (`ble_app_midi_peek` returns 0, `ble_app_midi_in` does nothing), so no MIDI flows even if something connects. See the "first advertising event" note in section 4.
- Company id `0xFFFF` is the Bluetooth SIG's "for testing" value. We have no registered id. The marker byte and the check (section 3) are what make a packet ours.

## 2. The radio

| | |
|---|---|
| Role | Leader: advertiser. Follower: passive scanner (no SCAN_REQ, `BLE_SCAN_ACTIVE 0`), does not advertise at all. Never both. |
| PDU | `ADV_NONCONN_IND` (type 2), TxAdd = 1 (random address): first header byte `0x42`, length byte `0x1C` (6 + 22). |
| Address | A new random static address at every radio start (`radio_up()`, top two bits set). It is not stable across power cycles. |
| Channels | 37, 38, 39 in every event, 1250 us apart (`advidx` in `ble_hw_wl82.c`). |
| Interval | `BLE_ADV_INTERVAL 32` = 20 ms (32 x 0.625 ms, the shortest the stack allows) plus the engine's random advDelay 0 to 10 ms (comment in `ble_hw.h`). So one event every 20 to 30 ms, about 35 to 50 a second. Measured rate on air: **unverified**. |
| Packet content | The leader rewrites the packet in the radio's event interrupt right after each advertising event, so the next event carries a fresh position (`hw_adv_refresh()` in `ble_hw_wl82.c`, `brothers_adv_build()` in `ble.c`). |
| Follower scan | 40 ms interval, 37.5 ms window (`BLE_SCAN_INTERVAL 64`, `BLE_SCAN_WINDOW 60`), the channel moves 37, 38, 39 every interval. A follower hears one of the three copies of an event, so its timestamp can be 0, 1.25 or 2.5 ms late. The follower does not know which (the channel is passed to `ble_ll_hw_adv_report` and dropped). The phase filter below absorbs it. |
| Report ring | `BLE_SCAN_RING 16` raw reports (any advertiser of type ADV_IND, SCAN_RSP, ADV_NONCONN_IND around you), filled in the RX interrupt, emptied by `brothers_poll()`. |
| Interrupts | The radio runs in two interrupts, IRQ 45 (event) and IRQ 29 (RX), at priority 2 (`BLE_HW_IRQ_PRIO`), below the audio and the 2 kHz tick. |

## 3. The packet

On air, after the 2 byte PDU header and the 6 byte AdvA, the advertising data is one AD structure of 22 bytes:

| Bytes | Value | Meaning |
|---|---|---|
| 0 | `0x15` (21) | AD length |
| 1 | `0xFF` | AD type: manufacturer specific data |
| 2..3 | `FF FF` | company id `0xFFFF` (little endian) |
| 4..21 | the 18 byte payload `p[0..17]` below | |

There is no Flags AD structure, no name, no service UUID. Total PDU: 2 + 6 + 22 = 30 bytes.

The payload (protocol v2). All multi-byte fields are **little endian**.

| p[] | Size | Name | Meaning |
|---|---|---|---|
| 0 | u8 | marker | `0xB8`. Also the version: v2. (v1 was a 10 byte packet with marker `0xB7`, from the project notes. No code reads it any more.) |
| 1 | u8 | seq | +1 per packet, wraps. Followers ignore it. Useful in a scanner. |
| 2 | u8 | state | bit 0: running. Bits 1..7: start counter (7 bits, +1 at every start, wraps 127 to 0). |
| 3..4 | u16 | period | Length of one clock tick in units of 2 us. A tick is 1/24 of a quarter note. us = value x 2. BPM = 2,500,000 / us. The leader computes `per_fm1ticks / 48` (`fm1_ticks` run at 24 MHz) so the value truncates: 120 BPM sends 10416 = 20832 us = 120.008 BPM. |
| 5..8 | u32 | position | Where the leader is, in 1/256 ticks, counted from its last start. Integer part = number of ticks the leader has emitted since start, the first tick being 1, plus the fraction of the tick in progress. The bar line (the first step of a 16 step pattern) is every tick whose number n has `(n - 1) % 96 == 0`: 1, 97, 193... Valid for the moment the packet goes out on air, not for when it was written (see `ETA_US`, section 4). |
| 9 | u8 | pattern | 0 = none. Otherwise bit 7 set and bits 0..3 the pattern number 0..15. |
| 10..11 | u16 | kick mask | Bit s (0..15) set = step s of the leader's current 16 step pattern has a kick. 0 = none. |
| 12..13 | u16 | fx mask | Held punch-in effects, bit n = pad n+1. Bits 0..7 are defined (section 6). Bits 8..15: reserved; senders send 0 and all receivers mask with `0xFF`. |
| 14 | u8 | scale | Low nibble: root 0..11 (C = 0). High nibble: 0 none, 1 major, 2 minor. 0 = no key. |
| 15 | u8 | swing | 0..96: how late the odd steps come, in 1/96 of a step. 96 would be a full step. 0 = straight. |
| 16 | u8 | reserved | Always 0. |
| 17 | u8 | check | `0x5A` XOR `p[0]` XOR ... XOR `p[16]`. |

Example: 120 BPM, running, 3rd start, position 97.5 ticks (half a tick after the downbeat of bar 2), pattern 2, kicks on steps 0, 4, 8, 12, FX STOP + LPF held, key D major, swing 12:

```
B8 07 07 B0 28 80 61 00 00 82 11 11 05 00 12 0C 00 02
```

Size rule: a follower accepts a packet only if the PDU is exactly 30 bytes, `AD[0..3] = 15 FF FF FF` and `p[0] = B8`, and the check matches. **Any change to the length needs a new marker value**, or old followers drop it.

Packets look the same stopped or playing. Only `p[2]` bit 0 changes (and the position stops advancing, below).

## 4. The leader

A leader is a firmware that calls, from its audio interrupt, one function per clock tick, plus a few state setters (section 7).

- `brothers_lead_run(1)` at start: `lead_n = 0`, `lead_ts = now`, start counter +1, run = 1. `brothers_lead_run(0)` at stop: run = 0 (the tick counter and the tempo stay).
- `brothers_lead_tick(off, per)` at every MIDI clock tick (24 a quarter note, 6 a step). `off` = the sample, inside the audio block being rendered, where the tick falls (0..block-1): the tick's time is `fm1_ticks() + off x 544` (24 MHz / 44.1 kHz). `per` = tick period in `fm1_ticks` = **60,000,000 / BPM** (500,000 at 120 BPM). Give it the exact tempo, or followers will show and play a slightly different tempo.
- The first `brothers_lead_tick` after `brothers_lead_run(1)` must be the downbeat tick: tick number 1 is step 0 of bar 1. Chipbox, Smileyface and Drumlab arrange their sequencers so the first tick fires at the first step.
- When the next packet is built, `position = lead_n x 256 + (since x 256) / per`, with `since = (fm1_ticks() - lead_ts) + ETA_US x 24`. `ETA_US = 21500`: the packet goes out about this long after it is written (one interval plus the middle of the advDelay minus the event's start). It was tuned by ear. `since` is capped at 8 periods.
- **Playing**: position advances with the ticks and in between, extrapolated.
- **Stopped** (but Lead on): packets go on at the same rate with run = 0. The position stops moving after at most 8 ticks beyond the last tick. Pattern, FX, key and swing keep updating. A leader that was never started sends period 500,000 fm1_ticks (120 BPM) and position 0.
- **Lead off** (`brothers_set(0)`): advertising stops. Followers see silence.
- The first advertising event after `brothers_set(1)` is the stack's own connectable `ADV_IND` (name and BLE-MIDI service), because the packet is only replaced after that first event. Inferred from `ble_hw_wl82.c`, **unverified on air**.
- Two leaders at once: nothing arbitrates. A follower locks onto the first address it hears and ignores the other. Which one that is, is luck.

## 5. The follower

A follower calls `brothers_poll()` often. It takes new packets from the scan ring, keeps a phase locked estimate of the leader's tick count, and calls `bt_rt(0xF8 / 0xFA / 0xFC)` at the right times. The host turns those into its own clock.

**Accepting a packet** (`fol_take()`): PDU type 2, exact length 30, `AD = 15 FF FF FF B8`, check ok. The first leader heard fixes the address; packets from other addresses are ignored until the lock is dropped. Then `fol_packet()`:

1. Period `per_us = p[3..4] x 2`. Outside 5,000 to 120,000 us (500 down to 20.8 BPM) the packet is ignored.
2. Receive time: the radio's RX interrupt stamps the packet with `fm1_ticks()`; `fol_packet()` subtracts **400 us** (the interrupt's lateness, tuned by ear).
3. First packet (lock): reference = (time, position, period). Nothing is emitted until a bar line.
4. Later packets: estimate `est = fol_pos(time)` = reference position + elapsed us x 256 / `per_us` (elapsed capped at 4 s). `err = position - est`.
   - `|err| > 3 ticks`: the leader jumped; take its position.
   - else a nudge: `ref = est + err / 8`.
   - period: `per_us = (7 x per_us + new) / 8`.
5. Record `run`, pattern, kick mask, fx, scale, swing. Time of last packet.
6. If the start counter changed: emit `0xFC` first if playing, drop `playing`, set the emitted-tick counter to the leader's position, and wait for the next bar line (which is at once if the leader just restarted, since its position is 0).

**Making clock, start and stop** (`fol_run()`, every call of `brothers_poll()`):

- `c = fol_pos(now) >> 8` (the leader's whole ticks now). While `c > emitted` (at most 24 per call): `emitted++`; if not playing, run = 1 and `(emitted - 1) % 96 == 0` then `bt_rt(0xFA)`, playing; if playing `bt_rt(0xF8)`. So a follower joins **at a bar line**, and a start is followed by a clock for that same tick, as in MIDI.
- If playing and run = 0: `bt_rt(0xFC)`.
- More than 24 ticks behind (a long stall): skip ahead (`emitted = c`).
- Tempo: `brothers_bpm()` = `(2,500,000 + per_us / 2) / per_us`. Chipbox and Smileyface set their tempo from this (kept to 30..255). Drumlab measures tick spacing (smoothed 3:1) and displays the same.
- **No `0xFB` (continue) is ever generated.** A restart is a new `0xFA` at the next bar line.

**When the leader goes away**: 1.5 s without a valid packet (`fol_run()`): `bt_rt(0xFC)` if playing, then the lock is dropped. The follower scans again and will lock onto the next leader it hears (any, joining at a bar line). The host should fall back to its own clock then (Chipbox, Smileyface and Drumlab drop back to their own clock about 0.4 s after the last tick). Changing the mode with `brothers_set()` also clears the lock and sends `0xFC` if playing.

Numbers to remember: 1/8 nudge per packet, 1/8 period filter, 3 tick jump threshold, 400 us RX offset, 21.5 ms leader ETA, 1.5 s timeout, 24 ticks per poll, 4 s extrapolation cap.

Phase error in the field (sync drift over minutes, jitter in ms): **unverified**. The project notes record only that a user heard play, stop and tempo as exact with Drumlab leading and Chipbox and Smileyface following.

## 6. Sidechain, pattern follow, FX, key, swing

All of these are in the same packet. The leader sends every field every time; a **follower option** decides whether it uses them. In the three firmwares the options are the menu switches Sidechain, Patterns, Punch-in FX and Key, all Off until the user sets them.

### Sidechain (duck)

`brothers_duck(depth)` returns the volume to use now, 0..256 (256 = untouched). It is called once per audio block. It looks at the last four steps of the leader's 16 step pattern from the kick mask:

- The leader's position gives the playing step `step0 = (ticks - 1) / 6` (6 ticks a step; steps are numbered by bar, `&15` when indexing the mask).
- For each of the last 4 steps `st` with its kick bit set, age = time since that step began, from the position and `per_us`. If age < 200 ms: `g = 256 - depth + depth x age / 200000 us`. The lowest `g` wins.
- Only while the leader is running and the mask is not 0. Otherwise 256.
- Effect: at the kick the volume drops at once to `256 - depth`, then climbs back linearly in **200 ms**. All three firmwares call it with `depth = 190`: down to 66/256 (about -11.7 dB) at the kick. The gain steps per audio block (5.8 ms); there is no per-sample smoothing (unverified whether that is audible).
- The mask assumes the leader plays a 16 step bar of 96 ticks and the follower is bar-locked. A leader pattern of another length would not line up. How a leader fills the mask: Drumlab, lane 0 hits; Chipbox, drum channels' kick hits; Smileyface has no drums and sends 0.
- What a follower ducks is the host's choice: Drumlab the whole mix, Chipbox its note channels but not its drums, Smileyface the bass.

### Pattern follow

`p[9]` carries the leader's current pattern (0..15 with bit 7). `brothers_pat()` gives 0xFF when there is none or no lock. The packet says nothing about *when*: the three followers apply it at the end of their own loop, which is the leader's bar line because both are bar-locked and loops are 16 steps. They only act when the pattern changes. A start from the leader also moves Chipbox to the leader's page. A pattern number the follower does not have is ignored.

### Punch-in FX

Bit n of `p[12..13]` = pad n+1 held on the leader. The shared vocabulary:

| Bit | Value | Pad | Effect |
|---|---|---|---|
| 0 | 0x01 | 1 | STOP (tape stop) |
| 1 | 0x02 | 2 | REPEAT |
| 2 | 0x04 | 3 | LPF |
| 3 | 0x08 | 4 | HPF |
| 4 | 0x10 | 5 | ECHO |
| 5 | 0x20 | 6 | REVERB |
| 6 | 0x40 | 7 | CRUSH |
| 7 | 0x80 | 8 | GATE |

How each effect sounds and how to build it: [PUNCH-IN-FX.md](PUNCH-IN-FX.md). Pads 9 to 15 are each firmware's own and do not travel. The hold is level, not an event: a follower sets its pads to the mask as long as it is held and lets go when the bit clears. Chipbox implements them in its own FX engine; Smileyface and Drumlab share `bfx.c`. The follower acts on the mask only while it is following, and only when its Punch-in FX switch is on.

### Key and scale

`p[14]`: root in the low nibble (0..11), type in the high nibble (1 major, 2 minor, 0 none). Leaders that play notes (Chipbox, Smileyface) call `brothers_detect(notes, n)` about every 300 ms: it needs at least 3 distinct pitch classes, the root is the lowest note's pitch class, and the type is minor if the minor third has more notes than the major third, else major. A follower with Key on moves each note to the nearest note of the key (`brothers_snap`, a lower note first on a tie), using major 0,2,4,5,7,9,11 and natural minor 0,2,3,5,7,8,10 above the root. Drumlab sends 0 (no notes) and has no Key switch.

### Swing

`p[15]`: odd steps late by `swing / 96` of a step. Drumlab sends its swing x 8/3 and, as a follower with Patterns on, takes it x 3/8 back. Smileyface sends its own SWG and, with Patterns on and a pattern given, takes it (limited to 40). Chipbox sends 0 and has no swing to follow.

## 7. The API in `ble.c`

Everything the host sees, with the context it is meant to be called from. All state is `volatile` statics; there are no locks.

| Function | Who | Context and rate |
|---|---|---|
| `void brothers_set(uint32_t mode)` | any | Main loop, when the user changes it. 0 off, 1 lead, 2 follow. The first non-zero call brings the radio up (`radio_up()`: random address, `ble_init`, attach IRQ 45 and 29, radio calibration, `ble_hw_wl82_start`): it runs in the caller, how long it takes is **unverified**, keep the watchdog fed. Off stops advertising and scanning; the radio stays initialised until reset (no shutdown code exists). |
| `volatile uint8_t brothers_mode`, `volatile uint32_t brothers_state` | any | Read anywhere. mode: 0, 1, 2. state: 0 radio off, 1 radio up. |
| `void brothers_lead_run(uint32_t run)` | leader | Start edge (1) and stop edge (0). Once each. Before the first tick. |
| `void brothers_lead_tick(uint32_t off, uint32_t per)` | leader | The audio interrupt (ours), at every clock tick while running. |
| `void brothers_lead_set(pat, kick, fx, scale, swing)` | leader | Any context, e.g. each main loop pass. `pat` 0..15 or 0xFF none. |
| `void brothers_poll(void)` | follower | At least every ~10 ms. Chipbox and Smileyface: each main loop pass. Drumlab: once per audio block from the render. The ring holds 16 reports; a call can emit up to 24 ticks. Whoever calls it, calls `bt_rt` from the same context. |
| `void bt_rt(uint32_t st)` | **host provides** | Called by `brothers_poll()` with `0xF8`, `0xFA` or `0xFC`. Must be cheap and not block. Chipbox and Smileyface push it into a 32 byte queue for the main loop (as USB MIDI); Drumlab bumps counters. |
| `brothers_pat`, `brothers_fx`, `brothers_scale`, `brothers_swing`, `brothers_kick` | follower | Last values, 0 (0xFF for the pattern) when not following or no lock. Read in the same context as `brothers_poll()`, or anywhere if you accept a stale value. |
| `brothers_duck(depth)` | follower | Once per audio block. Reads the clock estimate and `fm1_ticks()`. |
| `brothers_detect`, `brothers_snap` | leader, follower | Pure functions. |
| `brothers_bpm()` | follower | 0 when not locked. |
| `brothers_link()` | any | Menu status: follower 0 searching, 1 locked, 2 playing along; leader 1 ready, 2 leading. The three firmwares show Off / Ready / Leading / Searching / Linked / In sync. |

Also exported: `isr_ble_event`, `isr_ble_rx`, `ble_wl82_event_irq`, `ble_wl82_rx_irq` (the vectors), `shim_ms`, `shim_log`. `ble.o` needs from the host: `bt_rt`, `memcpy`, `memset`. Nothing else (checked with the symbol table of `build/ble.o`).

`ble_m1_start()`, which `CLAUDE.md` still mentions, is gone: it was the first bring-up (advertise only). `brothers_set()` replaced it.

## 8. Radio, USB and power: what to know

- **The radio makes the FM-1 miss installer requests.** With the radio on, handshakes were lost (about 5% on Drumlab, full installs stalling after 0 to 4 requests; Chipbox and Smileyface 0%), from the project notes. So each app watches `ota_hit_ms` (set in `ota.c` when the installer talks, `APP_OTA_HIT`) and calls `brothers_set(0)`, then restores the old mode 20 s after the last request. See `lab.c` ("paused"). The page docs also tell users to set Sync to Off before installing.
- **Bluetooth is always Off after power up and must never stay on across a power cycle** (the user's rule). The mode bits are still stored in the settings but nothing reads them at boot: only the switches (sidechain, patterns, FX, key) are restored. Do not call `brothers_set()` at start-up.
- The radio is never started by the update loader. `ble.c` is linked into the app only. The loader's hash is pinned in `tools/build.py`.
- A hung app: the watchdog is fed only in the main loop (`platform_poll()`), not in an interrupt. Two failed boots in a row drop into USB rescue by themselves. Holding OCT- at power-on is the manual rescue. The radio does not change any of that.
- Calibration: `radio_up()` sets `fm1_rf_cal_on = 1`: the radio calibrates itself at every start.
- Battery and radio power: **unverified**.

## 9. Cost

From `build/ble.o` of the latest builds (not linked, before `--gc-sections`): text 26.9 KB, rodata 7.3 KB (the radio start-up tables are in it), data 98 B, bss 5.9 KB, noinit 48 B, `.ram_text` 36 B. Image sizes after linking: Drumlab 159 KB, Smileyface 133 KB, Chipbox 321 KB. The build refuses to go over 96 KiB of RAM `.data` + `.bss`. CPU in the radio interrupts: **unverified** (no measurement in the repos). The follower's own work per call is small: a ring read, a few divisions, at most 24 `bt_rt` calls.

## 10. Licence of what to copy

Copy the whole `firmware/ble/` folder, with `LICENSES/` and `README.md`. The files have their own licences, in each file's SPDX line:

| Files | Licence |
|---|---|
| `ble.c` | GPL-3.0-only (ours) |
| `stack/*` (from [fm1-base](https://github.com/charlesvestal/fm1-base), Henri David and Charles Vestal) | GPL-3.0-only OR MPL-2.0. We use it under the GPL. |
| `chip/fm1_ble.h`, `chip/fm1_ble_rf.h` | GPL-3.0-only OR MPL-2.0 |
| `chip/fm1_cc.h`, `fm1_guard.h`, `fm1_irq.h`, `fm1_time.h` (from [felucca-bsp](https://github.com/hugelton/felucca-bsp), Leo Kuroshita and Charles Vestal) | MPL-2.0 |
| `chip/ble_rf_tables.h`, `chip/fm1_rf_cal.h` (radio start-up captured from M-VAVE's stock V15, and the calibration after JieLi's SDK) | Apache-2.0 |

`fm1_irq.h` and a few `stack/` files carry small local changes (marked `Drumlab:` or `FM1_IRQ_NO_FAULT`): `ble_ll_central.c` and `ble_hw_wl82.c` for the broadcast, `fm1_irq.h` for no crash record. The README in that folder says "kept as they are": that is not exact. If you copy, keep the changed files as they are in ours, or diff. This is not legal advice: read each file's header and `LICENSES/`; the MPL files stay MPL.

## 11. Interop checklist

Test your firmware against one real Chipbox, Smileyface or Drumlab. Tick them in order.

1. Scan with a phone or the Python in the agents file. With a brother set to Lead you see manufacturer data `0xFFFF`, 18 bytes starting `B8`, the check matches, `seq` rises, packets every 20 to 30 ms, no name, not connectable.
2. Your firmware as leader: the brother's status goes Searching, Linked. Press play: In sync, and it starts at a bar line (within one bar, on the leader's downbeat).
3. Tempo: 60, 120, 180, 240 BPM on your leader. The brother shows the same BPM (it is `2,500,000 / per_us`).
4. Stop on the leader: the follower stops. Start again: the follower restarts at the next bar line.
5. Turn your leader off or power it down: the follower stops by 1.5 s and goes to Searching.
6. Your firmware as follower, with a brother leading: starts on the bar line, tempo matches, stops when the leader stops, stops about 1.5 s after the leader is switched off.
7. Sidechain: Drumlab or Chipbox leader with kicks on steps 0, 4, 8, 12: your follower dips on each and comes back in 200 ms.
8. Patterns: change the leader's pattern: the follower changes at its next bar end.
9. Punch-in FX: hold pads 1 to 8 one by one on a brother leader: bits 0 to 7 in the packet, your follower reacts (see [PUNCH-IN-FX.md](PUNCH-IN-FX.md)).
10. Key: a Chipbox or Smileyface leader with notes in a minor key: `p[14]` high nibble 2, root = its lowest note.
11. Installer: with Sync on, run the installer against your own firmware. Does it still reach it? Does your app pause the radio, as the brothers do?
12. Power cycle with Sync on. Sync must be Off after the restart, with the switches kept.
13. Hold OCT- at power-on with the radio on first: rescue must still come up.
14. Stress: leave it 30 minutes. Leader and follower must still be bar-locked (look at the downbeat), nothing drifts.

## 12. Known gaps and unverified

- Air rate, jitter, drift and CPU numbers: unverified.
- Time `radio_up()` blocks: unverified. Radio power draw: unverified.
- First event being a connectable `ADV_IND`: from code only.
- `0xFFFF` company id can collide with any other test tool. The marker and check keep it unlikely to be mistaken.
- No continue (`0xFB`), no song position, no tempo changes between ticks other than the period field, no leader election.
- If two options use the same clock at once (MIDI Clock Follow and Bluetooth Follow both on), a host that feeds both into one tick counter gets double ticks. The hosts gate each source by its own mode, but both can be on at once.
