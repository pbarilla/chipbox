# Add Brothers Link to an FM-1 firmware: instructions for a coding agent

Task: make this FM-1 firmware lead and/or follow Brothers Link, the Bluetooth Low Energy sync (not standard BLE-MIDI) of Chipbox, Smileyface and Drumlab. Read [BROTHERS-LINK.md](BROTHERS-LINK.md) for the spec; this file is the work order. Do the steps in order. Do not skip the checks.

Rules:

- Never change the update loader. Never link the radio into it.
- Do not flash or touch any device unless the user says so. Do not record audio.
- Bluetooth must be Off after every power up. Never restore it from flash.
- Copy `firmware/ble/` as it is. Do not "clean it up".

## 0. What your firmware must already have

- The Felucca / Dinghy platform: `hal/`, `crt0.S`, `app.ld` (with a `.noinit` region), `tools/build.py` like ours, `fm1_time_init()` run at start (TIMER4 at 24 MHz: `fm1_ticks()`), `fm1_irq_init()`, a watchdog fed in the main loop.
- A sequencer driven by a MIDI-clock-like tick, 24 a quarter note, 6 a step, run from the audio interrupt.
- A main loop that calls `platform_poll()`.
- A way to feed a MIDI clock in (`0xF8`, `0xFA`, `0xFC`) to the sequencer, if you want to follow.

## 1. Copy

Copy the whole folder `firmware/ble/` from any of the three repos (they are identical: `diff -r` is empty) into your `firmware/ble/`:

```
firmware/ble/ble.c         the glue (GPL-3.0-only, ours)
firmware/ble/stack/        BLE link layer + host (from fm1-base)
firmware/ble/chip/         radio registers, start-up tables (from felucca-bsp)
firmware/ble/LICENSES/     MPL-2.0, Apache-2.0
firmware/ble/README.md
```

Keep the licences and the headers of every file. See section 10 of the spec.

## 2. Build it as its own object, into the app only

`ble.c` is its own translation unit. It must see the headers in `ble/chip` and `ble/stack` and **not** your `hal/` (same file names, different content). In `tools/build.py` (copy from `drumlab/tools/build.py`):

```python
BLE_FLAGS = ["-Os", "-ffunction-sections", "-fno-builtin", "-Wall", "-Wno-unused-function",
             "-Ifirmware/ble/chip", "-Ifirmware/ble/stack"]          # not -Ifirmware/hal
# optional: '-DBLE_DEVICE_NAME="FM-1 MyBox"'   (only used in the very first advertising event)
```

In `build_app()`, next to the compile of your main unit, add one line:

```python
("cc", *BLE_FLAGS, "-c", FW / "ble" / "ble.c", "-o", OUT / "ble.o")
```

and add `OUT / "ble.o"` to the `ld` line of the app (after your own object). Do **not** add anything to `build_loader()`.

Check after the build:

- `loader: ota.bin NNNN B` is the same as before, and the build does not complain about `LOADER_SHA256`. If the loader changed, you did something wrong. Do not flash.
- RAM line: `.data+.bss` under 96 KiB. `ble.o` adds about 6 KB bss, 27 KB code, 7 KB rodata.
- Keep `-Os` for the app. `-O2` is known to break the USB SIE code (`usb.c`).
- `ble.o` needs `bt_rt`, `memcpy`, `memset` from you and defines `brothers_*`, `isr_ble_event`, `isr_ble_rx`, `ble_wl82_event_irq`, `ble_wl82_rx_irq`, `shim_ms`, `shim_log`. If your code has symbols of these names, rename yours.

Toolchain: JieLi `pi32v2` clang, Linux x86-64 only. On macOS `tools/build.py` runs it in a Docker container (`JIELI_TOOLCHAIN`, `JIELI_DOCKER=1`). Dinghy's `docs/RULES.md` has the platform rules.

## 3. Declare the API in your main unit

```c
extern volatile uint8_t brothers_mode;               /* 0 off, 1 lead, 2 follow */
extern void brothers_set(uint32_t mode);
extern void brothers_poll(void);
extern uint32_t brothers_link(void);                 /* 0 searching, 1 locked / ready, 2 playing / leading */
extern uint32_t brothers_bpm(void);                  /* follower: the leader's BPM, 0 none */
extern void brothers_lead_run(uint32_t run);
extern void brothers_lead_tick(uint32_t off, uint32_t per);
extern void brothers_lead_set(uint32_t pat, uint32_t kick, uint32_t fx, uint32_t scale, uint32_t swing);
extern uint32_t brothers_pat(void), brothers_fx(void), brothers_scale(void), brothers_swing(void), brothers_kick(void);
extern uint32_t brothers_duck(uint32_t depth);
/* and you provide: */
void bt_rt(uint32_t st);                             /* 0xF8 clock, 0xFA start, 0xFC stop */
```

For a host simulator build without the radio, guard these with stubs (see the top of `drumlab/firmware/src/lab.c`).

## 4. The hooks

### Leader: audio interrupt

```c
/* when the sequencer starts (from either context) */
if (brothers_mode == 1u) brothers_lead_run(1);       /* BEFORE the first tick */
/* when it stops */
if (brothers_mode == 1u) brothers_lead_run(0);

/* in the audio render, at every clock tick while running.
 * i = the sample inside this block where the tick falls (0..n-1),
 * bpm = the exact tempo */
if (brothers_mode == 1u) brothers_lead_tick(i, 60000000u / bpm);
```

- Tick 1 after `brothers_lead_run(1)` is step 0 of the first bar. Make your sequencer fire its first tick on its first step.
- Ticks are 24 per quarter note. If your sequencer has another resolution, give it only every n-th tick that makes 24.
- `per` is in `fm1_ticks` (24 MHz): `60,000,000 / BPM`. Be exact.
- Tempo changes: just give the new `per` at the next tick.

### Leader: main loop (every pass)

```c
if (brothers_mode == 1u)
    brothers_lead_set(cur_pattern /* 0..15, or 0xFF */, kick_mask /* bit s: step s has a kick, 0 if none */,
                      held_fx_mask & 0xFFu /* bit n = pad n+1, section 6 of the spec */,
                      0 /* or brothers_detect(notes, n) every ~300 ms */, swing /* 0..96 */);
```

### Follower: main loop (every pass) and the clock

```c
static uint8_t btq[32]; static volatile uint8_t btq_w, btq_r;
void bt_rt(uint32_t st) { if ((uint8_t)(btq_w - btq_r) < 32u) btq[btq_w++ & 31u] = (uint8_t)st; }

/* main loop */
brothers_poll();                                     /* the radio's packets into bt_rt() calls */
while (btq_r != btq_w) {
    uint8_t st = btq[btq_r++ & 31u];
    if (brothers_mode != 2u) continue;               /* only when following */
    if (st == 0xF8u) { /* tick */ ; seq_bpm = brothers_bpm(); }
    else if (st == 0xFAu) { /* start: step 0 on the NEXT 0xF8 */ }
    else if (st == 0xFCu) { /* stop */ }
}
```

- Your sequencer, when following, advances only on these ticks (6 per step) and falls back to its own clock when no tick came for about 0.4 s. Chipbox: `ext_on`, `ext_in`, `ext_seen` in `chips.c`. Smileyface: `voice.c`. Drumlab: `labcore.c` `fol_ticks`.
- Drumlab calls `brothers_poll()` from the audio render and uses counters instead of a queue. Either is fine; keep `brothers_poll()` and `bt_rt()` in the same context. Call it at least every 10 ms.
- If you also follow a USB MIDI clock, keep the two sources apart (we tag the Bluetooth ones with `d1 == 0x55`) and gate each by its own mode, so both are not counted.
- A start `0xFA` arrives at a bar line. A restart of the leader gives `0xFC`, then `0xFA` at the next bar line. There is no `0xFB`.

### Follower: the extras (all optional)

```c
/* audio render, once per block */
uint32_t duck = (brothers_mode == 2u && sidechain_on) ? brothers_duck(190u) : 256u;
/* ... multiply what you want ducked by duck / 256 ... */

/* main loop, with the options on */
uint32_t p = brothers_pat();       if (p != 0xFFu && p != last) { last = p; /* switch pattern at the end of your loop */ }
uint32_t fx = brothers_fx() & 0xFFu;  /* hold your pads 1..8 as bit n = pad n+1; the eight effects and how to build them: PUNCH-IN-FX.md */
uint32_t key = brothers_scale();   /* with Key on: note = brothers_snap(note, key) before you play it; 0 = no key */
uint32_t sw = brothers_swing();    /* 0..96 odd steps late, in 1/96 of a step */
```

The eight punch-in effects, with a recipe for each: [PUNCH-IN-FX.md](PUNCH-IN-FX.md).

### The menu (copy our words)

A page called **Bluetooth** in your GLO + HOME menu: `Sync: Off / Lead / Follow` (OCT+ cycles; call `brothers_set()`), `Status: Off / Ready / Leading / Searching / Linked / In sync` (from `brothers_mode` and `brothers_link()`), then your switches (Sidechain, Patterns, Punch-in FX, Key). Switches may be saved; **Sync must not be restored at boot**.

### The installer pause (mandatory)

The radio makes the FM-1 miss installer requests. Copy this block into your main loop (see `drumlab/firmware/src/lab.c`, `ota_hit_ms` comes from `ota.c` when `APP_OTA_HIT` is defined, `platform.c` declares it):

```c
{ static uint32_t paused;
  if (ota_hit_ms && brothers_mode && !paused) { paused = brothers_mode; brothers_set(0); }
  if (paused && fm1_ms - ota_hit_ms > 20000u) { brothers_set(paused); paused = 0; ota_hit_ms = 0; } }
```

Also: after `ota_session()` returns without committing, repaint the whole screen (our `ota_back` flag).

## 5. What to expect on the air

A Lead brother sends one non-connectable advertising packet every 20 to 30 ms on channels 37, 38 and 39:

- no local name, no service UUIDs, no Flags;
- manufacturer data, company id `0xFFFF`, 18 bytes;
- the first byte is `0xB8`; the last is the XOR check; byte 1 counts up;
- address: random static, new at each radio start.

Example payload (120 BPM, running, position 97.5):

```
B8 07 07 B0 28 80 61 00 00 82 11 11 05 00 12 0C 00 02
```

When the leader stops, byte 2 loses bit 0. When it is off, nothing is sent. A Follow brother sends nothing at all.

## 6. Test with a scanner

Phone: nRF Connect or LightBlue. Look for a device with no name, "Manufacturer data 0xFFFF", raw bytes starting `0xB8`, not connectable.

Python with [bleak](https://github.com/hbldh/bleak) (`pip install bleak`). Run it with the leader on:

```python
import asyncio, struct
from bleak import BleakScanner

def decode(p):
    if len(p) != 18 or p[0] != 0xB8: return None
    c = 0x5A
    for b in p[:17]: c ^= b
    if c != p[17]: return None
    per = (p[3] | p[4] << 8) * 2
    return dict(seq=p[1], run=p[2] & 1, start=p[2] >> 1, per_us=per, bpm=round(2.5e6 / per, 2),
                tick=struct.unpack('<I', p[5:9])[0] / 256, pat=(p[9] & 15) if p[9] & 0x80 else None,
                kick=f"{struct.unpack('<H', p[10:12])[0]:016b}", fx=f"{struct.unpack('<H', p[12:14])[0]:08b}",
                root=p[14] & 15, scale=p[14] >> 4, swing=p[15])

def cb(dev, adv):
    p = adv.manufacturer_data.get(0xFFFF)
    if p is None: return
    d = decode(bytes(p))
    print(dev.address, adv.rssi, d if d else ("not ours: " + bytes(p).hex()))

async def main():
    async with BleakScanner(cb):
        await asyncio.sleep(30)
asyncio.run(main())
```

What to check: `check` passes (the function returns a dict), `seq` rises by one per packet, `tick` rises with the leader's tempo (`1 / (per_us x 1e-6)` ticks a second), `run` follows the leader's play button, `start` goes up by one at each start, `kick` and `fx` show the leader's steps and pads. On macOS CoreBluetooth may merge or delay advertisements, so the rate you see can be lower than the true 35 to 50 a second (unverified).

For your follower, the test is the other way round: let a brother lead and watch your tempo, start at the bar line, and stop 1.5 s after the leader is turned off. Then go through the interop checklist in the spec (section 11).

## 7. Pitfalls

1. The loader. Never add `ble.o` or radio flags to it. If the loader hash differs, stop. A bad loader bricked a test FM-1.
2. Headers. `ble.c` must be built with `-Ifirmware/ble/chip` and no `-Ifirmware/hal`.
3. `-O2` for the app. Do not.
4. `brothers_lead_run(1)` before the first tick, and the first tick on the first step. If the follower starts one step late or early, this is why.
5. `per` must be `60,000,000 / BPM` in 24 MHz ticks, not microseconds.
6. `brothers_set()` brings the radio up in the caller and takes time. Main loop only, user action only, with the watchdog fed afterwards. Never at boot, never from an interrupt.
7. Never restore Sync from flash. The user's rule: Bluetooth is off after every power up.
8. Pause the radio while the installer talks (section 4). Without it, installs of your firmware will stall with the radio on.
9. `bt_rt()` must not block and must be cheap. Do not call `midi_send()`-style code that takes the main loop's lock from an interrupt.
10. Do not feed Bluetooth ticks and USB clock ticks into one counter at once.
11. A follower that keeps its own clock running underneath will drift. When following, step only on the ticks.
12. Do not extend the packet without a new marker (`0xB9`...). Old followers drop any length but 22 bytes of AD.
13. The sidechain assumes a 16 step bar. For another length, send kick mask 0.
14. A host simulator build must have stubs, or it will not link.
15. After you flash for the first time, check that the USB installer still reaches your firmware and that OCT- at power-on still gives the rescue. A running radio can hide both.

## 8. Done when

- The interop checklist in the spec passes against one real brother.
- The build output shows the unchanged loader line.
- A power cycle leaves Bluetooth Off.
- The new firmware's docs say how to turn it on (copy the words in section 4, "The menu").
