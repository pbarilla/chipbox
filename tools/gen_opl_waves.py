# SPDX-License-Identifier: GPL-3.0-only
# Writes firmware/gen/opl_waves.h: the OPL waves 1..3 as 1024 entry tables from SIN_T (what fm4_wave() computes), so the renderer reads them like the sine.
import re
h = open("firmware/gen/chip_tables.h").read()
i = h.index("SIN_T[1024] = {")
body = h[i:h.index("};", i)]
v = [int(x) for x in re.findall(r"-?\d+", body[body.index("{") + 1:])]
assert len(v) == 1024
def tab(f): return ",".join(str(f(i, v[i])) for i in range(1024))
open("firmware/gen/opl_waves.h", "w").write("""/* SPDX-License-Identifier: GPL-3.0-only */
/* The OPL family's waves 1..3 as tables (half sine, absolute sine, quarter pulse sine): written by tools/gen_opl_waves.py from SIN_T, equal to fm4_wave() at p = i << 22. */
static const int16_t OPL_W[3][1024] = {
{%s},
{%s},
{%s}};
""" % (tab(lambda i, s: s if s > 0 else 0), tab(lambda i, s: abs(s)), tab(lambda i, s: 0 if (i >> 8) & 1 else abs(s))))
