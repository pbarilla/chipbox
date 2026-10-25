#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Writes the sample projects of the website's Projects page (site/projects/samples/*.chipbox and samples.json) straight in the project format
(firmware/src/proj.c proj_pack, version 3), so no device is needed. Every tune is original or public domain.
    python3 tools/make_samples.py [name ...]     (no names: all of the new ones)"""
import base64, json, sys
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "site" / "projects"
K = dict(NES_PU=0, NES_TRI=1, GB_PU=2, GB_WAV=3, SMS=4, PCE=5, TIA=6, VRC6_PU=7, VRC6_SAW=8, AY=9, MD=10, VB=11,
         POKEY=21, SPK=22, OPL=23, WSG=24, N163=25, S5B=26)
for i, n in enumerate("NES GB SMS PCE TIA VRC6 AY MD VB".split()): K["DRUMS_" + n] = 12 + i
for i, n in enumerate("POKEY SPK OPL WSG N163 S5B".split()): K["DRUMS_" + n] = 27 + i
K.update(MMC5=33, FDS=34, SCC=35, OPLL=36, VRC7=41, OPM=42, OPL3=43, WS=44)
for i, n in enumerate("MMC5 FDS SCC OPLL".split()): K["DRUMS_" + n] = 37 + i
for i, n in enumerate("VRC7 OPM OPL3 WS".split()): K["DRUMS_" + n] = 45 + i
K.update(SAA=49, VIC=50, TED=51, LYNX=52)
for i, n in enumerate("SAA VIC TED LYNX".split()): K["DRUMS_" + n] = 53 + i
K.update(PC98_FM=57, PC98_SSG=58, NEO_FM=59, NEO_SSG=60, NGP=61, OPL1=62, OPZ=63, SN77=64, C30=65)
for i, n in enumerate("PC98 NEO NGP OPL1 OPZ SN77 C30".split()): K["DRUMS_" + n] = 66 + i
K.update(MSM=73, ZX=74)
for i, n in enumerate("MSM ZX".split()): K["DRUMS_" + n] = 75 + i
K.update(OPN_FM=77, OPN_SSG=78, DRUMS_OPN=79, SID=80, DRUMS_SID=81)
# wave, duty, atk, dec, sus, rel, vib_spd, vib_dep, sweep, pan  (the firmware's INST_DEF, chips.c)
D = {"NES_PU": (0, 2, 0, 5, 9, 4, 5, 0, 0, 1), "NES_TRI": (0, 0, 0, 0, 15, 2, 5, 0, 0, 1), "GB_PU": (0, 2, 0, 5, 9, 4, 5, 0, 0, 1),
     "GB_WAV": (15, 0, 0, 0, 15, 2, 5, 0, 0, 1), "SMS": (0, 0, 0, 6, 14, 4, 5, 4, 0, 1), "PCE": (5, 0, 0, 0, 12, 2, 5, 0, 0, 1),
     "TIA": (5, 0, 0, 4, 12, 3, 5, 0, 0, 1), "VRC6_PU": (0, 3, 0, 5, 10, 4, 5, 0, 0, 1), "VRC6_SAW": (0, 0, 0, 0, 15, 2, 5, 0, 0, 1),
     "AY": (0, 0, 0, 5, 12, 3, 5, 0, 0, 1), "VB": (10, 0, 0, 0, 15, 2, 5, 0, 0, 1), "POKEY": (1, 0, 0, 4, 12, 3, 5, 0, 0, 1),
     "SPK": (0, 0, 0, 0, 15, 1, 5, 0, 0, 1), "WSG": (5, 0, 0, 0, 12, 2, 5, 0, 0, 1), "N163": (10, 0, 0, 0, 15, 2, 5, 0, 0, 1),
     "S5B": (0, 0, 0, 5, 12, 3, 5, 0, 0, 0)}
DRUM = (0, 0, 0, 4, 15, 4, 5, 0, 0, 1)
FM = dict(wave=1, duty=4, wf=9, wc=0, wt=4, atk=0, dec=6, sus=12, rel=3, vib_spd=5, vib_dep=0, sweep=0, pan=1)
OPL = dict(FM, duty=0, wf=10, wc=0, wt=3)
WFMAP = [0, 4, 6, 7, 8, 9, 9, 10, 10, 11, 11, 11, 12, 12, 12, 12]       # the INDEX knob was linear, now 3 dB a step: the same depth on the new scale
MDALG = {0: 4, 1: 0, 2: 2, 3: 7}                  # the Mega Drive had 3 operators and 4 algorithms, now 4 and 8
NAMES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def midi(tok):
    n = NAMES[tok[0]]
    i = 1
    if tok[i] == "#": n += 1; i += 1
    elif tok[i] == "b": n -= 1; i += 1
    return n + (int(tok[i:]) + 1) * 12


def N(s):                                       # 16 steps: names, dots for rests
    t = s.split()
    assert len(t) == 16, (len(t), s)
    return [0 if x == "." else midi(x) for x in t]


class Project:
    def __init__(self, bpm, pages=4):
        self.bpm, self.pages, self.ch, self.ev, self.song = bpm, pages, [], {}, []

    def add(self, kind, **kw):
        fields = dict(zip("wave duty atk dec sus rel vib_spd vib_dep sweep pan".split(), DRUM if kind.startswith("DRUMS") else D.get(kind, (0,) * 10)))
        if kind == "MD": fields = dict(FM)
        if kind == "OPL": fields = dict(OPL)
        fields.update(dict(wf=0, wc=0, wt=0, vol=15, arp=0, arp_rate=1, arp_dir=0)); fields.update({k: v for k, v in fields.items() if k in ("wf", "wc", "wt")})
        if kind == "MD": fields.update(wf=9, wc=0, wt=4)
        if kind == "OPL": fields.update(wf=10, wc=0, wt=3)
        if kind in ("MD", "OPL") and "wf" in kw: kw = dict(kw, wf=WFMAP[kw["wf"]])
        if kind == "MD" and "duty" in kw: kw = dict(kw, duty=MDALG[kw["duty"]])
        fields.update(kw)
        self.ch.append((K[kind], fields))
        return len(self.ch) - 1

    def notes(self, page, c, steps, gate, vel=110):
        for s, n in enumerate(steps):
            if n: self.ev[(page, c, s)] = (n, vel, gate)

    def drum(self, page, c, lane, pat):         # x a hit, X an accent
        for s, ch in enumerate(pat):
            if ch in "xX":
                n, v, g = self.ev.get((page, c, s), (0, 0, 0))
                hits = n | (v & 7) << 8 | 1 << lane
                acc = g | (v >> 3 & 7) << 8 | ((1 << lane) if ch == "X" else 0)
                self.ev[(page, c, s)] = (hits & 255, (hits >> 8 & 7) | (acc >> 8 & 7) << 3, acc & 255)

    def bytes(self):
        o = bytearray([3, self.bpm & 255, self.bpm >> 8, self.pages, len(self.ch)])
        for kind, f in self.ch:
            o.append(kind)
            for k in "wave duty atk dec sus rel vib_spd vib_dep sweep pan vol arp arp_rate arp_dir wf wc wt".split(): o.append(f[k] & 255)
        ev = sorted(self.ev.items())
        o += bytes([len(ev) & 255, len(ev) >> 8])
        for (p, c, s), (n, v, g) in ev: o += bytes([p, c, s, n, v, g])
        o.append(len(self.song)); o += bytes(self.song)
        return bytes(o)


# ---- little arrangers
def tone(chord, d):                             # a chord's d-th tone upward: 0 1 2 root third fifth, 3 4 5 the next octave
    return chord[d % 3] + 12 * (d // 3)


SHAPES = [[(0, 3), (2, 4), (4, 5), (6, 4), (8, 3), (10, 4), (12, 5), (14, 6)],
          [(0, 5), (3, 4), (6, 3), (8, 4), (11, 3), (14, 2)],
          [(0, 3), (4, 3), (6, 4), (8, 5), (12, 5), (14, 4)],
          [(0, 6), (4, 5), (8, 4), (10, 5), (12, 6), (14, 7)]]


def lead(chord, shape, shift=0, off=0):
    r = [0] * 16
    for s, d in SHAPES[shape % 4]: r[s] = tone(chord, d) + shift
    return r


def arp(chord, pat=(0, 1, 2, 4), shift=0):
    return [tone(chord, pat[i % len(pat)]) + shift for i in range(16)]


def bass(root, style):
    r = [0] * 16
    if style == "oct":
        for i in range(8): r[i * 2] = root + (12 if i % 2 else 0)
    elif style == "quarter":
        for i in range(4): r[i * 4] = root
    elif style == "off":
        for i in range(4): r[i * 4 + 2] = root
        r[0] = root
    elif style == "sync":
        for s in (0, 3, 6, 8, 11, 14): r[s] = root
        r[14] = root + 7
    elif style == "walk":
        for i, d in enumerate((0, 0, 7, 0, 0, 12, 7, 5)): r[i * 2] = root + d
    elif style == "eighth":
        for i in range(8): r[i * 2] = root
    return r


def drums(p, page, c, style):
    if style == "rock":
        p.drum(page, c, 0, "X.....x.x....x.."); p.drum(page, c, 1, "....X.......X..."); p.drum(page, c, 7, "x.x.x.x.x.x.x.x.")
    elif style == "four":
        p.drum(page, c, 0, "x...x...x...x..."); p.drum(page, c, 1, "....x.......x..."); p.drum(page, c, 7, ".x.x.x.x.x.x.x.x"); p.drum(page, c, 8, "......x.......x.")
    elif style == "break":
        p.drum(page, c, 0, "X..x..x.X.x....."); p.drum(page, c, 1, "....X..x.x..X..."); p.drum(page, c, 7, "x.x.x.x.x.x.x.xx")
    elif style == "sparse":
        p.drum(page, c, 0, "X.......x......."); p.drum(page, c, 1, "........X......."); p.drum(page, c, 7, "x...x...x...x...")
    if page == 3: p.drum(page, c, 9, "X...............")


CH = dict(Am=[57, 60, 64], F=[53, 57, 60], C=[48, 52, 55], G=[55, 59, 62], Dm=[50, 53, 57], Bb=[58, 62, 65], Em=[52, 55, 59], D=[50, 54, 57],
          B=[59, 63, 66], Cm=[48, 51, 55], Ab=[56, 60, 63], Eb=[51, 55, 58], Fm=[53, 56, 60], Gm=[55, 58, 62], A=[57, 61, 64], Fsm=[54, 57, 61], E=[52, 56, 59],
          Bm=[59, 62, 66], Cmaj7=[48, 52, 55], Am7=[57, 60, 64], Fmaj7=[53, 57, 60])
ORDER = [0, 1, 2, 3, 0, 1, 2, 3]


def build(bpm, prog, voices, leadshift=12, bassoct=-12, drumstyle="rock", order=ORDER, shapes=(0, 1, 2, 3)):
    """voices: list of (kind, role, kwargs); roles: lead, lead2, arp, pad, bass, drums"""
    p = Project(bpm)
    idx = [(p.add(k, **kw), role) for k, role, kw in voices]
    for pg, name in enumerate(prog):
        ch = CH[name]
        for c, role in idx:
            if role == "lead": p.notes(pg, c, lead(ch, shapes[pg], leadshift), 8)
            elif role == "lead2": p.notes(pg, c, lead(ch, shapes[(pg + 2) % 4], leadshift - 12), 8)
            elif role == "arp": p.notes(pg, c, arp(ch, shift=12), 4)
            elif role == "pad": p.notes(pg, c, [tone(ch, 0) + 12] + [0] * 15, 95)
            elif role == "pad2": p.notes(pg, c, [tone(ch, 1) + 12] + [0] * 15, 95)
            elif role == "pad3": p.notes(pg, c, [tone(ch, 2) + 12] + [0] * 15, 95)
            elif role.startswith("bass"): p.notes(pg, c, bass(ch[0] + bassoct, role[4:] or "oct"), 7)
            elif role == "drums": drums(p, pg, c, drumstyle)
    p.song = list(order)
    return p


def mk(title, p, fn):
    data = p.bytes()
    assert len(data) < 12000
    (OUT / "samples" / f"{fn}.chipbox").write_text(json.dumps({"chipbox": 1, "projects": [{"name": title, "data": base64.b64encode(data).decode()}]}) + "\n")
    return {"name": title, "file": f"samples/{fn}.chipbox"}


def samples():
    S = []
    # one console each
    S.append(mk("NES", build(150, ["Am", "F", "C", "G"], [("NES_PU", "lead", dict(duty=2, vol=13)), ("NES_PU", "arp", dict(duty=0, vol=8, dec=3, sus=7)), ("NES_TRI", "bass", {}), ("DRUMS_NES", "drums", {})]), "nes"))
    S.append(mk("Game Boy", build(128, ["C", "Am", "F", "G"], [("GB_PU", "lead", dict(duty=1, vol=13)), ("GB_PU", "arp", dict(duty=3, vol=7, dec=3, sus=6)), ("GB_WAV", "bass", dict(wave=10)), ("DRUMS_GB", "drums", {})], drumstyle="four", shapes=(2, 0, 3, 1)), "game-boy"))
    S.append(mk("Master System", build(140, ["Dm", "Bb", "F", "C"], [("SMS", "lead", dict(vol=13)), ("SMS", "arp", dict(vol=8, dec=3)), ("SMS", "bass", dict(vol=14)), ("DRUMS_SMS", "drums", {})], drumstyle="break", shapes=(1, 3, 0, 2)), "master-system"))
    S.append(mk("PC Engine", build(112, ["Fsm", "D", "A", "E"], [("PCE", "lead", dict(wave=5, vol=12, vib_dep=3, pan=0)), ("PCE", "pad", dict(wave=15, vol=9, atk=4, rel=8, pan=2)), ("PCE", "pad2", dict(wave=15, vol=9, atk=4, rel=8, pan=1)), ("PCE", "bass", dict(wave=0, vol=14)), ("DRUMS_PCE", "drums", {})], drumstyle="four"), "pc-engine"))
    S.append(mk("Atari 2600", build(118, ["Em", "C", "D", "B"], [("TIA", "lead", dict(wave=3, vol=12)), ("TIA", "bass", dict(wave=6, vol=14)), ("TIA", "arp", dict(wave=1, vol=8, dec=3)), ("DRUMS_TIA", "drums", {})], drumstyle="rock", shapes=(3, 1, 2, 0)), "atari-2600"))
    S.append(mk("Konami VRC6", build(156, ["Dm", "Bb", "C", "A"], [("VRC6_SAW", "lead", dict(vol=13, vib_dep=3)), ("VRC6_PU", "arp", dict(duty=7, vol=9, dec=3, sus=7)), ("VRC6_PU", "bass", dict(duty=4, vol=14)), ("DRUMS_VRC6", "drums", {})], drumstyle="break", shapes=(0, 2, 1, 3)), "konami-vrc6"))
    S.append(mk("AY-3-8910", mountain_king(), "ay-3-8910"))
    S.append(mk("Mega Drive", build(100, ["Am7", "Fmaj7", "Am7", "G"], [("MD", "bass", dict(wave=1, wf=6, wc=1)), ("MD", "arp", dict(duty=1, wave=3, wf=5, wc=2, vol=9, dec=4)), ("MD", "lead", dict(duty=0, wave=3, wf=9, wc=3, vib_dep=3, vol=12)), ("DRUMS_MD", "drums", {})], drumstyle="break", shapes=(1, 2, 3, 0)), "mega-drive"))
    S.append(mk("Virtual Boy", build(84, ["Cmaj7", "Am7", "Fmaj7", "G"], [("VB", "lead", dict(wave=15, vol=12, pan=0, vib_dep=2)), ("VB", "pad", dict(wave=10, vol=9, atk=6, rel=9, pan=2)), ("VB", "pad2", dict(wave=10, vol=9, atk=6, rel=9, pan=1)), ("VB", "bass", dict(wave=15, vol=13)), ("DRUMS_VB", "drums", {})], drumstyle="sparse", bassoct=-24, leadshift=24), "virtual-boy"))
    # more than one
    S.append(mk("NES, Game Boy", build(140, ["Am", "G", "F", "E"], [("NES_PU", "lead", dict(duty=2, vol=13)), ("GB_PU", "arp", dict(duty=2, vol=8, dec=3, sus=6)), ("GB_WAV", "bass", dict(wave=5)), ("DRUMS_NES", "drums", {})], drumstyle="rock", shapes=(0, 2, 1, 3)), "nes-game-boy"))
    S.append(mk("Mega Drive, Master System", build(118, ["Cm", "Ab", "Eb", "Bb"], [("MD", "bass", dict(wave=1, wf=7, wc=1)), ("MD", "lead", dict(duty=2, wave=3, wf=8, wc=2, vol=12, vib_dep=3)), ("SMS", "arp", dict(vol=8, dec=3)), ("DRUMS_MD", "drums", {}), ("DRUMS_SMS", "drums", {})], drumstyle="four", shapes=(1, 0, 3, 2)), "mega-drive-master-system"))
    S.append(mk("Sunsoft 5B, Konami VRC6", build(144, ["Em", "C", "D", "B"], [("S5B", "bass", dict(vol=13, pan=1, wave=0)), ("VRC6_SAW", "lead", dict(vol=13, vib_dep=3)), ("VRC6_PU", "arp", dict(duty=5, vol=8, dec=3, sus=6)), ("DRUMS_S5B", "drums", {})], drumstyle="break", bassoct=-24, shapes=(2, 3, 0, 1)), "sunsoft-5b-konami-vrc6"))
    S.append(mk("AdLib, PC Speaker, Atari 8-bit", build(120, ["Dm", "Gm", "Bb", "A"], [("OPL", "bass", dict(duty=1, wave=1, wf=6, wc=1)), ("SPK", "lead", {}), ("OPL", "pad", dict(duty=3, wave=3, wf=6, vol=8, dec=4, sus=6)), ("POKEY", "arp", dict(wave=3, vol=8, dec=3)), ("DRUMS_POKEY", "drums", {})], drumstyle="rock", shapes=(0, 3, 2, 1)), "adlib-pc-speaker-atari-8-bit"))
    S.append(mk("Namco 163, Namco WSG", build(132, ["C", "Am", "F", "G"], [("WSG", "lead", dict(wave=10, vol=13)), ("N163", "pad", dict(wave=15, vol=8, atk=6, rel=8)), ("N163", "pad2", dict(wave=15, vol=8, atk=6, rel=8)), ("N163", "pad3", dict(wave=15, vol=8, atk=6, rel=8)), ("N163", "bass", dict(wave=5, vol=14)), ("DRUMS_WSG", "drums", {})], drumstyle="four", shapes=(2, 1, 0, 3)), "namco-163-namco-wsg"))
    return S


def mountain_king():                            # In the Hall of the Mountain King (Grieg, 1875; public domain), the theme
    p = Project(132)
    lead_c = p.add("AY", vol=13, wave=2)
    arp_c = p.add("AY", vol=8, wave=0, dec=3)
    bass_c = p.add("AY", vol=14, wave=3)
    dr = p.add("DRUMS_AY")
    theme = [N("E4 F#4 G4 A4 B4 G4 B4 . Bb4 F#4 Bb4 . A4 F4 A4 ."), N("E4 F#4 G4 A4 B4 G4 B4 . E5 B4 G4 B4 E5 . . ."),
             N("E4 F#4 G4 A4 B4 G4 B4 . Bb4 F#4 Bb4 . A4 F4 A4 ."), N("A4 G4 F#4 E4 F#4 G4 A4 B4 G4 E4 G4 B4 E5 . . .")]
    root = [40, 40, 40, 40]
    for pg in range(4):
        p.notes(pg, lead_c, theme[pg], 9)
        p.notes(pg, arp_c, arp([52, 55, 59], (0, 1, 2, 4), 12), 4)
        p.notes(pg, bass_c, bass(40, "oct" if pg < 3 else "sync"), 7)
        drums(p, pg, dr, "rock")
    p.song = [0, 1, 2, 3, 0, 1, 2, 3]
    return p


def stress():                                   # every console, 64 pages: sixteen sections of four, each led by another console
    leads = ["NES_PU", "GB_WAV", "SMS", "PCE", "TIA", "VRC6_SAW", "AY", "MD", "VB", "POKEY", "SPK", "OPL", "WSG", "N163", "S5B"]
    basses = ["PCE", "VRC6_SAW", "N163", "GB_WAV", "MD", "OPL", "S5B", "NES_PU"]
    arps = ["AY", "POKEY", "SMS", "WSG", "VB", "TIA", "SPK", "PCE", "NES_PU", "S5B"]
    progs = [["Am", "F", "C", "G"], ["Dm", "Bb", "F", "C"], ["Em", "C", "G", "D"], ["Cm", "Ab", "Eb", "Bb"]]
    styles = ["oct", "walk", "sync", "quarter"]
    dstyle = ["sparse", "rock", "four", "break"]
    p = Project(140, 64)
    ch = {k: p.add(k, **extra) for k, extra in [(k, {}) for k in leads]}
    ch["PCE"] = ch["PCE"]
    dr = p.add("DRUMS_NES")
    for sec in range(16):
        prog = progs[sec // 4]
        for i in range(4):
            pg = sec * 4 + i
            c = CH[prog[i]]
            if sec == 15:                           # everyone, as chords
                for j, k in enumerate(["PCE", "N163", "WSG", "OPL", "S5B", "VRC6_SAW", "MD", "VB", "GB_WAV", "AY"]):
                    p.notes(pg, ch[k], [tone(c, j % 3) + 12 * (1 + j // 3)] + [0] * 15, 95)
                p.notes(pg, ch["NES_PU"], lead(c, i, 12), 8)
                p.notes(pg, ch["SMS"], lead(c, i + 2, 0), 8)
                drums(p, pg, dr, "four")
                continue
            L = leads[sec]
            B = basses[sec % len(basses)]
            if B == L: B = basses[(sec + 1) % len(basses)]
            p.notes(pg, ch[L], lead(c, (sec + i) % 4, 12), 8)
            p.notes(pg, ch[B], bass(c[0] - 12, styles[(sec + i // 2) % 4]), 7)
            if sec % 2 == 1 and sec >= 3:
                A = arps[sec % len(arps)]
                if A in (L, B): A = "SPK" if "SPK" not in (L, B) else "TIA"
                p.notes(pg, ch[A], [x if s % 2 == 0 else 0 for s, x in enumerate(arp(c, shift=12))], 4)
            if sec >= 2 and sec != 7: drums(p, pg, dr, dstyle[(sec + i // 2) % 4])
    p.song = list(range(64))
    print("stress:", len(p.ev), "events,", len(p.bytes()), "bytes")
    return p


if __name__ == "__main__":
    (OUT / "samples").mkdir(exist_ok=True)
    new = samples()
    old = json.loads((OUT / "samples.json").read_text()) if (OUT / "samples.json").exists() else []
    keep = [o for o in old if o["file"].split("/")[-1] in {"atari-8-bit.chipbox", "pc-speaker-ode.chipbox", "adlib-dungeon.chipbox", "namco-wsg-chase.chipbox", "namco-163-wall.chipbox", "sunsoft-5b-buzz.chipbox"}]
    keep = [{"name": o["name"], "file": o["file"]} for o in keep]
    singles = new[:9] + keep + new[9:]
    sym = stress()
    singles.append(mk("All consoles", sym, "all-consoles"))
    d = sym.bytes()                             # the same project is the firmware's DEMO 2
    rows = [", ".join(str(x) for x in d[i:i + 24]) for i in range(0, len(d), 24)]
    (OUT.parent.parent / "firmware" / "gen" / "demo2.h").write_text("/* Made by tools/make_samples.py: the console symphony, DEMO 2 (a project, firmware/src/proj.c format). */\nstatic const uint8_t DEMO2[%d] = {\n    %s\n};\n" % (len(d), ",\n    ".join(rows)))
    (OUT / "samples.json").write_text(json.dumps(singles, indent=1) + "\n")
    print(len(singles), "samples")
