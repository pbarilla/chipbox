#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Does every lesson in firmware/src/lessons.c to the letter in the host sim and checks that each step finishes (the host prints "LESSON i STEP j OK").
#   python3 tests/lesson_test.py [BINARY] [lesson numbers ...]      (BINARY: the host build of chipbox.c; without it the script builds one)
import os, re, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FK = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26]       # the white keys, F3 up
BK = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25]
SEL, ALG, K1, K2, K3, K4, PRE = 0, 1, 2, 3, 4, 5, 6
OCTD, OCTU, FX, SELB, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC = range(14)

class R:
    def __init__(s): s.t = 300; s.L = []; s.once = False
    def ev(s, x, dt=150): s.t += dt; s.L.append(f"{s.t} {x}")
    def wait(s, ms): s.t += ms
    def btn(s, b, dt=150): s.ev(f"btn {b}", dt)
    def enc(s, i, d=1, dt=150):
        n = -(-4 // abs(d)) if 2 <= i <= 5 and abs(d) < 4 and not s.once else 1      # a knob step wants a few clicks
        for _ in range(n): s.ev(f"enc {i} {d}", dt if _ == 0 else 60)
    def encs(s, i, d, n): [s.enc(i, d, 90) for _ in range(n)]
    def key(s, k, hold=120): s.ev(f"keys {hex(1 << k)}", 150); s.ev("keys 0", hold)
    def white(s, n): s.key(FK[n - 1])
    def black(s, n): s.key(BK[n - 1])
    def held(s, mask, ms): s.ev(f"held {hex(mask)}", 100); s.t += ms; s.ev("held 0", 0)
    def home(s): s.held(0x100, 70); s.wait(450)
    def hold_edit(s): s.held(0x40, 800); s.wait(450)
    def tap_edit(s): s.held(0x40, 100); s.wait(450)
    def hold_sel(s): s.held(0x8, 800); s.wait(450)
    def play_rack(s): s.btn(PLAY); s.ev("held 0x1000", 0); s.t += 600; s.ev("held 0", 0)       # PLAY in the rack: held, the page loops
    def step(s): s.wait(1200)

def L_get_around(r):
    r.home(); r.step(); r.home(); r.step(); r.home(); r.step(); r.enc(PRE, 1); r.step(); r.play_rack()
def L_first_notes(r):
    r.btn(SEQ); r.step()
    r.white(8); r.step()
    for n in (9, 10, 11): r.white(n)
    r.step(); r.enc(SEL, 1); r.step()
    r.btn(PLAY); r.step(); r.btn(PLAY)
def L_rests(r):
    r.btn(SEQ); r.step(); r.white(8); r.step(); r.enc(SEL, 1); r.step(); r.white(9); r.step(); r.btn(PLAY); r.step(); r.btn(PLAY)
def L_length_erase(r):
    r.btn(SEQ); r.step(); r.enc(SEL, 1); r.step()
    r.enc(K2, 2); r.step(); r.btn(EDIT); r.step(); r.white(8)
def L_play_tape(r):
    r.play_rack(); r.step(); r.enc(SEL, 1); r.step()
    r.enc(SEL, -1); r.enc(SEL, -1); r.step(); r.enc(SEL, -1); r.step(); r.enc(SEL, 1); r.step(); r.btn(PLAY)
def L_record(r):
    r.btn(REC); r.step()
    for _ in range(6): r.white(8); r.wait(180)
    r.step(); r.btn(REC)
def L_pages(r):
    r.enc(ALG, 1); r.step(); r.btn(SEQ); r.step(); r.white(8); r.step(); r.enc(ALG, -1); r.step(); r.btn(PLAY)
def L_song(r):
    r.home(); r.home(); r.step(); r.btn(OCTU); r.step(); r.enc(PRE, 1); r.step(); r.enc(SEL, -1); r.step(); r.wait(2600)
    r.btn(PLAY); r.step(); r.btn(PLAY)
def L_copy_page(r):
    r.home(); r.home(); r.step(); r.hold_edit(); r.step(); r.tap_edit(); r.step(); r.btn(SEQ); r.step(); r.white(9)
def L_tempo(r):
    r.enc(K1, 5); r.step(); r.play_rack(); r.step(); r.enc(K1, 3); r.step(); r.btn(PLAY)
def L_sound(r):
    r.home(); r.step(); r.enc(K1, 1); r.step(); r.enc(K2, -2); r.step(); r.enc(K3, 1); r.step(); r.btn(PLAY)
def L_envelope(r):
    r.home(); r.step(); r.enc(ALG, 1); r.step(); r.enc(K1, 1); r.step(); r.enc(K3, 1); r.step(); r.enc(K4, 1)
def L_arp(r):
    r.home(); r.step(); r.enc(ALG, 1); r.enc(ALG, 1); r.step(); r.enc(K1, 1); r.step(); r.enc(K2, 1); r.step(); r.white(5)
def L_vibrato(r):
    r.home(); r.step(); r.enc(K3, 2); r.step(); r.enc(K4, 2); r.step(); r.white(5)
def L_add_channel(r):
    r.enc(PRE, 1); r.step(); r.btn(OCTU); r.step(); r.enc(SEL, 1); r.step(); r.btn(OCTU); r.step(); r.btn(OCTU)
def L_add_drums(r):
    r.enc(PRE, 1); r.step(); r.btn(OCTU); r.step(); r.btn(OCTU); r.step(); r.encs(SEL, 1, 14); r.btn(OCTU); r.step()
    r.btn(SEQ); r.step(); r.black(2); r.step(); r.white(3); r.step(); r.btn(PLAY)
def L_accents(r):
    r.enc(PRE, 1); r.step(); r.btn(SEQ); r.step(); r.ev("held 0x10", 100); r.white(4); r.ev("held 0", 100); r.step(); r.btn(PLAY)
def L_mixing(r):
    r.enc(K2, -2); r.step(); r.enc(K4, 2); r.step(); r.enc(K3, 1); r.step(); r.enc(PRE, 1); r.step(); r.play_rack()
def L_fx(r):
    r.play_rack(); r.step(); r.btn(FX); r.step(); r.ev(f"keys {hex(1 << FK[1])}", 100); r.wait(400); r.ev("keys 0", 0); r.step(); r.btn(FX)
def L_undo(r):
    r.btn(SEQ); r.step(); r.white(8); r.wait(900); r.step(); r.white(10); r.wait(900); r.step(); r.btn(LFO)
def L_copy_channel(r):
    r.hold_sel(); r.step(); r.btn(OCTU); r.step(); r.enc(PRE, 1)
def L_new_console(r):
    r.tap_edit(); r.step(); r.enc(SEL, 1); r.step(); r.btn(OCTU); r.step(); r.btn(OCTU)
def L_draw_wave(r):
    r.home(); r.step()
    for _ in range(5): r.enc(ALG, 1, 350)
    r.step(); r.enc(K1, 3); r.step(); r.enc(SEL, 1); r.step(); r.enc(K2, 1); r.step(); r.white(5)
def L_fm(r):
    r.home(); r.step(); r.btn(SELB); r.step(); r.enc(K1, 1); r.step(); r.white(5)
def L_odd_meters(r):
    r.btn(SEQ); r.step(); r.white(8); r.step(); r.enc(K3, -1); r.step(); r.btn(PLAY); r.step(); r.enc(K3, 1); r.enc(K3, 1); r.enc(K3, 1)
def L_long_pages(r):
    r.btn(SEQ); r.step(); r.enc(K4, 1); r.step(); r.encs(SEL, 1, 17); r.step(); r.home(); r.play_rack()
def L_tempo_changes(r):
    r.home(); r.home(); r.step(); r.btn(OCTU); r.step(); r.tap_edit(); r.step(); r.btn(SEQ); r.step(); r.enc(K1, 10); r.step()
    r.home(); r.home(); r.home(); r.wait(2600); r.btn(PLAY)
def L_glide(r):
    r.btn(SEQ); r.step(); r.btn(SELB); r.step(); r.btn(SELB); r.step(); r.home(); r.home(); r.home(); r.wait(2600); r.btn(PLAY)
def L_first_song(r):
    r.btn(OCTU); r.btn(OCTU); r.btn(OCTU); r.step(); r.btn(SEQ); r.step()
    for n in (8, 9, 10, 11): r.white(n)
    r.step(); r.home(); r.step(); r.enc(PRE, 1); r.btn(OCTU); r.btn(OCTU); r.encs(SEL, 1, 14); r.btn(OCTU); r.step()
    r.btn(SEQ); r.step(); r.black(2)
    for n in (1, 3, 5, 7): r.white(n)
    r.step(); r.home(); r.home(); r.home(); r.step(); r.btn(OCTU); r.step(); r.hold_edit(); r.step(); r.btn(PLAY)

def L_slide(r):
    r.enc(K3, 1); r.step(); r.enc(K3, -1); r.enc(K3, -1); r.step(); r.play_rack(); r.step(); r.btn(PLAY)
def L_tails(r):
    r.btn(ARP); r.step(); r.play_rack(); r.step(); r.btn(ARP)
def L_latch(r):
    r.play_rack(); r.step(); r.btn(FX); r.step()
    r.ev(f"keys {hex(1 << FK[15])}", 100); r.wait(200); r.ev("keys 0", 0); r.step()
    r.ev(f"keys {hex(1 << FK[3])}", 100); r.wait(200); r.ev("keys 0", 0); r.step()
    r.ev(f"keys {hex(1 << FK[15])}", 100); r.wait(200); r.ev("keys 0", 0)
def L_sid(r):
    r.home(); r.step(); r.enc(K1, 1); r.step(); r.enc(K4, 1); r.step(); r.encs(ALG, 1, 4); r.step(); r.enc(K1, 3); r.step(); r.play_rack()
def L_pce(r):
    r.home(); r.step(); r.enc(K1, 1); r.step(); r.encs(ALG, 1, 3); r.step(); r.enc(K2, 3); r.step(); r.play_rack()
def L_tia(r):
    r.home(); r.step(); r.enc(K1, 2); r.step(); r.enc(K4, 2); r.step(); r.play_rack()
def L_vrc6(r):
    r.home(); r.step(); r.enc(K1, 3); r.step(); r.enc(K3, 2); r.step(); r.play_rack()
def L_ay(r):
    r.home(); r.step(); r.enc(K1, 2); r.step(); r.encs(ALG, 1, 3); r.step(); r.enc(K2, 2); r.step(); r.play_rack()

sys.path.insert(0, os.path.join(ROOT, "tools"))
import chip_lessons
def chip_action(steps):                          # a chip lesson, from the same list the firmware's entries are made from
    acts = chip_lessons.actions(steps)
    def run(r):
        for a in acts:
            if a[0] == "home": r.home()
            elif a[0] == "knob": r.enc([K1, K2, K3, K4][a[1] - 1], 1); r.encs([K1, K2, K3, K4][a[1] - 1], -1, 4)       # up, then down: one of them moves a knob that sits at its end
            elif a[0] == "alg": r.encs(ALG, 1, a[1])
            elif a[0] == "sel": [r.btn(SELB) for _ in range(a[1])]
            elif a[0] == "key": r.white(5)
            r.step()
    return run
GENERAL = {f.__name__[2:]: f for f in [L_get_around, L_first_notes, L_rests, L_length_erase, L_play_tape, L_record, L_pages, L_song, L_copy_page, L_tempo, L_sound, L_envelope, L_arp, L_vibrato, L_add_channel, L_add_drums,
           L_accents, L_mixing, L_fx, L_undo, L_copy_channel, L_new_console, L_draw_wave, L_odd_meters, L_long_pages, L_tempo_changes, L_glide, L_slide, L_tails, L_latch, L_first_song]}
GEN_NAMES = ["Get Around", "First Notes", "Rests", "Length and Erase", "Play and Tape", "Record Live", "Pages", "Make a Song", "Copy a Page", "Tempo", "Change the Sound", "Envelope", "Arpeggio", "Vibrato and Sweep", "Add a Channel", "Add Drums",
             "Drum Accents", "Mixing", "Punch-in FX", "Undo", "Copy a Channel", "New Console", "Draw a Wave", "Odd Meters", "Long Pages", "Tempo Changes", "Glide", "Slide Steps", "Note Tails", "FX Latch", "Your First Song"]
sys.path.insert(0, os.path.join(ROOT, "tools"))
import masterclasses
def mc_action(items):                           # a masterclass, from the list its firmware entry is made from
    acts = masterclasses.actions(items)
    KN = [K1, K2, K3, K4]
    def run(r):
        for a in acts:
            if a[0] == "read": r.btn(OCTU)
            elif a[0] == "key": r.white(5)
            elif a[0] == "sel": [r.btn(SELB) for _ in range(a[1])]
            elif a[0] == "alg": r.encs(ALG, 1, a[1])
            elif a[0] == "knobs":
                for _ in range(abs(a[2])): r.ev(f"enc {KN[a[1] - 1]} {1 if a[2] > 0 else -1}", 60)
            elif a[0] == "py": exec(a[1], dict(globals(), r=r))
            r.step()
    return run
ACTION_BY_NAME = dict(zip(GEN_NAMES, [GENERAL[n] for n in GENERAL]))
ACTION_BY_NAME.update({name: chip_action(st) for name, _, st in chip_lessons.L})
ACTION_BY_NAME.update({name: mc_action(items) for _, name, _, items in masterclasses.MC})

def steps_of():
    src = open(os.path.join(ROOT, "firmware/src/lessons.c")).read()
    body = src[src.index("static const lesson_t LESSON[]"):src.index("#define LES_N")]
    body = re.sub(r'#include "(lessons_\w+\.h)"', lambda m: open(os.path.join(ROOT, "firmware/src", m.group(1))).read(), body)
    out = []
    for m in re.finditer(r'\{"([^"]+)", S_\w+, (\d+),', body): out.append((m.group(1), int(m.group(2))))
    return out

def run(binary, i, name, n):
    r = R()
    r.held(0x100, 70); r.wait(600)                                  # the quick start of a new device
    r.held(0x180, 100); r.wait(1200)                                # GLO + HOME: the menu, on its first row
    r.wait(300); r.btn(OCTU); r.wait(600)                           # the first row opens the list
    for _ in range(i): r.enc(SEL, 1, 70)
    r.wait(400); r.btn(OCTU); r.wait(1500)                          # start the lesson
    ACTION_BY_NAME[name](r)
    r.wait(2500)
    d = tempfile.mkdtemp()
    open(d + "/s.txt", "w").write("\n".join(r.L) + f"\n{r.t + 500} end\n")
    env = dict(os.environ, HOST_FLASH=d + "/flash.bin")
    out = subprocess.run([binary, d + "/o", d + "/s.txt"], env=env, capture_output=True, text=True, timeout=600).stdout
    for m in re.finditer(r"LESSON (\d+) STEP (\d+) INSTANT", out):
        if int(m.group(1)) == i: print(f"   INSTANT: lesson {i} step {m.group(2)} is already true when it shows")
    got = sorted(int(m.group(2)) for m in re.finditer(r"LESSON (\d+) STEP (\d+) OK", out) if int(m.group(1)) == i)
    return got

def main():
    args = sys.argv[1:]
    binary = args[0] if args and os.path.exists(args[0]) else None
    nums = [int(a) for a in args if a.isdigit()]
    if not binary:
        binary = tempfile.mkdtemp() + "/cb"
        subprocess.check_call(["cc", "-O2", "-w", "-DCHIPBOX_HOST", "-DPOOL=", "-D__attribute__(x)=", '-DAPP_VER="dev"', f"-I{ROOT}/firmware/src", f"-I{ROOT}/firmware/gen", "-o", binary, f"{ROOT}/firmware/src/chipbox.c", "-lm"])
    ls = steps_of()
    missing = [n for n, _ in ls if n not in ACTION_BY_NAME]
    assert not missing, missing
    bad = 0
    for i, (name, n) in enumerate(ls):
        if nums and i not in nums: continue
        got = run(binary, i, name, n)
        ok = got == list(range(n))
        bad += not ok
        print(f"{i:2} {name:20} {len(got)}/{n} steps" + ("" if ok else f"   MISSING {sorted(set(range(n)) - set(got))}"))
    print("all lessons finish" if not bad else f"{bad} lessons do not finish")
    sys.exit(1 if bad else 0)
if __name__ == "__main__": main()
