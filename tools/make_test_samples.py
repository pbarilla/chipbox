#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The TEST projects: builds tests/make_test_samples.c (the firmware's own engine and proj_pack), runs it, wraps the projects as .chipbox files
# in site/projects/samples/ (test-*.chipbox), adds them to samples.json (titles only; older Test: entries are replaced) and writes tests/test_samples.txt (what is on every channel and page).
# Nothing else in samples/ and not demo2.h is touched.   python3 tools/make_test_samples.py
import base64, json, subprocess, sys, tempfile
from pathlib import Path
root = Path(__file__).resolve().parent.parent
TITLES = [("test-fm-4op-a", "Test: FM 4-op A"), ("test-fm-4op-b", "Test: FM 4-op B"), ("test-fm-2op", "Test: FM 2-op"), ("test-psg-knobs", "Test: PSG knobs"),
          ("test-noise", "Test: noise"), ("test-sweep", "Test: sweep, vibrato, kits"), ("test-draw-waves", "Test: draw waves"), ("test-rack-mix", "Test: rack mix"),
          ("test-psg-knobs-2", "Test: PSG knobs 2"), ("test-5b-sid", "Test: 5B and SID")]
tmp = Path(tempfile.mkdtemp())
src = (root / "firmware/src/proj.c").read_text()
(tmp / "proj_core.h").write_text(src[src.index("static uint32_t proj_pack("):src.index("/* undo:")])
exe = tmp / "mts"
subprocess.run(["cc", "-O2", "-DPOOL=", "-D__attribute__(x)=", f"-I{tmp}", f"-I{root/'tests'}", f"-I{root/'firmware/src'}", f"-I{root/'firmware/gen'}",
                str(root / "tests/make_test_samples.c"), "-lm", "-o", str(exe)], check=True)
r = subprocess.run([str(exe), str(tmp)])
out = root / "site/projects/samples"
for fn, title in TITLES:
    data = (tmp / f"{fn}.bin").read_bytes()
    (out / f"{fn}.chipbox").write_text(json.dumps({"chipbox": 1, "projects": [{"name": title, "data": base64.b64encode(data).decode()}]}) + "\n")
sj = root / "site/projects/samples.json"
lst = [e for e in json.loads(sj.read_text()) if not e["name"].startswith("Test:")]
lst += [{"name": t, "file": f"samples/{fn}.chipbox"} for fn, t in TITLES]
sj.write_text(json.dumps(lst, indent=1) + "\n")
(root / "tests/test_samples.txt").write_text((tmp / "legend.txt").read_text())
print("wrote", len(TITLES), "projects")
sys.exit(r.returncode)
