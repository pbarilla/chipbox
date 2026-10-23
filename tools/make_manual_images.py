#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The panel pictures for the tutorial: the FM-1 drawn as the Play online page draws it, with a control ringed and, if you like, a
screenshot in the screen. Writes site/manual/img/panel-*.png. Needs Pillow.   python3 tools/make_manual_images.py"""
import os
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(__file__), "..", "site", "manual", "img")
X0, Y0, W, H = 30, 82, 1040, 664                 # the page's viewBox
KNOBS = {"MASTER": (139, 187), "SELECT": (253, 185), "PRESETS": (139, 295), "ALGORITHM": (253, 295),
         "KNOB1": (665, 185), "KNOB2": (772, 185), "KNOB3": (880, 185), "KNOB4": (988, 185)}
BTNS = {"OCT-": (147, 380, 84), "OCT+": (249, 380, 84), "FX": (663, 287, 58), "SEL": (729, 287, 58), "ENV": (794, 287, 58), "LFO": (860, 287, 58),
        "EDIT": (925, 287, 58), "GLO": (990, 287, 58), "HOME": (663, 352, 58), "SAVE": (729, 352, 58), "ARP": (794, 352, 58), "SEQ": (860, 352, 58),
        "PLAY": (925, 352, 58), "REC": (990, 352, 58)}
BLACK = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25}
INK, DIM, ACC = (232, 232, 238), (138, 138, 154), (240, 170, 50)
FONT = "/System/Library/Fonts/Menlo.ttc"


def font(sz):
    try:
        return ImageFont.truetype(FONT, sz, index=1)
    except Exception:
        return ImageFont.load_default()


def key_box(k):
    wi = sum(1 for j in range(k) if j not in BLACK)
    if k in BLACK:
        cx = 109 + (wi - 0.5) * 58.7
        return (cx - 23, 452, cx + 23, 546)
    cx = 109 + wi * 58.7
    return (cx - 23, 553, cx + 23, 701)


def panel(high=(), screen=None, label=None, keys=(), width=960, lit=(), crop=True):
    S = 2.0 * width / W                            # draw at twice the size, then shrink
    im = Image.new("RGB", (int(W * S), int(H * S)), (14, 14, 18))
    d = ImageDraw.Draw(im)
    P = lambda x, y: ((x - X0) * S, (y - Y0) * S)
    def rect(x1, y1, x2, y2, r, fill, outline=None, w=0):
        a, b = P(x1, y1), P(x2, y2)
        d.rounded_rectangle([a, b], radius=r * S, fill=fill, outline=outline, width=int(w * S))
    rect(32, 84, 1068, 744, 46, (24, 24, 32), (58, 58, 74), 4)
    rect(342, 127, 588, 373, 6, (10, 10, 14))
    rect(90, 349, 308, 408, 10, (16, 16, 22)); rect(621, 247, 1037, 393, 14, (16, 16, 22)); rect(67, 435, 1033, 715, 14, (16, 16, 22))
    if screen is not None:
        sc = screen.convert("RGB").resize((int(208 * S), int(208 * S)), Image.NEAREST)
        im.paste(sc, tuple(int(v) for v in P(361, 145)))
    else:
        rect(361, 145, 569, 353, 2, (0, 0, 0))
    f = font(int(15 * S))
    for name, (cx, cy) in KNOBS.items():
        a, b = P(cx - 29, cy - 29), P(cx + 29, cy + 29)
        d.ellipse([a, b], fill=(14, 14, 18), outline=(58, 58, 74), width=int(4 * S))
        d.line([P(cx, cy - 6), P(cx, cy - 22)], fill=INK, width=int(5 * S))
        tw = d.textlength(name, font=f)
        d.text((P(cx, 0)[0] - tw / 2, P(0, cy - 53)[1]), name, font=f, fill=(122, 122, 134))
    for name, (cx, cy, w) in BTNS.items():
        on = name in lit
        rect(cx - w / 2, cy - 21, cx + w / 2, cy + 21, 9, (240, 70, 50) if on else (35, 35, 46), (58, 58, 74), 2)
        tw = d.textlength(name, font=f)
        d.text((P(cx, 0)[0] - tw / 2, P(0, cy)[1] - 8 * S), name, font=f, fill=(255, 255, 255) if on else (184, 184, 200))
    for k in range(27):
        x1, y1, x2, y2 = key_box(k)
        rect(x1, y1, x2, y2, 23, (240, 70, 50) if k in lit else ((58, 58, 72) if k in BLACK else (74, 74, 90)))
    for k in keys:                                  # keys to point at
        x1, y1, x2, y2 = key_box(k)
        rect(x1, y1, x2, y2, 23, ACC)
    boxes = []
    for h in high:
        if h in KNOBS:
            cx, cy = KNOBS[h]; boxes.append((cx - 46, cy - 46, cx + 46, cy + 46, 46))
        elif h in BTNS:
            cx, cy, w = BTNS[h]; boxes.append((cx - w / 2 - 12, cy - 33, cx + w / 2 + 12, cy + 33, 16))
        elif h == "keys":
            boxes.append((60, 428, 1040, 722, 18))
        elif h == "screen":
            boxes.append((336, 121, 594, 379, 12))
    for x1, y1, x2, y2, r in boxes:
        rect(x1, y1, x2, y2, r, None, ACC, 7)
    out = im.resize((width, int(H * width / W)), Image.LANCZOS)
    if crop:                                        # the top of the panel only: knobs, buttons, screen
        out = out.crop((0, 0, width, int((424 - Y0) * width / W)))
    if label:
        full = Image.new("RGB", (out.width, out.height + 40), (14, 14, 18))
        full.paste(out, (0, 0))
        ImageDraw.Draw(full).text((14, out.height + 8), label, font=font(20), fill=ACC)
        out = full
    return out


def shot(n):
    p = os.path.join(OUT, n + ".png")
    return Image.open(p) if os.path.exists(p) else None


# (name, controls ringed, screen, keys lit orange, crop to the top)
JOBS = [
    ("basics-projects", [], "b_picker", (), True), ("basics-pages", [], "b_rack", (), True), ("basics-song", [], "b_song", (), True),
    ("s1-hold-home", ["HOME"], "b_picker", (), True), ("s1-select", ["SELECT"], "m_picker_sel", (), True), ("s1-open", ["HOME"], "m_rack_blank", (), True),
    ("s1-delete", ["EDIT"], "m_delete", (), True), ("s1-menu", ["GLO", "HOME"], "m_menu", (), True),
    ("s2-rack", ["HOME"], "m_rack_blank2", (), True), ("s2-presets", ["PRESETS"], "m_rack_blank2", (), True), ("s2-octplus", ["OCT+"], "m_add_list", (), True),
    ("s2-scroll", ["SELECT", "PRESETS"], "m_add_list2", (), True), ("s2-added", [], "m_four", (), True),
    ("s3-tempo", ["KNOB1"], "m_tempo_anim", (), True),
    ("s4-song", ["HOME"], "m_song", (), True), ("s4-pages", ["OCT+"], "m_song7", (), True),
    ("e-seq", ["SEQ"], "m_rack_ready", (), True), ("e-rec", ["REC"], "m_rack_ready", (), True),
    ("q-presets", ["PRESETS"], "m_presets", (), True), ("q-algorithm", ["ALGORITHM"], "m_page1", (), True), ("q-seq", ["SEQ"], "m_seq_empty", (), True),
    ("q-key", [], "m_seq_note1", (), False), ("q-notes", [], "m_seq_notes", (), False), ("q-select", ["SELECT"], "m_seq_cursor", (), True),
    ("q-len", ["KNOB2"], "m_seq_len", (), True),
    ("q-edit", ["EDIT"], "m_seq_clear", (), True), ("q-play", ["PLAY"], "m_seq_play", (), True), ("q-page", ["ALGORITHM"], "m_seq_page", (), True),
    ("q-oct", ["OCT-", "OCT+"], "m_seq_oct", (), True), ("q-home", ["HOME"], "m_rack_notes", (), True),
    ("r-rec", ["REC"], "m_rec_deck", (), True), ("r-play", [], "m_rec_play", (), False), ("r-slow", ["SELECT"], "m_rec_tape", (), True),
    ("u-lfo", ["LFO"], "m_undo", (), True),
    ("b-oct", ["OCT-"], "m_bass_oct", (), True), ("b-key", [], "m_bass_seq", (), False),
    ("st-oct", ["OCT+"], "m_stabs_oct", (), True), ("st-keys", [], "m_stabs_seq", (), False),
    ("t-chip", ["HOME"], "m_chip_sms", (), True), ("t-vol", ["KNOB2"], "m_chip_sms_vol", (), True),
    ("c-knobs", ["KNOB1", "KNOB2", "KNOB3", "KNOB4"], "m_chip_sms", (), True), ("c-algo", ["ALGORITHM"], "m_chip_env", (), True),
    ("z-home", ["HOME"], "m_song_before", (), True), ("z-slot8", ["OCT+", "PRESETS"], "m_song_slot8", (), True), ("z-play", ["PLAY"], "m_song_play", (), True),
    ("a-picker", ["HOME"], "am_picker", (), True), ("a-slot", ["SELECT"], "am_slot", (), True), ("a-tempo", ["KNOB1"], "am_tempo", (), True),
    ("a-cons", ["OCT+"], "am_cons", (), True), ("a-sub", ["OCT+"], "am_sub", (), True), ("a-lanes0", ["SEQ"], "am_lanes0", (), True),
    ("a-ch0", [], "am_ch0", (17,), False), ("a-hats", [], "am_hats", (0, 4, 7, 11, 14, 18, 21, 24), False),
    ("a-sd", [], "am_sd", (3, 7, 12, 14, 21, 26), False), ("a-bd", [], "am_bd", (1, 0, 4, 18, 19), False),
    ("a-play", ["PLAY"], "am_play", (), True), ("a-song", ["HOME"], "am_song", (), True), ("a-song2", ["OCT+", "PRESETS"], "am_song2", (), True),
    ("a-copy", ["EDIT"], "am_copy", (), True), ("a-bar3", [], "am_bar3", (), False), ("a-bar4", [], "am_bar4", (), False), ("a-final", ["PLAY"], "am_final", (), True),
    ("x-fx", ["FX"], "x_fx", (), True), ("x-tape", [], "x_tape", (), False), ("x-glo", ["GLO"], "m_help", (), True),
]


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, high, scr, keys, crop in JOBS:
        im = shot(scr) if scr else None
        panel(high, im, None, keys, crop=crop).save(os.path.join(OUT, "pn-" + name + ".png"), optimize=True)
    print("wrote", len(JOBS), "pictures")


if __name__ == "__main__":
    main()
