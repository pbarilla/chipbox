# SPDX-License-Identifier: GPL-3.0-only
# Compare two dumps of tests/fm_ab.c (kind by kind, ended by a block of 0x7f7f7f7f): the level of each and the error between them, in dB.
#   python3 tests/fm_ab.py before.bin after.bin
import sys, numpy as np
def kinds(p):
    a = np.fromfile(p, dtype=np.int32).reshape(-1, 256); out = []; cur = []
    for b in a:
        if b[0] == 0x7f7f7f7f: out.append(np.concatenate(cur).astype(np.float64)); cur = []
        else: cur.append(b)
    return out
A, B = kinds(sys.argv[1]), kinds(sys.argv[2])
for i, (a, b) in enumerate(zip(A, B)):
    pa, pb = np.sqrt(np.mean(a * a)) + 1e-9, np.sqrt(np.mean(b * b)) + 1e-9
    e = np.sqrt(np.mean((a - b) ** 2)) + 1e-9
    print("kind %d: level %.1f -> %.1f dB (%+.3f), error %.1f dB below the signal" % (i, 20 * np.log10(pa), 20 * np.log10(pb), 20 * np.log10(pb / pa), 20 * np.log10(pa / e)))
