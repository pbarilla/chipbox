#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Random input for a host sim (the same script language in all three firmwares): python3 tests/fuzz.py SEED SECONDS > script.txt
# Buttons, held chords, key masks, knobs, now and then a long quiet stretch. The sanitizer builds must stay silent under it.
import random, sys
seed, secs = int(sys.argv[1]), int(sys.argv[2])
r = random.Random(seed)
out = ["# fuzz seed %d, %d s" % (seed, secs)]
t = 300
BTN = [1 << i for i in range(16)]
while t < secs * 1000:
    k = r.random()
    if k < 0.22:
        out.append("%d btn %d" % (t, r.randrange(16)))
    elif k < 0.40:
        m = r.choice(BTN) | (r.choice(BTN) if r.random() < 0.3 else 0)
        out.append("%d held 0x%x" % (t, m)); t += r.randrange(20, 900); out.append("%d held 0" % t)
    elif k < 0.70:
        m = 0
        for _ in range(r.randrange(0, 5)): m |= 1 << r.randrange(27)
        out.append("%d keys 0x%x" % (t, m)); t += r.randrange(10, 400); out.append("%d keys 0" % t)
    else:
        out.append("%d enc %d %d" % (t, r.randrange(7), r.randrange(-12, 13)))
    t += r.randrange(5, 120)
    if r.random() < 0.01: t += r.randrange(1000, 6000)
out.append("%d end" % (t + 500))
print("\n".join(out))
