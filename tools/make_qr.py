#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The QR code on the Quick Start's third page: the manual's address as a bit matrix in firmware/gen/qr.h.
Needs segno (pip install segno).   python3 tools/make_qr.py"""
import os
import segno

URL = "https://ihavenocluewhatimdoing.com/chipbox/guide/"
OUT = os.path.join(os.path.dirname(__file__), "..", "firmware", "gen", "qr.h")

q = segno.make(URL, error="L", micro=False)
rows = [[1 if c else 0 for c in r] for r in q.matrix]
n = len(rows)
with open(OUT, "w") as f:
    f.write("/* SPDX-License-Identifier: GPL-3.0-only */\n/* Written by tools/make_qr.py: %s */\n#define QR_N %d\n" % (URL, n))
    f.write("static const uint32_t QR_ROW[QR_N] = {\n")
    for r in rows:
        f.write("    0x%08xu,\n" % sum(b << (n - 1 - i) for i, b in enumerate(r)))
    f.write("};\n")
print("version", q.version, n, "x", n)
