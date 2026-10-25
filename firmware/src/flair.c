/* SPDX-License-Identifier: GPL-3.0-only */
/* Animations for no reason: top-bar scenes (a project opened, the octave, a channel picked, a view change, a critter
 * walking by when nothing happens).
 * Included by chipbox.c after fxview.c. */
#define CRIT_MS 20000u                          /* quiet this long: a critter walks across the bar */
static uint32_t act_ms;                         /* the last time anything was touched */

static void anim_cont(uint32_t kind, uint32_t arg, uint32_t dur)        /* an animation that goes on while you keep turning, instead of starting over */
{
    if (an_kind == kind && fm1_ms < an_end) { an_arg = arg; an_end = fm1_ms + dur; }
    else { an_kind = kind; an_t0 = fm1_ms; an_arg = arg; an_end = fm1_ms + dur; }
}

static void flair_frame(uint32_t kind, uint32_t ms, uint32_t arg, const theme_t *at)
{
    int32_t i, k;
    char n[8];
    switch (kind) {
    case AN_OPEN: {                                 /* a floppy slides in over the slot and drops on it, the slot's number beside it */
        int32_t tx = 12 + (int32_t)arg * 16 - 6, x = ms < 350u ? ease(ms, 350, 250, tx) : tx;
        int32_t dip = ms >= 350u && ms < 450u ? (int32_t)(ms - 350u) / 25 : ms >= 450u && ms < 550u ? (int32_t)(550u - ms) / 25 : 0;
        for (i = 0; i < 12; i++) cv_rect(12 + i * 16, 33, 11, 6, (uint32_t)i == arg && ms >= 350u ? sg_color((uint32_t)i) : RGB(58, 58, 72));
        ico_floppy(x, 3 + dip);
        if (ms >= 350u) {
            sparks(12 + (int32_t)arg * 16 + 5, 33, ms - 350u, 400, 26, sg_color(arg));
            if (arg >= PROJ_DEMOS) { fmt_u(n, arg - PROJ_DEMOS + 1u); cv_text(206, 5, &FONT_L, n, R_WHITE); } else cv_text(190, 14, &FONT_S, arg ? "DEMO 2" : "DEMO 1", R_WHITE);
        }
        break; }
    case AN_OCT: {                                  /* five octaves of keys, the one you are on lights, sliding from the one you left */
        int32_t hx = ease(ms, 220, 30 + (int32_t)(arg >> 4) * 32, 30 + (int32_t)(arg & 15u) * 32), o, v = (int32_t)(arg & 15u) - 2;
        for (o = 0; o < 5; o++) {
            int32_t x0 = 30 + o * 32, d = x0 > hx ? x0 - hx : hx - x0;
            uint16_t c = d < 16 ? at->acc : RGB(150, 150, 165);
            for (k = 0; k < 7; k++) cv_rect(x0 + k * 4, 9, 3, 24, c);
            for (k = 0; k < 6; k++) if (k != 2) cv_rect(x0 + k * 4 + 3 - 1, 9, 3, 14, RGB(30, 30, 38));
        }
        cv_rect(hx, 5, 28, 2, R_WHITE);
        n[0] = v < 0 ? '-' : '+'; n[1] = (char)('0' + (v < 0 ? -v : v)); n[2] = 0;
        if (v == 0) { n[0] = '0'; n[1] = 0; }
        cv_text(198, 5, &FONT_L, n, R_WHITE);
        break; }
    case AN_CHAN: {                                 /* the channel's console pops up with its name, a dot for each channel with yours lit */
        uint32_t idx = arg & 255u, chip = (arg >> 8) & 255u;
        const theme_t *ct = &THEME[chip];
        int32_t sp = chan_n ? 180 / (int32_t)chan_n : 14, x0;
        if (sp > 14) sp = 14;
        x0 = 120 - sp * (int32_t)chan_n / 2;
        for (i = 0; i < (int32_t)chan_n; i++) {
            uint32_t on = (uint32_t)i == idx;
            cv_rect(x0 + i * sp + (sp - 4) / 2 - (on ? 1 : 0), 37 - (on ? 1 : 0), on ? 5 : 3, on ? 5 : 3, on ? ct->acc : RGB(64, 64, 78));
        }
        {   char nm[12];                                /* its name: the kind, counted among the channels of that name (PU1, PU2, WAV1) */
            const char *kn = idx < chan_n ? KIND_NAME[chan[idx].kind] : "";
            uint32_t j = 0, nth = 1, q;
            while (*kn) nm[j++] = *kn++;
            for (q = 0; q < idx && q < chan_n; q++) { const char *a = KIND_NAME[chan[q].kind], *b = KIND_NAME[chan[idx].kind]; while (*a && *a == *b) { a++; b++; } if (!*a && !*b) nth++; }
            fmt_u(nm + j, nth);
            x0 = 120 - (console_w(chip) + 12 + text_w(&FONT_L, nm)) / 2;
            ico_system(chip, x0, console_y(chip), ms < 260u ? ease(ms, 260, -30, 0) : 0, 0);
            cv_text(x0 + console_w(chip) + 12, 3, &FONT_L, nm, R_WHITE);
        }
        break; }
    case AN_VIEW: {                                 /* a band of the view's colour sweeps open, names the view, and closes (arg 3: the punch-in layer) */
        static const uint16_t COL[5] = {RGB(240, 150, 40), RGB(60, 190, 170), RGB(150, 110, 235), RGB(235, 100, 170), RGB(70, 150, 255)};
        static const char *const WORD[5] = {"RACK", "CHIP", "SONG", "FX", "SEQ"};
        uint32_t v = arg == 1u ? 0u : arg == 0u ? 1u : arg == 2u ? 2u : arg == 4u ? 4u : 3u;
        if (ms < 140u) { int32_t w = ease(ms, 140, 0, 240); cv_rect(120 - w / 2, 0, w, HDR_H, COL[v]); }
        else if (ms < 440u) {
            int32_t tw = text_w(&FONT_L, WORD[v]), x0 = 120 - (tw + 26) / 2;
            cv_rect(0, 0, 240, HDR_H, COL[v]);
            if (v < 3u) ico_view(v, x0, 13, R_BG);
            else for (i = 0; i < 4; i++) cv_rect(x0 + (i & 1) * 9, 12 + (i >> 1) * 9, 7, 7, R_BG);
            cv_text(x0 + 26, 5, &FONT_L, WORD[v], R_BG);
        } else { int32_t h = ease(ms - 440u, 160, HDR_H, 0); cv_rect(0, (HDR_H - h) / 2, 240, h, COL[v]); }
        break; }
    case AN_WAVE: {                                 /* the wave you are shaping, drawn across the bar as a scope */
        uint8_t wv[32];
        uint32_t c = arg;
        if (c >= chan_n) break;
        wave_make(&chan[c].in, wv);
        for (i = 0; i < 64; i++) cv_rect(40 + i * 3, 36 - wv[(i / 2 + (int32_t)(ms / 60u)) & 31] * 28 / 15, 3, 3, at->acc);
        break; }
    case AN_UNDO: {                                 /* an eraser rubs out a scribble, crumbs falling; no scribble when there was nothing to undo */
        int32_t front = ms < 520u ? ease(ms, 520, 44, 196) : 196 + (int32_t)(ms - 520u) / 3, zx;
        uint16_t pk = RGB(244, 128, 150);
        if (arg) for (zx = 56; zx < 190; zx += 8) if (zx + 8 > front) {
            int32_t ya = (zx / 8) & 1 ? 12 : 31, yb = (zx / 8) & 1 ? 31 : 12;
            cv_line(zx, ya, zx + 8, yb, R_WHITE); cv_line(zx + 1, ya, zx + 9, yb, R_WHITE);
        }
        {   int32_t ey = 10 + (int32_t)((ms / 55u) & 1u) * 2, ex = front - 34;
            cv_rect(ex, ey, 34, 22, RGB(236, 236, 242)); cv_rect(ex + 14, ey, 6, 22, RGB(70, 120, 220));        /* the sleeve */
            cv_rect(ex + 20, ey, 14, 22, pk); cv_rect(ex + 20, ey, 14, 3, RGB(255, 170, 185));                  /* the rubber */
            cv_rect(ex, ey + 20, 34, 2, RGB(150, 150, 165));
            for (i = 0; i < 3; i++) cv_rect(ex + 30 - (int32_t)((ms / 40u + (uint32_t)i * 5u) % 12u), ey + 24 + (int32_t)((ms / 25u + (uint32_t)i * 7u) % 8u), 2, 2, pk);
        }
        break; }
    default: {                                      /* a critter walks by: a bug, a hopping note, a saucer, a cat, a cassette, a ghost */
        int32_t x = (int32_t)(ms * 280u / 4000u) - 20, y, mir = (int32_t)(arg & 1u);
        uint16_t c = sg_color(arg * 3u + 1u);
        uint32_t v = arg % 6u;
        if (mir) x = 240 - x;
        if (v == 0u) {                              /* chip bug */
            int32_t hop = ms % 1000u < 300u ? swing(ms % 1000u, 600) * 6 / 127 : 0;
            if (hop < 0) hop = -hop;
            y = 24 - hop;
            cv_rect(x, y, 14, 9, c); cv_rect(x + 2, y - 2, 10, 2, c);
            cv_rect(x + 3, y + 2, 3, 3, R_WHITE); cv_rect(x + 8, y + 2, 3, 3, R_WHITE);
            cv_rect(x + 4, y + 3, 1, 2, R_BG); cv_rect(x + 9, y + 3, 1, 2, R_BG);
            cv_line(x + 3, y - 2, x + 1, y - 6, c); cv_line(x + 10, y - 2, x + 12, y - 6, c);
            for (k = 0; k < 3; k++) { int32_t l = (int32_t)((ms / 90u + (uint32_t)k) & 1u) * 2; cv_rect(x + 2 + k * 4, y + 9, 1, 4 + l, c); cv_rect(x + 3 + k * 4, y + 9, 1, 3, c); }
        } else if (v == 1u) {                       /* a note hopping, a dotted trail behind */
            int32_t hop = swing(ms * 2u, 1000); if (hop < 0) hop = -hop;
            y = 31 - hop * 22 / 127;
            for (k = 1; k < 5; k++) cv_rect(x - (mir ? -k : k) * 9, 35, 2, 2, mix16(R_BG, c, 200u - (uint32_t)k * 40u));
            disc(x, y, 4, c); cv_rect(x + 3, y - 14, 2, 14, c); cv_rect(x + 3, y - 14, 7, 3, c);
        } else if (v == 2u) {                       /* a saucer, wobbling, a beam that flickers */
            y = 9 + swing(ms, 700) * 3 / 127;
            cv_rect(x + 4, y - 4, 12, 4, mix16(R_BG, c, 180)); cv_rect(x, y, 20, 4, c); cv_rect(x + 3, y + 4, 14, 2, mix16(R_BG, c, 150));
            cv_rect(x + 3, y + 1, 2, 2, R_WHITE); cv_rect(x + 9, y + 1, 2, 2, R_WHITE); cv_rect(x + 15, y + 1, 2, 2, R_WHITE);
            if ((ms / 120u) & 1u) for (k = 0; k < 6; k++) cv_rect(x + 5 - k, y + 6 + k * 3, 10 + 2 * k, 1, mix16(R_BG, c, 90));
        } else if (v == 3u) {                       /* a cat: it faces the way it walks, the tail swishes */
            int32_t sw = 22, step = (int32_t)((ms / 110u) & 1u), tw = (int32_t)((ms / 200u) & 1u);
#define CX(o, w) (mir ? x + sw - (o) - (w) : x + (o))
            y = 20;
            cv_rect(CX(2, 14), y + 4, 14, 8, c);                                                                   /* body */
            cv_rect(CX(13, 9), y, 9, 9, c);                                                                        /* head */
            cv_rect(CX(13, 2), y - 3, 2, 3, c); cv_rect(CX(20, 2), y - 3, 2, 3, c);                                /* ears */
            cv_rect(CX(18, 2), y + 3, 2, 2, R_WHITE); cv_rect(CX(19, 1), y + 3, 1, 2, R_BG);                       /* an eye */
            cv_rect(CX(0, 3), y + 3 - tw * 3, 3, 6 + tw * 3, c); cv_rect(CX(0, 4), y + 1 - tw * 3, 4, 3, c);       /* the tail */
            cv_rect(CX(3, 3), y + 12, 3, 5 - step, c); cv_rect(CX(7, 3), y + 12, 3, 4 + step, c);
            cv_rect(CX(11, 3), y + 12, 3, 5 - step, c); cv_rect(CX(15, 3), y + 12, 3, 4 + step, c);                 /* legs, alternating */
#undef CX
        } else if (v == 4u) {                       /* a cassette, reels turning, sliding along */
            int32_t sp = (int32_t)(ms / 45u);
            y = 16 + swing(ms, 900) * 2 / 127;
            cv_rect(x, y, 30, 20, RGB(70, 72, 84)); cv_rect(x + 2, y + 2, 26, 8, RGB(226, 226, 232)); cv_rect(x + 4, y + 3, 16, 2, c);
            cv_rect(x + 8, y + 12, 14, 6, RGB(30, 30, 38));
            for (k = 0; k < 2; k++) {
                int32_t rx = x + 8 + k * 14, ry = y + 15;
                disc(rx, ry, 3, RGB(200, 200, 208));
                cv_line(rx, ry, rx + VIB[(sp + 8) & 31] * 3 / 127, ry + VIB[sp & 31] * 3 / 127, RGB(40, 40, 50));
            }
        } else {                                    /* a ghost, floating, looking where it goes */
            int32_t look = mir ? -1 : 1, fl = (int32_t)((ms / 140u) & 1u);
            y = 12 + swing(ms, 800) * 4 / 127;
            cv_rect(x + 2, y, 10, 2, c); cv_rect(x, y + 2, 14, 12, c);
            cv_rect(x, y + 14, 4, 3 - fl, c); cv_rect(x + 5, y + 14, 4, 2 + fl, c); cv_rect(x + 10, y + 14, 4, 3 - fl, c);
            cv_rect(x + 3, y + 5, 3, 4, R_WHITE); cv_rect(x + 8, y + 5, 3, 4, R_WHITE);
            cv_rect(x + 4 + (look > 0 ? 1 : 0), y + 7, 2, 2, RGB(30, 40, 160)); cv_rect(x + 9 + (look > 0 ? 1 : 0), y + 7, 2, 2, RGB(30, 40, 160));
        }
        break; }
    }
}
