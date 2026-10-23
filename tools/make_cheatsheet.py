#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The cheat sheet: site/cheatsheet/index.html, made from tools/controls.json (the one list of what each control does). The pages
workflow runs it on every deploy, so a release brings it up to date. Prints well (A4, landscape).
   python3 tools/make_cheatsheet.py [--version X.Y.Z]   (without it: site/firmware/latest.json, or "dev")"""
import html, json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
X0, Y0, W, H = 30, 82, 1040, 664
KNOBS = [("MASTER", 139, 187), ("SELECT", 253, 185), ("PRESETS", 139, 295), ("ALGORITHM", 253, 295),
         ("KNOB1", 665, 185), ("KNOB2", 772, 185), ("KNOB3", 880, 185), ("KNOB4", 988, 185)]
BTNS = [("OCT-", 147, 380, 84), ("OCT+", 249, 380, 84), ("FX", 663, 287, 58), ("SEL", 729, 287, 58), ("ENV", 794, 287, 58), ("LFO", 860, 287, 58),
        ("EDIT", 925, 287, 58), ("GLO", 990, 287, 58), ("HOME", 663, 352, 58), ("SAVE", 729, 352, 58), ("ARP", 794, 352, 58), ("SEQ", 860, 352, 58),
        ("PLAY", 925, 352, 58), ("REC", 990, 352, 58)]
BLACK = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25}
LABEL = {"KNOB1": "KNOB 1", "KNOB2": "KNOB 2", "KNOB3": "KNOB 3", "KNOB4": "KNOB 4"}


def version():
    if "--version" in sys.argv:
        return sys.argv[sys.argv.index("--version") + 1]
    try:
        return json.load(open(os.path.join(ROOT, "site", "firmware", "latest.json")))["version"]
    except Exception:
        return os.environ.get("CHIPBOX_VERSION", "dev")


def panel(short):
    o = ['<svg viewBox="%d %d %d %d" class="panel" role="img" aria-label="The FM-1 and what its controls do">' % (X0, Y0, W, H)]
    o.append('<rect x="32" y="84" width="1036" height="660" rx="46" class="body"/><rect x="342" y="127" width="246" height="246" rx="6" class="tray"/>')
    o.append('<rect x="90" y="349" width="218" height="59" rx="10" class="tray"/><rect x="621" y="247" width="416" height="146" rx="14" class="tray"/><rect x="67" y="435" width="966" height="280" rx="14" class="tray"/>')
    o.append('<rect x="361" y="145" width="208" height="208" class="screen"/><text x="465" y="257" class="screentext">240 x 240</text>')
    for name, cx, cy in KNOBS:
        o.append('<circle cx="%d" cy="%d" r="29" class="knob"/><line x1="%d" y1="%d" x2="%d" y2="%d" class="notch"/>' % (cx, cy, cx, cy - 6, cx, cy - 22))
        o.append('<text x="%d" y="%d" class="lab">%s</text>' % (cx, cy - 41, LABEL.get(name, name)))
        s = short.get(name, "")
        if s: o.append('<text x="%d" y="%d" class="role">%s</text>' % (cx, cy + 50, html.escape(s)))
    for name, cx, cy, w in BTNS:
        free = not short.get(name)
        o.append('<rect x="%g" y="%d" width="%d" height="42" rx="9" class="btn%s"/>' % (cx - w / 2, cy - 21, w, " free" if free else ""))
        o.append('<text x="%d" y="%d" class="btxt">%s</text>' % (cx, cy + 5, name))
        s = short.get(name, "")
        o.append('<text x="%d" y="%d" class="role">%s</text>' % (cx, cy + 36, html.escape(s)))
    wi = 0
    for k in range(27):
        if k in BLACK:
            cx = 109 + (wi - 0.5) * 58.7
            o.append('<rect x="%g" y="452" width="46" height="94" rx="23" class="key black"/>' % (cx - 23))
        else:
            cx = 109 + wi * 58.7
            o.append('<rect x="%g" y="553" width="46" height="148" rx="23" class="key"/>' % (cx - 23))
            wi += 1
    o.append('<text x="550" y="690" class="role big">27 keys, F3 to G5</text></svg>')
    return "".join(o)


def main():
    d = json.load(open(os.path.join(HERE, "controls.json")))
    views = d["views"]
    rows = []
    for c in d["controls"]:
        cells = "".join("<td>%s</td>" % (html.escape(c.get(v, "")) or '<span class="none">·</span>') for v in views)
        rows.append("<tr><th>%s</th>%s</tr>" % (html.escape(c["name"]), cells))
    pads = "".join('<span class="pad p%d">%s</span>' % (i // 4, html.escape(n)) for i, n in enumerate(d["pads"]))
    tips = "".join("<li>%s</li>" % html.escape(t) for t in d["tips"])
    page = '''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cheat sheet · Chipbox</title>
<style>
  :root { --bg:#0e0e12; --panel:#181820; --tray:#101016; --ink:#e8e8ee; --dim:#8a8a9a; --red:#eb4034; --line:#2a2a36; --key:#4a4a5a; --keyb:#3a3a48; color-scheme:dark; }
  * { box-sizing:border-box; }
  body { margin:0; background:var(--bg); color:var(--ink); font:14px/1.45 ui-monospace, "SF Mono", Menlo, Consolas, monospace; }
  main { max-width:1100px; margin:0 auto; padding:28px 16px 56px; }
  header { display:flex; flex-wrap:wrap; align-items:baseline; gap:6px 18px; margin:0 0 14px; }
  h1 { font-size:34px; margin:0; letter-spacing:.02em; }
  .tag { background:var(--red); color:#fff; font-size:12px; letter-spacing:.12em; padding:2px 9px; vertical-align:middle; margin-left:8px; }
  header span.v { color:var(--dim); }
  header nav { margin-left:auto; display:flex; gap:16px; }
  a { color:#46aaff; }
  .panel { width:100%; height:auto; display:block; margin:6px 0 18px; }
  .panel .body { fill:var(--panel); stroke:#3a3a4a; stroke-width:4; } .panel .tray { fill:var(--tray); } .panel .screen { fill:#000; }
  .panel .knob { fill:var(--bg); stroke:#3a3a4a; stroke-width:4; } .panel .notch { stroke:var(--ink); stroke-width:5; stroke-linecap:round; }
  .panel .btn { fill:#23232e; stroke:#3a3a4a; stroke-width:2; } .panel .btn.free { opacity:.45; }
  .panel .key { fill:var(--key); } .panel .key.black { fill:var(--keyb); }
  .panel text { text-anchor:middle; font-family:inherit; font-weight:700; }
  .panel .lab { fill:var(--dim); font-size:14px; letter-spacing:.08em; } .panel .btxt { fill:#c8c8d4; font-size:13px; }
  .panel .role { fill:#f0aa32; font-size:12px; } .panel .role.big { font-size:18px; fill:var(--dim); } .panel .screentext { fill:#444; font-size:16px; }
  table { border-collapse:collapse; width:100%; margin:6px 0 20px; font-size:13px; }
  th, td { text-align:left; padding:5px 10px 5px 0; border-bottom:1px solid var(--line); vertical-align:top; }
  thead th { color:var(--dim); font-weight:400; letter-spacing:.1em; font-size:12px; }
  tbody th { white-space:nowrap; font-weight:700; padding-right:14px; }
  .none { color:#444; }
  h2 { font-size:13px; letter-spacing:.14em; text-transform:uppercase; color:var(--dim); margin:22px 0 8px; font-weight:400; }
  .pads { display:grid; grid-template-columns:repeat(4, minmax(0, 1fr)); gap:6px; max-width:520px; }
  .pad { padding:8px 4px; text-align:center; border-radius:4px; font-weight:700; border:2px solid; }
  .p0 { border-color:#ebd23c; color:#ebd23c; } .p1 { border-color:#78c846; color:#78c846; } .p2 { border-color:#3cbeaa; color:#3cbeaa; } .p3 { border-color:#a06ee6; color:#a06ee6; }
  ul { margin:0; padding-left:20px; } li { margin:3px 0; }
  .tables { overflow-x:auto; }
  @media print {
    @page { size:A4 landscape; margin:9mm; }
    :root { --bg:#fff; --panel:#f1f1f4; --tray:#e2e2e8; --ink:#111; --dim:#555; --line:#bbb; --key:#cfcfd8; --keyb:#9a9aa8; color-scheme:light; }
    body { font-size:10.5px; } main { max-width:none; padding:0; } header nav { display:none; } h1 { font-size:24px; }
    .panel { width:62%; margin:0 auto 6px; } .panel .role { fill:#a35a08; } .panel .btn { fill:#fff; stroke:#888; } .panel .btxt { fill:#222; } .panel .screen { fill:#ddd; }
    .panel .body { stroke:#888; } .panel .knob { fill:#fff; stroke:#888; } table { font-size:9.5px; } th, td { padding:2px 6px 2px 0; }
    .none { color:#aaa; } .cols { display:grid; grid-template-columns:1fr 1fr; gap:14px; }
  }
</style>
</head>
<body>
<main>
  <header>
    <h1>Chipbox<span class="tag">CHEAT SHEET</span></h1>
    <span class="v">v@@VERSION@@</span>
    <nav><a href="../">Home</a><a href="../manual/">Tutorials</a><a href="../guide/">Manual</a><a href="../play/">Play online</a></nav>
  </header>
  @@PANEL@@
  <div class="tables"><table>
    <thead><tr><th></th>@@HEADS@@</tr></thead>
    <tbody>@@ROWS@@</tbody>
  </table></div>
  <div class="cols">
    <div><h2>FX pads</h2><div class="pads">@@PADS@@</div></div>
    <div><h2>Tips</h2><ul>@@TIPS@@</ul></div>
  </div>
</main>
</body>
</html>
'''
    for k, v in (('VERSION', html.escape(version())), ('PANEL', panel(d['short'])), ('HEADS', ''.join('<th>%s</th>' % v for v in views)), ('ROWS', ''.join(rows)), ('PADS', pads), ('TIPS', tips)):
        page = page.replace('@@%s@@' % k, v)
    out = os.path.join(ROOT, "site", "cheatsheet")
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, "index.html"), "w").write(page)
    print("wrote site/cheatsheet/index.html", version())


if __name__ == "__main__":
    main()
