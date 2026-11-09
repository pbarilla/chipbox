#!/usr/bin/env python3
# Unpacks the site's sample projects (.chipbox JSON) and the two demos into raw payload files, builds tests/sample_levels.c and runs it (levels per project).
import json, base64, subprocess, sys, tempfile, re
from pathlib import Path
root = Path(__file__).resolve().parent.parent
tmp = Path(tempfile.mkdtemp())
files = []
for p in sorted((root / "site/projects/samples").glob("*.chipbox")):
    for i, pr in enumerate(json.loads(p.read_text())["projects"]):
        q = tmp / f"{p.stem}-{i}.bin"; q.write_bytes(base64.b64decode(pr["data"])); files.append(str(q))
d = (root / "firmware/gen/demo2.h").read_text()
m = re.search(r"\{([^}]*)\}", d)
if m:
    q = tmp / "DEMO2.bin"; q.write_bytes(bytes(int(x, 0) for x in m.group(1).replace("\n", " ").split(",") if x.strip())); files.append(str(q))
src = (root / "firmware/src/proj.c").read_text()
(tmp / "proj_core.h").write_text(src[src.index("static uint32_t proj_pack("):src.index("/* undo:")])
exe = tmp / "sample_levels"
subprocess.run(["cc", "-O2", "-DPOOL=", "-D__attribute__(x)=", f"-I{tmp}", f"-I{root/'tests'}", f"-I{root/'firmware/src'}", f"-I{root/'firmware/gen'}", str(root / "tests/sample_levels.c"), "-lm", "-o", str(exe)], check=True)
subprocess.run([str(exe)] + files)
