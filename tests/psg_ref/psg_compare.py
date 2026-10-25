#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Game Boy, SN76489 (SMS), T6W28 (NGP), HuC6280 (PCE), VSU (VB), WonderSwan, Mikey (Lynx): chips.c against libvgm's cores.
Same register values both sides, the core is boxed down to 44.1 kHz like the engine's 4x oversampling. Plain python (no numpy).
    REF_LIBVGM=/path/to/libvgm  (source, built with -DSNDEMU__ALL=ON into $REF_LIBVGM/b, static libs in b/bin)
    python3 tests/psg_ref/psg_compare.py [work_dir]
Prints per case: level (ours - core, dB) and the worst diff of harmonics above -30 dBerence (dB, relative to the fundamental)."""
import math, os, re, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
FW = HERE.parent.parent
WORK = Path(sys.argv[1]) if len(sys.argv) > 1 else FW / "build" / "psg_ref"
WORK.mkdir(parents=True, exist_ok=True)
LIBVGM = Path(os.environ["REF_LIBVGM"])
HDR = (FW / "firmware" / "gen" / "chip_tables.h").read_text()


def table(name):
    m = re.search(r"%s(?:\[\d+\])+ = \{(.*?)\};" % name, HDR, re.S)
    return [int(v) for v in re.findall(r"-?\d+", m.group(1))]


def cc(out, src, *extra):
    subprocess.run(["cc", "-O2", "-DPOOL=", "-D__attribute__(x)=", f"-I{FW}/firmware/src", f"-I{FW}/firmware/gen", *extra, "-o", str(WORK / out), str(HERE / src), "-lm"], check=True)


cc("ours", "psg_ours.c")
cc("an", "psg_an.c")
cc("drum", "psg_drum.c")
subprocess.run(["cc", "-O2", f"-I{LIBVGM}", f"-I{LIBVGM}/emu", "-o", str(WORK / "ref"), str(HERE / "psg_ref.c"), str(LIBVGM / "b/bin/libvgm-emu.a"), str(LIBVGM / "b/bin/libvgm-utils.a"), "-lm", "-lz", "-lc++"], check=True)


def f(n): return 440.0 * 2 ** ((n - 69) / 12)


def ref(dev, writes, secs, out, core="0"):
    s = "".join("%d %d %d %d\n" % w for w in writes)
    r = subprocess.run([str(WORK / "ref"), dev, core, str(secs), out], input=s, text=True, capture_output=True)
    if r.returncode: print("ref failed", r.stderr)


def ours(kind, note, secs, out, wave=0, duty=0, pan=1, vol=15, wf=0, wc=0, wt=0, analog=0, dump=False):
    r = subprocess.run([str(WORK / "ours"), str(kind), str(note), str(secs), out, str(wave), str(duty), str(pan), str(vol), str(wf), str(wc), str(wt), str(analog)],
                       env=dict(os.environ, **({"DUMPW": "1"} if dump else {})), capture_output=True, text=True, check=True)
    return [int(v) for v in r.stderr.split()] if dump else None


def an(path, f0, t0=0.3, t1=1.7, nh=8):
    o = subprocess.run([str(WORK / "an"), path, str(t0), str(t1), str(f0), str(nh)], capture_output=True, text=True).stdout.split("\n")
    rms = float(o[0].split()[1])
    h = [float(v) for v in o[3].split(":")[1].split()] if f0 > 0 else []
    return rms, h


def case(label, a_path, b_path, f0):
    ra, ha = an(a_path, f0); rb, hb = an(b_path, f0)
    d = [abs(x - y) for x, y in zip(ha, hb) if x > -30 and y > -30]
    print(f"{label:34s} level {ra - rb:+5.1f} dB   worst diff of harmonics above -30 dB {max(d) if d else 0:4.1f} dB")


O = str(WORK)
GB_PX, GB_WX, SMS_N, NGP_N, PCE_NT, VB_INC, WS_INC = (table(n) for n in ("GB_PX", "GB_WX", "SMS_N", "NGP_N", "PCE_NT", "VB_INC", "WS_INC"))
LYNX_MASK, LYNX_LEN = table("LYNX_MASK"), table("LYNX_LEN")

print("== Game Boy pulse, wave (SameBoy core)")
gb = lambda a, d: (0, 0, a, d)
on = [gb(0x16, 0x80), gb(0x14, 0x77), gb(0x15, 0xFF)]
for note in (48, 60, 84):
    for duty in range(4):
        x = GB_PX[note]
        ref("gb", on + [gb(1, duty << 6), gb(2, 0xF0), gb(3, x & 255), gb(4, 0x80 | x >> 8 & 7)], 2, f"{O}/r.f32", "SBOY")
        ours(2, note, 2, f"{O}/o.f32", duty=duty)
        case(f"pulse note {note} duty {duty}", f"{O}/o.f32", f"{O}/r.f32", 131072 / (2048 - x))
for wv in (0, 5, 10, 15):
    for vk, nr32 in ((15, 0x20), (11, 0x40), (7, 0x60)):
        x = GB_WX[60]
        tab = ours(3, 60, 2, f"{O}/o.f32", wave=wv, vol=vk, dump=True)
        ram = [gb(0x20 + i, tab[2 * i] << 4 | tab[2 * i + 1]) for i in range(16)]
        ref("gb", on + [gb(0x0A, 0)] + ram + [gb(0x0A, 0x80), gb(0x0C, nr32), gb(0x0D, x & 255), gb(0x0E, 0x80 | x >> 8 & 7)], 2, f"{O}/r.f32", "SBOY")
        case(f"wave {wv} volume {nr32 >> 5}", f"{O}/o.f32", f"{O}/r.f32", 65536 / (2048 - x))

print("== SN76489 (SMS) tone, volume steps")
sn = lambda d, c=0: (0, c, 0, d)
for note in (48, 60, 84):
    n = SMS_N[note]
    ref("sms", [sn(0x80 | n & 15), sn(n >> 4 & 63), sn(0x90)], 2, f"{O}/r.f32", "MAME"); ours(4, note, 2, f"{O}/o.f32")
    case(f"SMS tone note {note}", f"{O}/o.f32", f"{O}/r.f32", 3579545 / (32 * n))
n = SMS_N[60]
for att in (0, 5, 10, 14):
    ref("sms", [sn(0x80 | n & 15), sn(n >> 4 & 63), sn(0x90 | att)], 2, f"{O}/r.f32", "MAME")
    knob = next(k for k in range(16) if (15 * (k + 1)) >> 4 == 15 - att)
    ours(4, 60, 2, f"{O}/o.f32", vol=knob)
    case(f"SMS attenuation {att} (-{2 * att} dB)", f"{O}/o.f32", f"{O}/r.f32", 0)

print("== T6W28 (NGP): chip 0 = tone + left volumes, chip 1 = right volumes")
for note in (48, 60):
    n = NGP_N[note]
    for pan, (vl, vr) in ((1, (0, 0)), (0, (0, 15)), (2, (15, 0))):
        ref("ngp", [sn(0x80 | n & 15), sn(n >> 4 & 63), sn(0x90 | vl), sn(0x90 | vr, 1)], 2, f"{O}/r.f32", "MAME"); ours(61, note, 2, f"{O}/o.f32", pan=pan)
        case(f"NGP note {note} pan {pan}", f"{O}/o.f32", f"{O}/r.f32", 3072000 / (32 * n))

print("== HuC6280 (PCE): 5 bit wave, MAME core")
pw = lambda a, d: (0, 0, a, d)
for wv in (0, 5, 15):
    for note in (48, 84):
        tab = ours(5, note, 2, f"{O}/o.f32", wave=wv, dump=True); n = PCE_NT[note]
        w = [pw(0, 0), pw(1, 0xFF), pw(4, 0x40), pw(4, 0)] + [pw(6, s) for s in tab] + [pw(2, n & 255), pw(3, n >> 8), pw(5, 0xFF), pw(4, 0x9F)]
        ref("pce", w, 2, f"{O}/r.f32", "MAME")
        case(f"PCE wave {wv} note {note}", f"{O}/o.f32", f"{O}/r.f32", 3579545 / (32 * n))

print("== VSU (VB): 6 bit wave")
vw = lambda a, d: (0, 0, a, d)
for wv in (0, 5, 15):
    for note in (48, 84):
        tab = ours(11, note, 2, f"{O}/o.f32", wave=wv, dump=True)
        fin = VB_INC[note] * 176400 / 2 ** 32; x = 2048 - round(5000000 / 32 / fin)
        w = [vw(0x100, 0)] + [vw(i, tab[i]) for i in range(32)] + [vw(0x106, 0), vw(0x101, 0xFF), vw(0x102, x & 255), vw(0x103, x >> 8), vw(0x104, 0xF0), vw(0x105, 0), vw(0x100, 0x80)]
        ref("vsu", w, 2, f"{O}/r.f32")
        case(f"VB wave {wv} note {note}", f"{O}/o.f32", f"{O}/r.f32", 5000000 / (32 * (2048 - x)))

print("== WonderSwan: 4 bit wave (24 kHz mix)")
ww = lambda a, d: (0, 0, a, d)
for wv in (0, 5, 15):
    for note in (48, 84):
        tab = ours(44, note, 2, f"{O}/o.f32", wave=wv, dump=True)
        d = round(96000 / (WS_INC[note] * 176400 / 2 ** 32))
        w = [ww(0x10000 + 0x200 + i, tab[2 * i] | tab[2 * i + 1] << 4) for i in range(16)] + [ww(0x8F, 8), ww(0x80, (2048 - d) & 255), ww(0x81, (2048 - d) >> 8), ww(0x88, 0xFF), ww(0x90, 1), ww(0x91, 0x0F)]
        ref("ws", w, 2, f"{O}/r.f32")
        case(f"WS wave {wv} note {note}", f"{O}/o.f32", f"{O}/r.f32", 96000 / d)

print("== Mikey (Lynx): square and the LFSR tone sets (chip 0 regs 0x20 + 8 ch: vol fb out shift backup ctl count other)")
mk = lambda a, d: (0, 0, a, d)
for wv in range(0, 7):
    note = 60; per = 2 if wv == 0 else LYNX_LEN[wv]; mask = 1 if wv == 0 else LYNX_MASK[wv]
    clk, b = next((c, round(1e6 / (1 << c) / (f(note) * per)) - 1) for c in range(7) if 0 <= round(1e6 / (1 << c) / (f(note) * per)) - 1 <= 255)
    w = [mk(0x20, 127), mk(0x21, mask & 63 | mask >> 4 & 0xC0), mk(0x22, 0), mk(0x23, 0), mk(0x24, b), mk(0x26, 0), mk(0x25, 0x18 | clk | (0x80 if mask & 0x80 else 0)), mk(0x27, 0)]
    ref("mikey", w, 2, f"{O}/r.f32"); ours(52, note, 2, f"{O}/o.f32", wave=wv)
    case(f"Lynx sound {wv} (cycle {per})", f"{O}/o.f32", f"{O}/r.f32", 1e6 / (1 << clk) / (b + 1) / per)
