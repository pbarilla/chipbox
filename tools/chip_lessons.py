#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The chip lessons: one for every chip. This file is the source: it writes firmware/src/lessons_chips.h (the LESSON[] entries) and the host test (tests/lesson_test.py)
# reads the same list to do each lesson to the letter.  python3 tools/chip_lessons.py
# A step: (text, pill, value, op, number, action). Texts: verb first, the control, what it does to something on screen (the dial's label), 26 characters at most.
import os, sys

def H():   return ("Press HOME to open CHIP", "HOME", "V_VIEW", "O_EQ", 0, ("home",))
def KEY(): return ("Press a key to hear it", "KEYS", "V_KEYS", "O_EQ", 1, ("key",))
def K(n, label, v="V_INALL"): return (f"Turn KNOB {n} for {label}", f"KNOB {n}", v, "O_CHG", 0, ("knob", n))
def KT(n, text, v="V_INALL"): return (text, f"KNOB {n}", v, "O_CHG", 0, ("knob", n))
class A:                                         # ALGORITHM to an absolute page (mod: how many pages the screen has, to wrap)
    def __init__(s, page, text, mod=0): s.page, s.text, s.mod = page, text, mod
def SEL(text, foc, op="O_GE"): return (text, "SEL", "V_FMFOC", op, foc, ("sel", 1))
class DRAW(A):                                  # the DRAW page (the last one)
    def __init__(s, page, text="Turn ALGORITHM to BRUSH"): A.__init__(s, page, text); s.last = True

FM = "V_FMHASH"
L = []                                           # (name, kind constant, steps)
def lesson(name, kd, *steps): L.append((name, kd, list(steps)))

lesson("The NES Pulse", "KD_NES_PU", H(), K(1, "DUTY"), K(4, "SWEEP"), K(3, "VIB"), A(1, "Turn ALGORITHM: next page"), K(1, "ATK"), K(4, "REL"), KEY())
lesson("The Game Boy", "KD_GB_PU", H(), K(1, "DUTY"), K(4, "PAN"), A(3, "Turn ALGORITHM to SWEEP"), K(1, "SWEEP"), KEY())
lesson("The Master System", "KD_SMS", H(), K(1, "NOISE"), K(3, "VIB"), K(4, "SWEEP"), A(1, "Turn ALGORITHM: next page"), K(2, "DEC"), KEY())
lesson("The PC Engine", "KD_PCE", H(), K(1, "WAVE"), K(4, "PAN"), A(3, "Turn ALGORITHM to FOLD"), K(2, "FOLD"), K(4, "TWIST"), A(4, "Turn ALGORITHM to SWEEP"), K(1, "SWEEP"), KEY())
lesson("The Atari TIA", "KD_TIA", H(), K(1, "TYPE"), K(4, "SWEEP"), K(3, "VIB"), A(1, "Turn ALGORITHM: next page"), K(1, "ATK"), KEY())
lesson("The VRC6 Saw", "KD_VRC6_SAW", H(), K(1, "OVER"), K(3, "VIB"), K(4, "SWEEP"), A(1, "Turn ALGORITHM: next page"), K(3, "SUS"), KEY())
lesson("The AY Chip", "KD_AY", H(), K(1, "NOISE"), A(3, "Turn ALGORITHM to PERIOD"), K(2, "BUZZ"), K(3, "BRATIO"), K(1, "PERIOD"), KEY())
lesson("The Mega Drive", "KD_MD", H(), K(1, "ALGO", FM), K(3, "PAN", FM), SEL("Press SEL to pick operator", 1), K(1, "TL", FM), A(1, "Turn ALGORITHM to AR"), K(1, "AR", FM), K(3, "D1L", FM), KEY())
lesson("The Virtual Boy", "KD_VB_WAV", H(), K(1, "WAVE"), K(4, "PAN"), A(3, "Turn ALGORITHM to TWIST"), K(4, "TWIST"), A(4, "Turn ALGORITHM to SWEEP"), K(1, "SWEEP"), KEY())
lesson("The Atari POKEY", "KD_POKEY", H(), K(1, "DIST"), K(4, "SWEEP"), A(3, "Turn ALGORITHM to HPF"), K(1, "HPF"), K(2, "CLOCK"), KEY())
lesson("The PC Speaker", "KD_SPK", H(), K(1, "DUTY"), K(3, "VIB"), K(4, "SWEEP"), A(2, "Turn ALGORITHM to SHAPE"), K(1, "SHAPE"), K(2, "RATE"), KEY())
lesson("The AdLib", "KD_OPL", H(), K(1, "CNT", FM), K(2, "FB", FM), A(1, "Turn ALGORITHM to TREMD"), K(1, "TREMD", FM), K(2, "VIBD", FM), SEL("Press SEL to pick operator", 1), K(4, "WAVE", FM), KEY())
lesson("Namco WSG", "KD_WSG", H(), K(1, "WAVE"), K(4, "SWEEP"), A(3, "Turn ALGORITHM to FOLD"), K(2, "FOLD"), K(3, "STEP"), KEY())
lesson("The Namco 163", "KD_N163", H(), K(1, "WAVE"), A(3, "Turn ALGORITHM to STEP"), K(3, "STEP"), A(4, "Turn ALGORITHM to LENGTH"), K(1, "LENGTH"), DRAW(5), KEY())
lesson("Sunsoft 5B", "KD_S5B", H(), K(1, "NOISE"), K(4, "BUZZ"), A(3, "Turn ALGORITHM to PERIOD"), K(1, "PERIOD"), K(3, "MIXENV"), KEY())
lesson("The NES MMC5", "KD_MMC5", H(), K(1, "DUTY"), K(3, "VIB"), A(1, "Turn ALGORITHM: next page"), K(2, "DEC"), KEY())
lesson("The Famicom Disk", "KD_FDS", H(), K(1, "WAVE"), K(4, "DEPTH"), A(4, "Turn ALGORITHM to RATIO"), K(1, "RATIO"), K(2, "SWEEP"), K(3, "SHAPE"), KEY())
lesson("The MSX SCC", "KD_SCC", H(), K(1, "WAVE"), K(4, "SWEEP"), A(3, "Turn ALGORITHM to FOLD"), K(2, "FOLD"), K(4, "TWIST"), DRAW(4), KEY())
lesson("MSX Music", "KD_OPLL", H(), K(1, "INST", FM), K(2, "FB", FM), K(3, "PEDAL", FM), SEL("Press SEL to pick operator", 1), K(1, "TL", FM), K(3, "MUL", FM), KEY())
lesson("The VRC7", "KD_VRC7", H(), K(1, "INST", FM), K(2, "FB", FM), K(3, "PEDAL", FM), SEL("Press SEL to pick operator", 1), K(1, "TL", FM), A(1, "Turn ALGORITHM to AR"), K(1, "AR", FM), KEY())
# FM synthesis: the long one
lesson("FM Synthesis", "KD_OPM", H(), K(1, "ALGO", FM), K(2, "FB", FM), K(3, "PAN", FM), A(1, "Turn ALGORITHM to SPEED", 4), K(1, "SPEED", FM), K(3, "PMD", FM),
       A(2, "Turn ALGORITHM to PMS", 4), K(1, "PMS", FM), SEL("Press SEL to pick operator", 1), A(0, "Turn ALGORITHM to TL", 3), K(1, "TL", FM), K(2, "MUL", FM),
       A(1, "Turn ALGORITHM to AR", 3), K(1, "AR", FM), K(3, "D1L", FM), SEL("Press SEL for operator 2", 2, "O_EQ"), K(4, "D2R", FM), KEY())
lesson("The OPL3", "KD_OPL3", H(), K(1, "CONN", FM), K(3, "PAN", FM), A(1, "Turn ALGORITHM to TREMD", 3), K(1, "TREMD", FM), SEL("Press SEL to pick operator", 1), K(4, "WAVE", FM), KEY())
lesson("The WonderSwan", "KD_WS", H(), K(1, "WAVE"), K(4, "PAN"), A(3, "Turn ALGORITHM to FOLD"), K(2, "FOLD"), DRAW(4), KEY())
lesson("The SAA1099", "KD_SAA", H(), K(1, "NOISE"), K(4, "PAN"), A(3, "Turn ALGORITHM to ENV"), K(1, "ENV"), K(2, "ERES"), K(3, "NRATE"), KEY())
lesson("The VIC-20", "KD_VIC", H(), K(1, "TYPE"), K(4, "SWEEP"), K(3, "VIB"), A(1, "Turn ALGORITHM: next page"), K(1, "ATK"), KEY())
lesson("The Plus/4 TED", "KD_TED", H(), K(1, "TYPE"), K(4, "SWEEP"), A(2, "Turn ALGORITHM to SHAPE"), K(1, "SHAPE"), K(2, "RATE"), KEY())
lesson("The Atari Lynx", "KD_LYNX", H(), K(1, "TYPE"), K(4, "PAN"), A(3, "Turn ALGORITHM to INTEG"), K(1, "INTEG"), K(2, "TAPS"), K(3, "BAL"), KEY())
lesson("The PC-98", "KD_PC98_FM", H(), K(1, "ALGO", FM), K(2, "FB", FM), K(3, "PAN", FM), SEL("Press SEL to pick operator", 1), K(1, "TL", FM), A(1, "Turn ALGORITHM to AR"), K(1, "AR", FM), KEY())
lesson("The Neo Geo", "KD_NEO_FM", H(), K(1, "ALGO", FM), K(2, "FB", FM), A(1, "Turn ALGORITHM to LFO", 3), K(1, "LFO", FM), K(2, "PMS", FM), SEL("Press SEL to pick operator", 1), K(2, "MUL", FM), KEY())
lesson("The Neo Geo Pocket", "KD_NGP", H(), K(1, "NOISE"), K(4, "PAN"), A(3, "Turn ALGORITHM to SWEEP"), K(1, "SWEEP"), KEY())
lesson("The OPL", "KD_OPL1", H(), K(1, "CNT", FM), K(2, "FB", FM), A(1, "Turn ALGORITHM to TREMD", 3), K(1, "TREMD", FM), SEL("Press SEL to pick operator", 1), K(1, "TL", FM), K(3, "KSL", FM), KEY())
lesson("The OPZ", "KD_OPZ", H(), K(1, "ALGO", FM), A(1, "Turn ALGORITHM to SPEED", 4), K(1, "SPEED", FM), K(2, "WAVE", FM), SEL("Press SEL to pick operator", 1), A(0, "Turn ALGORITHM to TL", 4), K(3, "FINE", FM), A(3, "Turn ALGORITHM to DT2", 4), K(1, "DT2", FM), KEY())
lesson("The SN76477", "KD_SN77", H(), K(1, "MIX"), K(4, "SLF"), A(3, "Turn ALGORITHM to FILTER"), K(1, "FILTER"), K(2, "RATE"), K(3, "DEPTH"), K(4, "DUTY"), KEY())
lesson("Namco C30", "KD_C30", H(), K(1, "WAVE"), K(4, "PAN"), A(3, "Turn ALGORITHM to FOLD"), K(2, "FOLD"), DRAW(4), KEY())
lesson("The MSM5232 Organ", "KD_MSM", H(), K(1, "FOOT"), K(4, "QUINT"), K(3, "VIB"), A(1, "Turn ALGORITHM: next page"), K(1, "ATK"), KEY())
lesson("The ZX Beeper", "KD_ZX", H(), K(1, "DUTY"), K(3, "VIB"), K(4, "SWEEP"), A(2, "Turn ALGORITHM to SHAPE"), K(1, "SHAPE"), K(2, "RATE"), KEY())
lesson("The YM2203", "KD_OPN_FM", H(), K(1, "ALGO", FM), K(2, "FB", FM), SEL("Press SEL to pick operator", 1), K(1, "TL", FM), K(2, "MUL", FM), A(1, "Turn ALGORITHM to AR", 3), K(1, "AR", FM), KEY())
lesson("The SID", "KD_SID", H(), K(1, "WAVE"), K(4, "CHIP"), A(3, "Turn ALGORITHM to PW"), K(1, "PW"), K(3, "SYNC"), A(4, "Turn ALGORITHM to CUT"), K(1, "CUT"), K(2, "RES"), K(3, "MODE"), KEY())

def actions(steps):
    """What the host test does for each step: ('home',) ('knob', n) ('alg', clicks) ('sel', n) ('key',) ('alg_last',)"""
    out, cur = [], 0
    for st in steps:
        if isinstance(st, A):
            d = (st.page - cur) % st.mod if st.mod else st.page - cur
            out.append(("alg", d)); cur = st.page
        else:
            out.append(st[5])
    return out

def steps_c(steps):
    cur, out = 0, []
    for st in steps:
        if isinstance(st, A):
            out.append('ST("%s", "ALGORITHM", %s, O_EQ, %d)' % (st.text, "V_LASTPG" if getattr(st, "last", False) else "V_CHIPPG", 1 if getattr(st, "last", False) else st.page))
        else:
            t, p, v, o, n, _ = st
            out.append('ST("%s", "%s", %s, %s, %d)' % (t, p, v, o, n))
    return out

def emit(root):
    parts = ([], [])
    k = 0
    for name, kd, steps in L:
        for st in steps:
            text = st.text if isinstance(st, A) else st[0]
            assert len(text) <= 26, (name, text)
        lines = steps_c(steps)
        parts[k].append('    {"%s", S_CHIP, %d, (const lstep_t[]){%s}, %s},\n' % (name, len(steps), ", ".join(lines), kd))
        if name == "FM Synthesis": k = 1                  # the FM masterclass goes here, in lessons.c
    for n, p in zip("ab", parts):
        with open(os.path.join(root, "firmware/src/lessons_chips_%s.h" % n), "w") as f:
            f.write("/* SPDX-License-Identifier: GPL-3.0-only */\n/* The chip lessons, made by tools/chip_lessons.py (edit that, not this). Included in LESSON[] in lessons.c. */\n" + "".join(p))

if __name__ == "__main__":
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    emit(root)
    print(len(L), "chip lessons")
