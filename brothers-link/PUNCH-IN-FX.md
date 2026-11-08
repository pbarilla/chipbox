# The eight shared punch-in effects

(Drumlab shows this layout only while Brothers Link is on; with it off its 16 pads are the PO-32's own effects.) Pads 1 to 8 of the FX layer are the same eight moves on Chipbox, Smileyface and Drumlab. They are the ones Brothers Link carries: hold a pad on the leader and every follower holds it too. This page says what each one does, how it feels, and how to build it in another firmware.

Chipbox runs its own engine (`firmware/src/fx.c`, stereo). Smileyface and Drumlab share one file, `firmware/src/bfx.c`, a mono port of Chipbox's master chain. The numbers below are read from those two files.

| Pad | Name | Bit n | Mask | Chipbox calls it | Smileyface and Drumlab call it |
|---|---|---|---|---|---|
| 1 | STOP | 0 | `0x01` | TAPE | STOP |
| 2 | REPEAT | 1 | `0x02` | RPT | REPEAT |
| 3 | LPF | 2 | `0x04` | LPF | LPF |
| 4 | HPF | 3 | `0x08` | HPF | HPF |
| 5 | ECHO | 4 | `0x10` | ECHO | ECHO |
| 6 | REVERB | 5 | `0x20` | VERB | REVERB |
| 7 | CRUSH | 6 | `0x40` | CRSH | CRUSH |
| 8 | GATE | 7 | `0x80` | GATE | GATE |

Bit n of the 16 bit fx mask in the link packet (`p[12..13]`) is pad n+1. Pad 16 is the LATCH. Pads 9 to 15 are each firmware's own and never travel.

## Why it's rad

- All your FM-1s share one vocabulary of eight moves. Learn it once.
- Hold a pad on the leader and every box in the room does the same thing at the same instant. Well, within a few tens of milliseconds, see "Over the link".
- The tape stop and the filters work on any firmware that implements them. Yours can too: the recipes are below.
- Three different machines start to feel like one instrument.

## The rules of this platform

- No `float` or `double`, no 64 bit division. Levels are Q15, gains Q8 or Q12, filter coefficients Q14. Tables are made on the host (Python) and pasted as `uint16_t` arrays.
- It all runs inside the audio interrupt: blocks of 256 frames at 44.1 kHz, one every 5.8 ms. No flash, no screen, no waiting. The block must finish in time, so an effect only runs while it is on or still ringing.
- Big buffers do not fit in the 96 KB of globals. Put them in the pool section (`BFX_POOL` in `bfx.c`, `POOL` in `fx.c`). All eight together: about 64 KB in `bfx.c`, about 103 KB in Chipbox (stereo echo).
- Ramps and tails run at 60 Hz (`bfx_tick`, 735 samples), per sample work runs in the block.
- Effects are in one chain. Order, the same in both engines: repeat, gate, crush, high-pass, low-pass, echo, tape level, reverb. Reverb is last, so it also rings through a tape stop.
- I did not measure what the effects cost. Each is a few multiplies per sample.

## The layer

- Chipbox and Smileyface: press FX to open the layer, press it again to leave. Drumlab: hold FX, or tap it to leave the layer open.
- The 16 pads are the 16 white keys from F3 up (key numbers `0 2 4 6 7 9 11 12 14 16 18 19 21 23 24 26`). Pad 1 is F3, pad 8 is F4, pad 16 is G5.
- A pad is on while its key is down. Pad 16 (LATCH) switches latch mode: with it on, a tap turns a pad on and the next tap turns it off.
- Switching on and off never jumps. The gate and the tape level ramp, the filters step their cutoff at 60 Hz, echo and reverb stop taking input and ring out. Details per effect.
- Nothing is applied from the key handler. The main loop turns keys into a mask, the audio side reads the mask: one writer on each side.

```c
/* bfx.c: the host sets the mask, calls these */
bfx_mask = pads & 0xFFu;                      /* bit n = pad n+1 */
bfx_tick();                                   /* every 735 samples (60 Hz) */
if (bfx_busy() || bfx_mask) bfx_run(out, n, 661500u / bpm);   /* the last argument: samples in a sixteenth */
else bfx_rec(out, n);                                          /* nothing is on: keep the dry mix, REPEAT loops the last sixteenth of it */
```

`out` is the stereo mix (left read, both written). A mono firmware just uses one channel.

## 1. STOP (tape stop)

**Sounds like:** the tape slows to a halt. The music dives two octaves down and fades out. Let go and it speeds back up.
**Feels like:** a brake pedal. A short tap is a little hiccup, a long hold is a dead stop.

**Build it:** a `speed` that runs 256 (normal) to 0, moved at 60 Hz. It drives two things: the voices' pitch and the master level.

```c
int32_t tgt = m & BFX_STOP ? 0 : 256;
if (bfx_speed > tgt) bfx_speed = tgt + (bfx_speed - tgt) * 13 / 16;     /* down: 13/16 of the gap a tick */
else if (bfx_speed < tgt) bfx_speed += (tgt - bfx_speed) / 3 + 4;      /* up: a third of the gap, plus 4 */
if (bfx_speed < 4 && tgt == 0) bfx_speed = 0;
bfx_pitch = bfx_speed < 256 ? -(256 - bfx_speed) * 24 : 0;             /* 1/256 semitone: -6144 at a halt = 2 octaves */
/* per sample */ tg = bfx_speed >= 256 ? 256 : bfx_speed * bfx_speed >> 8;   v = v * tg >> 8;
```

- Stop takes 19 ticks, 0.32 s. Start takes 8 ticks, 0.13 s (counted from these numbers).
- The level falls with the square of the speed, so the end is quiet before the pitch is gone.
- Add `bfx_pitch` to every voice's pitch (Q8 semitones). Smileyface adds it to the note pitch. Drumlab turns it into a ratio with `exp2fx(bfx_pitch * 5461, 24, 20)` and multiplies its sample step.
- Chipbox does more: a TAPE press also pauses the sequencer and sends note-offs, and SELECT becomes a tape dial (speed 0, 128, 256 or 512 while held; below 256 the pitch is `-(256 - speed) * 24`, above it `(speed - 256) * 12`, so half speed is an octave down and double speed an octave up). Letting go puts the dial back. Smileyface and Drumlab keep the sequencer running: only pitch and level move.

## 2. REPEAT (beat repeat)

**Sounds like:** the last sixteenth note, looped, for as long as you hold.
**Feels like:** a stutter you can aim. Hold for a roll, tap for a flam.

**Build it, audio ring (Smileyface, Drumlab):** write the dry mix into a ring, and when the pad goes down, loop the last sixteenth from it.

```c
bfx_rb[bfx_rbw++ & (BFX_RB - 1u)] = v;                                    /* the dry mix, 8192 frames */
if ((m ^ bfx_prev) & m & BFX_REP) { bfx_rl = step; bfx_rs = bfx_rbw - step; bfx_ri = 0; }   /* pad just went down */
if (m & BFX_REP) { v = bfx_rb[(bfx_rs + bfx_ri) & (BFX_RB - 1u)]; if (++bfx_ri >= bfx_rl) bfx_ri = 0; }
```

- `step` is `661500 / bpm` samples (5512 at 120 BPM). The ring holds 8190 at most, so below about 81 BPM the repeat is shorter than a sixteenth.
- There is no crossfade at the loop point and none at the edges.
- The repeat loops the last sixteenth of the dry mix, so the ring has to be written every block, even with every pad up (`bfx_rec` does that when `bfx_run` is not called). Skip it and the first repeat after a quiet spell loops old or silent audio.

**Build it, sequencer (Chipbox):** Chipbox does not loop audio. It re-fires the step that was playing when the pad went down. The pad sets `fx_rpt` to a repeat length in ticks: 12 (eighth, the R8 pad), 6 (sixteenth, pad 2), 3 (thirty-second, R32). Every `fx_rpt` ticks the sequencer plays that step again, with the gate cut to fit (`gate < fx_rpt ? gate : fx_rpt - 1`). The normal step advance is held while `fx_rpt` is non-zero, and it only starts if the sequencer is running and not paused.

## 3. LPF and 4. HPF

**Sounds like:** LPF closes a curtain: the highs go, the mix gets dull and far away. HPF thins everything to a hiss: the bass leaves first, then the body.
**Feels like:** holding sweeps, it does not snap. Let go and it opens again fast.

**Build it:** a 6 dB one-pole filter per side and a 16 entry table of coefficients, index 0 is off.

```c
static const uint16_t BFX_LP_A[16] = {16384, 9754, 8393, 7118, 5961, 4942, 4062, 3316, 2692, 2176, 1752, 1406, 1127, 901, 719, 573};
static const uint16_t BFX_HP_A[16] = {0, 124, 165, 220, 293, 389, 517, 685, 908, 1199, 1579, 2071, 2702, 3500, 4493, 5699};
if (a_hp) { lp1 += ((v - lp1) * a_hp) >> 14; v -= lp1; }                /* high-pass = input minus a low-pass */
if (a_lp < 16384) { lp2 += ((v - lp2) * a_lp) >> 14; v = lp2; }         /* 16384 = open, skipped */
```

- The tables come from `round(16384 * (1 - exp(-2 pi f / 44100)))` (`tools/gen_chip_tables.py`). LPF cutoff goes 8 kHz down to 250 Hz in 15 steps, spaced evenly in octaves (index 1 about 6.3 kHz, 8 about 1.3 kHz, 15 = 250 Hz). HPF goes 40 Hz up to 3 kHz the same way (index 1 about 53 Hz, 8 about 400 Hz, 15 = 3 kHz).
- Held: the index goes up by one every third tick (20 steps a second), so a full sweep takes 0.75 s. Released: down by two a tick, so it is open again in 8 ticks, 0.13 s.
- The index is read once per block, so the coefficient moves in steps of 5.8 ms or less. One state per channel (Chipbox has two channels, `fx_lp1[2]`, `fx_lp2[2]`). State is zeroed when nothing is on.
- No resonance. Add some and it is no longer this effect.

## 5. ECHO

**Sounds like:** the mix repeats, quieter each time, two sixteenths apart (an eighth note), and rings on after you let go.
**Feels like:** you throw a handful of what is playing into a hall of mirrors and it keeps going by itself for a couple of seconds.

**Build it:** a delay line with feedback, in the tempo.

```c
int32_t d = bfx_ec[bfx_pos], w = (m & BFX_ECHO) ? v : 0, nb = w + (d * 140 >> 8);
bfx_ec[bfx_pos] = clip16(nb);                                           /* feedback 140/256 = 0.55 */
v += d * 150 >> 8;                                                      /* wet 150/256 = 0.59 */
if (++bfx_pos >= echo_d) bfx_pos = 0;                                   /* echo_d = 2 * step samples */
```

- Delay `2 * 661500 / bpm` samples (11025 at 120 BPM = 250 ms). `bfx.c` has 16384 frames (0.37 s) so below about 81 BPM it is cut short. Chipbox clamps its tempo to 40..240 and has 22050 frames (0.5 s), so it is cut short below 60 BPM and has a stereo line.
- Input only while the pad is held. After release the line keeps running: `echo_tail` is set to 150 ticks (2.5 s) while held and counts down, then the audio stops. Each repeat is about 5 dB quieter.
- Once the tail is over the line is wiped, 2048 frames per block, so the next echo starts clean. That is a few memsets over a handful of blocks, not one big stall.
- The echo sits after the filters (you can filter the echo input with LPF) and before the tape level.

## 6. REVERB

**Sounds like:** a room. Dark, about a second and a half of tail, and it keeps ringing after you let go.
**Feels like:** a splash. Tap it on a snare, hold it on a pad.

**Build it:** four comb filters with a low-pass in each loop, in parallel, into two allpasses in series. Mono in, the same signal added to both sides.

```c
static const uint16_t BFX_VB_LEN[4] = {1116, 1277, 1422, 1617};         /* comb delays, samples (25 to 37 ms) */
static const uint16_t BFX_AP_LEN[2] = {225, 556};                        /* allpass delays (5 and 12.6 ms) */
/* per comb k */  d = comb[k][pos[k]];  lp[k] += (d - lp[k]) >> 2;       /* the highs die first */
                  w = in + (lp[k] * 222 >> 8);  comb[k][pos[k]] = clip16(w);  sum += d;     /* feedback 222/256 = 0.87 */
/* per allpass */ d = ap[k][pos]; w = sum + (d >> 1); ap[k][pos] = clip16(w); sum = d - (w >> 1);
v += sum * 3 >> 1;                                                       /* the wet, 1.5x */
```

- Input: `v >> 2` in `bfx.c`; Chipbox sums left and right and does `>> 3`. Input is zero when the pad is up.
- The tail: `verb_tail` is 200 ticks (3.3 s) while held and counts down. The comb loop gain gives roughly 1.5 s to -60 dB (from the numbers: 0.87 per pass, 30 ms a pass), shorter in the highs. When the timer ends the buffers are wiped in one go.
- Buffers: about 15 KB of `int16`.
- Keep the 1116, 1277, 1422, 1617 lengths: they are not multiples of each other, which is what stops it ringing like a pipe.

## 7. CRUSH

**Sounds like:** grit. The sound goes crunchy and aliased, like an old sampler.
**Feels like:** a switch, not a sweep. On, off.

**Build it:** sample and hold plus bit reduction.

```c
if (++hold_n >= 3) { hold_n = 0; held = v; }        /* keep every third sample: 14.7 kHz */
v = (held >> 9) * 512;                              /* drop the low 9 bits: 7 bits are left */
```

- Chipbox's DECI (a pad of its own, not shared) is the same hold with 8 instead of 3 and no bit drop.
- It comes after the gate and before the filters, so LPF takes the edge off the crush.
- There is no ramp: it goes on in one sample. If that clicks on your voices, ramp a wet/dry mix.

## 8. GATE

**Sounds like:** the sound chopped into sixteenths: on for half, off for half.
**Feels like:** a trance gate you switch on with a finger.

**Build it:** a counter that runs one sixteenth long, and a gain that is 0 in its second half.

```c
if (++gp >= step) gp = 0;                           /* step = 661500 / bpm samples */
if (gp >= step / 2u) target = 0;                    /* else 256 */
g += target > g ? (target - g > 4 ? 4 : target - g) : (g - target > 4 ? -4 : target - g);    /* 4/256 a sample: 64 samples, 1.5 ms */
v = v * g >> 8;
```

- The 1.5 ms ramp is what keeps it from clicking.
- The counter is free running: it counts only while the gate is held and does not reset on a press. It is the right length but not locked to the step grid. Lock it if you need to.
- Chipbox clamps its tempo to 40..240 for this. `bfx_run` takes the step length from the host (Smileyface its BPM, Drumlab its step length), with 126 BPM if the BPM is 0.
- The gain is shared with Chipbox's KILL pad (target 0, same ramp).

## Over the link

- Leader: every pad that is on (held or latched) is bit n of the mask. It hands the low 8 bits to the link from the main loop. `brothers_lead_set(pat, kick, pads & 0xFFu, key, swing)`.
- Follower: `brothers_fx() & 0xFFu` is the leader's pads. Treat each bit as that pad being held. Nothing else travels: no timing, no strength.

```c
/* Smileyface, main loop; Drumlab does it in the audio block */
bfx_mask = (my_pads & 0xFFu) | (brothers_mode == 2u && bt_fxsync ? brothers_fx() & 0xFFu : 0u);
```

- Only while following, and only with the Punch-in FX switch on. It is off until the user turns it on. Your own pads count too: the two are ORed, so a follower can add a pad of its own (Chipbox too, while its layer is open).
- The level is the whole story: the mask is a snapshot, not an event. The follower presses what appeared and releases what went away, and ramps and tails run on the follower exactly as for a local pad.
- A pad that arrives sounds like a local press. Chipbox's follower really presses TAPE and RPT: the tape pad pauses its sequencer and the repeat steers it. How that sits with a running Bluetooth clock: unverified.
- Leader stops: the mask keeps travelling while stopped. Pads 9 to 15 and pad 16 are never sent.
- Leader lost: `brothers_fx()` is 0 once the lock is dropped, 1.5 s after the last packet. Until then the last mask stays held, so a pad can hang for 1.5 s. Then every pad is let go, ramps and tails as normal. Sync set to Off, or the Punch-in FX switch turned off: let go at once.
- How late is a follower? A packet is written right after each advertising event, and goes on air at the next one: 20 to 30 ms later. So a press can wait from about 20 up to about 50 ms before it is on air, then 0 to 2.5 ms for the channel, then up to your poll time (a main loop pass on Chipbox and Smileyface, one block, 5.8 ms, on Drumlab). Call it 40 ms in the middle, 60 ms at worst. A lost packet adds another 20 to 30 ms. Read from the code. Not measured on air.
- The fx mask is not timestamped and not phase corrected, unlike the clock. A pad tapped for less than one advertising interval may never go out.
- The leader's own sound is immediate, so the followers always come in a little after it.

## Not verified

- Any of the timing above on air.
- The CPU cost of each effect.
