#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare chips.c with reference emulators on steady notes, same register values.
NES / Game Boy / SN76489: libvgm's vgm2wav (NSFPlay NES core, MAME-derived GB and SN76496 cores).
Needs numpy, build/host/chip_ref (see tests/chip_ref.c) and a built vgm2wav.
    REF_VGM2WAV=/path/to/vgm2wav python3 tests/ref_compare.py [out_dir]"""
import datetime, os, re, struct, subprocess, sys, wave
from pathlib import Path
import numpy as np

FW = Path(__file__).resolve().parent.parent
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else FW / "build" / "ref"
OUT.mkdir(parents=True, exist_ok=True)
VGM2WAV = os.environ["REF_VGM2WAV"]
CHIP_REF = FW / "build" / "host" / "chip_ref"
FS = 44100
HDR = (FW / "firmware" / "gen" / "chip_tables.h").read_text()


def table(name):
    m = re.search(r"%s(?:\[\d+\])+ = \{(.*?)\};" % name, HDR, re.S)
    return [int(v) for v in re.findall(r"-?\d+", m.group(1))]


NES_PT, NES_TT, GB_PX, GB_WX, SMS_N = (table(n) for n in ("NES_PT", "NES_TT", "GB_PX", "GB_WX", "SMS_N"))
GB_WAVES = [table("GB_WAVES")[i * 32:(i + 1) * 32] for i in range(4)]



# ---- reference renders
def vgm_file(path, cmds, secs, sn=False, gb=False, nes=False):
    data = bytearray(cmds)
    total = int(secs * FS)
    left = total
    while left > 0:
        w = min(left, 65535)
        data += b"\x61" + struct.pack("<H", w)
        left -= w
    data += b"\x66"
    h = bytearray(0x100)
    h[0:4] = b"Vgm "
    struct.pack_into("<I", h, 0x04, 0x100 + len(data) - 4)
    struct.pack_into("<I", h, 0x08, 0x161)
    if sn:
        struct.pack_into("<I", h, 0x0C, 3579545)
        struct.pack_into("<H", h, 0x28, 0x0009)
        h[0x2A] = 16
    struct.pack_into("<I", h, 0x18, total)
    struct.pack_into("<I", h, 0x34, 0x100 - 0x34)
    if gb: struct.pack_into("<I", h, 0x80, 4194304)
    if nes: struct.pack_into("<I", h, 0x84, 1789773)
    Path(path).write_bytes(bytes(h) + bytes(data))


def nes_w(a, d): return bytes([0xB4, a, d])
def gb_w(a, d): return bytes([0xB3, a, d])
def sn_w(d): return bytes([0x50, d])


def render_vgm(name, cmds, secs, **kw):
    v, w = OUT / f"{name}.vgm", OUT / f"{name}_ref.wav"
    vgm_file(v, cmds, secs, **kw)
    subprocess.run([VGM2WAV, str(v), str(w)], check=True, capture_output=True)
    return w


def render_ours(name, chip, ch, note, secs, par=0, pan=1):
    w = OUT / f"{name}_ours.wav"
    subprocess.run([str(CHIP_REF), str(chip), str(ch), str(note), str(secs), str(w), str(par), str(pan), "0"], check=True)   # 0: clean output stage
    return w


def load(path):                                  # through ffmpeg: vgm2wav writes WAVE_FORMAT_EXTENSIBLE
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(path), "-f", "f32le", "-ac", "2", "-ar", str(FS), "-"],
                         check=True, capture_output=True).stdout
    return (np.frombuffer(raw, dtype=np.float32).astype(float) * 32768).reshape(-1, 2), FS


# ---- measures
def mono_seg(path, t0=0.4, t1=1.3):
    a, fs = load(path)
    x = a.mean(axis=1)[int(t0 * fs):int(t1 * fs)]
    return x - x.mean(), fs


def harmonics(x, fs, f0, n=8):
    sp = np.abs(np.fft.rfft(x * np.hanning(len(x)))) / len(x)
    fr = np.fft.rfftfreq(len(x), 1 / fs)
    peaks = []
    for k in range(1, n + 1):
        sel = (fr > k * f0 * 0.985) & (fr < k * f0 * 1.015) & (fr < fs / 2 - 500)
        peaks.append(sp[sel].max() if sel.any() else 0.0)
    f_meas = fr[(fr > f0 * 0.9) & (fr < f0 * 1.1)][sp[(fr > f0 * 0.9) & (fr < f0 * 1.1)].argmax()]
    p = np.array(peaks)
    return f_meas, 20 * np.log10(np.maximum(p, 1e-9) / max(p[0], 1e-9))


def bands(x, fs):
    sp = np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    fr = np.fft.rfftfreq(len(x), 1 / fs)
    edges = [100, 200, 400, 800, 1600, 3200, 6400, 12800, 20000]
    e = np.array([sp[(fr >= a) & (fr < b)].sum() for a, b in zip(edges[:-1], edges[1:])])
    return 10 * np.log10(np.maximum(e, 1e-9) / e.sum())


def rms_db(x): return 20 * np.log10(max(x.std(), 1e-9) / 32768)


results = []


def tone_case(label, ours, ref, f0):
    xo, fs = mono_seg(ours)
    xr, _ = mono_seg(ref)
    fo, ho = harmonics(xo, fs, f0)
    fr_, hr = harmonics(xr, fs, f0)
    d = ho - hr
    mask = (ho > -50) & (hr > -50)
    worst = np.abs(d[mask]).max() if mask.any() else 0
    print(f"{label:28s} f0 exp {f0:8.2f} ours {fo:8.2f} ref {fr_:8.2f} | level ours {rms_db(xo):6.1f} ref {rms_db(xr):6.1f} dBFS "
          f"| harmonic diff max {worst:4.1f} dB")
    print("    ours " + " ".join(f"{v:6.1f}" for v in ho))
    print("    ref  " + " ".join(f"{v:6.1f}" for v in hr))
    results.append((label, rms_db(xo) - rms_db(xr), worst))





def hz_nes_p(t): return 1789773 / (16 * (t + 1))
def hz_nes_t(t): return 1789773 / (32 * (t + 1))


print("== NES (NSFPlay core)")
for note in (60, 84):
    for duty in range(4):
        t = NES_PT[note]
        c = nes_w(0x00, duty << 6 | 0x30 | 15) + nes_w(0x01, 0x00) + nes_w(0x02, t & 255) + nes_w(0x03, (t >> 8 & 7) | 0xF8) + nes_w(0x15, 0x01)
        n = f"nes_pulse_n{note}_d{duty}"
        tone_case(f"NES pulse note {note} duty {duty}", render_ours(n, 0, 0, note, 1.5, duty), render_vgm(n, c, 1.5, nes=True), hz_nes_p(t))
for note in (48, 60):
    t = NES_TT[note]
    c = nes_w(0x08, 0xFF) + nes_w(0x0A, t & 255) + nes_w(0x0B, (t >> 8 & 7) | 0xF8) + nes_w(0x15, 0x04)
    n = f"nes_tri_n{note}"
    tone_case(f"NES triangle note {note}", render_ours(n, 0, 2, note, 1.5), render_vgm(n, c, 1.5, nes=True), hz_nes_t(t))
print("\n== Game Boy (MAME-derived core)")
gb_on = gb_w(0x16, 0x80) + gb_w(0x14, 0x77) + gb_w(0x15, 0xFF)
for note in (60, 84):
    for duty in range(4):
        x = GB_PX[note]
        c = gb_on + gb_w(0x01, duty << 6) + gb_w(0x02, 0xF0) + gb_w(0x03, x & 255) + gb_w(0x04, 0x80 | (x >> 8 & 7))
        n = f"gb_pulse_n{note}_d{duty}"
        tone_case(f"GB pulse note {note} duty {duty}", render_ours(n, 1, 0, note, 1.5, duty), render_vgm(n, c, 1.5, gb=True), 131072 / (2048 - x))
for note in (60,):                               # pulse 2 is its own channel: check it makes sound too
    x = GB_PX[note]
    c = gb_on + gb_w(0x06, 2 << 6) + gb_w(0x07, 0xF0) + gb_w(0x08, x & 255) + gb_w(0x09, 0x80 | (x >> 8 & 7))
    n = f"gb_pulse2_n{note}"
    tone_case(f"GB pulse 2 note {note} duty 2", render_ours(n, 1, 1, note, 1.5, 2), render_vgm(n, c, 1.5, gb=True), 131072 / (2048 - x))
for wv in range(4):
    note = 60
    x = GB_WX[note]
    ram = b"".join(gb_w(0x20 + i, GB_WAVES[wv][2 * i] << 4 | GB_WAVES[wv][2 * i + 1]) for i in range(16))
    c = gb_on + gb_w(0x0A, 0x00) + ram + gb_w(0x0A, 0x80) + gb_w(0x0C, 0x20) + gb_w(0x0D, x & 255) + gb_w(0x0E, 0x80 | (x >> 8 & 7))
    n = f"gb_wave_w{wv}"
    tone_case(f"GB wave preset {wv} note {note}", render_ours(n, 1, 2, note, 1.5, wv), render_vgm(n, c, 1.5, gb=True), 65536 / (2048 - x))
print("\n== Master System (SN76489)")
for note in (60, 84):
    nn = SMS_N[note]
    c = sn_w(0x80 | (nn & 15)) + sn_w((nn >> 4) & 63) + sn_w(0x90)
    n = f"sms_tone_n{note}"
    tone_case(f"SMS tone note {note}", render_ours(n, 2, 0, note, 1.5), render_vgm(n, c, 1.5, sn=True), 3579545 / (32 * nn))
print("\n== summary (level = ours - ref in dB, spectrum = worst difference in dB)")
for label, lv, sp in results:
    print(f"{label:40s} level {lv:+6.1f}   spectrum {sp:5.1f}")
