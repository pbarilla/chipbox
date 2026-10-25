#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""AY-3-8910 and MSM5232 against libvgm's cores: pitch, volume steps, noise spectrum, tone / noise mixer, footage harmonics.
Plain Python (no numpy). Needs build/host/kind_ref (tests/kind_ref.c) and build/host/dev_ref (tests/dev_ref.c, links libvgm).
    python3 tests/arcade_compare.py"""
import array, math, os, re, subprocess, sys
from pathlib import Path
FW = Path(__file__).resolve().parent.parent
KIND_REF, DEV_REF = FW / "build" / "host" / "kind_ref", FW / "build" / "host" / "dev_ref"
WORK = FW / "build" / "arcade"
WORK.mkdir(parents=True, exist_ok=True)
FS = 44100
_src = (FW / "firmware" / "src" / "chips.c").read_text()
_names = re.findall(r"(KD_\w+)(?: = (KD_\w+) \+ (\d+))?", re.search(r"enum \{ KD_NES_PU.*?\};", _src, re.S).group(0))
KD, _n = {}, 0
for _nm, _b, _o in _names:
    if _nm == "KINDS": break
    if _b: _n = KD[_b] + int(_o)
    KD[_nm] = _n; _n += 1
def ours(kind, note, secs=1.0, **kw):
    out = str(WORK / 'ours.raw')
    args = [str(KIND_REF), str(kind), str(note), str(secs), out] + ['%s=%s' % kv for kv in kw.items()]
    subprocess.run(args, check=True)
    return load(out, 0)
def ref(dev, core, typ, clock, rate, script, secs, ch=0):
    sp = str(WORK / 'script.txt'); out = str(WORK / 'ref.raw')
    open(sp, 'w').write(script)
    r = subprocess.run([str(DEV_REF), dev, core, str(typ), str(clock), str(rate), sp, out, str(secs)], check=True, capture_output=True)
    global REFRATE
    REFRATE = int(r.stderr.decode().split()[1])
    return load(out, ch)
REFRATE = 44100
def load(p, ch=0):
    a = array.array('i'); a.fromfile(open(p, 'rb'), os.path.getsize(p) // 4)
    return [float(v) for v in a[ch::2]]
def goertzel(x, f, fs=FS):
    n = len(x); w = 2 * math.pi * f / fs; c = 2 * math.cos(w); s1 = s2 = 0.0
    for i in range(n):
        h = 0.5 - 0.5 * math.cos(2 * math.pi * i / (n - 1))
        s0 = x[i] * h + c * s1 - s2; s2 = s1; s1 = s0
    return math.sqrt(max(0, s1 * s1 + s2 * s2 - c * s1 * s2)) * 2 / (n * 0.5)
def estf(x, fs=FS):
    # zero crossing frequency estimate on mean-removed signal (rising crossings, interpolated)
    m = sum(x) / len(x); y = [v - m for v in x]
    cr = []
    for i in range(1, len(y)):
        if y[i - 1] < 0 <= y[i]: cr.append(i - 1 + (-y[i - 1]) / (y[i] - y[i - 1]))
    if len(cr) < 3: return 0
    return (len(cr) - 1) * fs / (cr[-1] - cr[0])
def refine(x, f0, fs=FS):
    best, bf = -1, f0
    for k in range(-40, 41):
        f = f0 * (1 + k * 0.0004)
        a = goertzel(x, f, fs)
        if a > best: best, bf = a, f
    return bf
def harm(x, f0, n=12, fs=FS):
    f0 = refine(x, f0, fs)
    a1 = goertzel(x, f0, fs)
    return f0, [goertzel(x, f0 * k, fs) for k in range(1, n + 1)]
def db(v): return 20 * math.log10(v) if v > 0 else -200
def rms(x):
    m = sum(x) / len(x); return math.sqrt(sum((v - m) ** 2 for v in x) / len(x))
def seg(x, t0, t1, fs=FS): return x[int(t0 * fs):int(t1 * fs)]
def fft(a):
    n = len(a)
    if n == 1: return a
    e = fft(a[0::2]); o = fft(a[1::2])
    r = [0] * n
    for k in range(n // 2):
        t = complex(math.cos(-2 * math.pi * k / n), math.sin(-2 * math.pi * k / n)) * o[k]
        r[k] = e[k] + t; r[k + n // 2] = e[k] - t
    return r
def psd_bands(x, fs, edges, nfft=16384):
    # average power spectrum (Hann, 50% overlap Welch) summed into bands [edges[i], edges[i+1]); returns dB power
    acc = [0.0] * (nfft // 2); cnt = 0
    win = [0.5 - 0.5 * math.cos(2 * math.pi * i / nfft) for i in range(nfft)]
    for s in range(0, len(x) - nfft, nfft // 2):
        seg_ = [x[s + i] * win[i] for i in range(nfft)]
        F = fft(seg_)
        for k in range(nfft // 2): acc[k] += abs(F[k]) ** 2
        cnt += 1
    out = []
    for i in range(len(edges) - 1):
        k0 = int(edges[i] * nfft / fs); k1 = max(k0 + 1, int(edges[i + 1] * nfft / fs))
        out.append(sum(acc[k0:k1]) / cnt)
    return out
OCT = [100, 200, 400, 800, 1600, 3200, 6400, 12800, 20000]
def bands_db(x, fs, edges=OCT):
    p = psd_bands(x, fs, edges); tot = sum(p)
    return [10 * math.log10(v / tot) if v > 0 else -200 for v in p], 10 * math.log10(tot)

CLK = 1773447
def ayscript(P, vol=15, mix=0x3E, N=10): return '0 7 %d\n0 6 %d\n0 0 %d\n0 1 %d\n0 8 %d\n' % (mix, N, P & 255, P >> 8, vol)
def ay_pitch():
    print('AY pitch: note, period, chip Hz (emu2149, MAME), ours Hz')
    for note in [24, 48, 72, 96, 108]:
        P = max(1, min(4095, round(CLK / 16 / (440 * 2 ** ((note - 69) / 12)))))
        a = estf(seg(ref('ay', 'emu', 16, CLK, FS, ayscript(P), 0.8), .2, .8))
        x = ref('ay', 'mame', 0, CLK, 0, ayscript(P), 0.8); fs = REFRATE
        print(note, P, round(a, 2), round(estf(seg(x, .2, .8, fs), fs), 2), round(estf(seg(ours(KD['KD_AY'], note, 0.8), .2, .8)), 2))
def ay_steps():
    print('AY volume steps, dB below step 15: MAME AY8910 (1k load), MAME YM2149, ours')
    r = []
    for v in range(1, 16):
        x = ref('ay', 'mame', 0, CLK, 0, ayscript(424, v), 0.5); r.append(rms(seg(x, .1, .5, REFRATE)))
    y = []
    for v in range(1, 16):
        x = ref('ay', 'mame', 16, CLK, 0, ayscript(424, v), 0.5); y.append(rms(seg(x, .1, .5, REFRATE)))
    o = [rms(seg(ours(KD['KD_AY'], 60, 0.5, vol=v), .1, .5)) for v in range(1, 16)]
    for v in range(15): print(v + 1, '%6.1f %6.1f %6.1f' % (db(r[v] / r[14]), db(y[v] / y[14]), db(o[v] / o[14])))
def ay_noise():
    print('AY noise, octave band shares dB (100 Hz .. 20 kHz): chip then ours')
    for N in [4, 10, 31]:
        x = ref('ay', 'mame', 0, CLK, 0, ayscript(1, 15, 0x37, N), 1.6); fs = REFRATE
        b, _ = bands_db(seg(x, .3, 1.6, fs), fs)
        note = round(12 * math.log2(CLK / 256 / N / 440) + 69)
        bo, _ = bands_db(seg(ours(KD['KD_AY'], note, 1.6, wave=15), .3, 1.6), FS)
        print('N', N, 'note', note); print('  ', ' '.join('%6.1f' % v for v in b)); print('  ', ' '.join('%6.1f' % v for v in bo))
def msm_harmonics():
    CK = 2119040
    print('MSM5232 footage harmonics dB (chip then ours) for footage masks 16 8 4 2 = bits 0..3')
    N = 60; f8 = 440 * 2 ** ((N - 69) / 12)
    for mask in [3, 6, 10, 15]:
        x = ref('msm', 'x', 1e-6, CK, 0, '0 8 0\n0 10 0\n0 12 %d\n0 0 %d\n' % (0x10 | mask, 0x80 | (N - 36)), 0.8); fs = REFRATE
        fb = f8 / 2 * (1 if mask & 1 else 2 if mask & 2 else 4 if mask & 4 else 8)
        fb, h = harm(seg(x, .3, .8, fs), fb, 12, fs)
        fo, ho = harm(seg(ours(KD['KD_MSM'], N, 0.8, wave=mask, pan=0), .3, .8), fb, 12)
        print('mask', mask); print('  ', ' '.join('%6.1f' % db(a / max(h)) for a in h)); print('  ', ' '.join('%6.1f' % db(a / max(ho)) for a in ho))
if __name__ == '__main__':
    for f in (ay_pitch, ay_steps, ay_noise, msm_harmonics): f(); print()
