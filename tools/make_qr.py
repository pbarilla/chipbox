#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The QR codes in the Quick Start and the tips: the addresses as bit matrices in firmware/gen/qr.h (all 29 x 29, version 3).
Needs segno (pip install segno).   python3 tools/make_qr.py"""
import os
import segno

BASE = "https://ihavenocluewhatimdoing.com/chipbox/"
URLS = [("QR_GUIDE", BASE + "guide/"), ("QR_PROJ", BASE + "projects/"), ("QR_GIT", "https://github.com/pbarilla/chipbox"),
        ("QR_PLAY", BASE + "play/"), ("QR_SITE", BASE)]
OUT = os.path.join(os.path.dirname(__file__), "..", "firmware", "gen", "qr.h")

with open(OUT, "w") as f:
    f.write("/* SPDX-License-Identifier: GPL-3.0-only */\n/* Written by tools/make_qr.py */\n#define QR_N 29\n")
    for i, (name, _) in enumerate(URLS):
        f.write("#define %s %d\n" % (name, i))
    f.write("static const uint32_t QR_ROW[%d][QR_N] = {\n" % len(URLS))
    for name, url in URLS:
        q = segno.make(url, error="L", micro=False)
        rows = [[1 if c else 0 for c in r] for r in q.matrix]
        assert len(rows) == 29, (url, q.version)
        f.write("    {/* %s */\n" % url)
        for r in rows:
            f.write("        0x%08xu,\n" % sum(b << (28 - i) for i, b in enumerate(r)))
        f.write("    },\n")
    f.write("};\n")
print("wrote", len(URLS), "codes")
