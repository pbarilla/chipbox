#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Builds and runs tests/proj_fuzz.c under the sanitizers: proj_pack / proj_check / proj_unpack are cut out of proj.c (the rest of that file is flash code).
import re, subprocess, sys, tempfile
from pathlib import Path
root = Path(__file__).resolve().parent.parent
src = (root / "firmware/src/proj.c").read_text()
i = src.index("static uint32_t proj_pack(")
j = src.index("/* undo:")
d = Path(tempfile.mkdtemp())
(d / "proj_core.h").write_text(src[i:j])
cmd = ["cc", "-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined", "-DPOOL=", "-D__attribute__(x)=", f"-I{d}", f"-I{root/'tests'}", f"-I{root/'firmware/src'}", f"-I{root/'firmware/gen'}",
       str(root / "tests/proj_fuzz.c"), "-lm", "-o", str(d / "pf")]
subprocess.check_call(cmd)
sys.exit(subprocess.call([str(d / "pf")]))
