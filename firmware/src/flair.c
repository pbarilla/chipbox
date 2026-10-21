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
        int32_t tx = 30 + (int32_t)arg * 20 - 6, x = ms < 350u ? ease(ms, 350, 250, tx) : tx;
        int32_t dip = ms >= 350u && ms < 450u ? (int32_t)(ms - 350u) / 25 : ms >= 450u && ms < 550u ? (int32_t)(550u - ms) / 25 : 0;
        for (i = 0; i < 8; i++) cv_rect(30 + i * 20, 33, 14, 6, (uint32_t)i == arg && ms >= 350u ? sg_color((uint32_t)i) : RGB(58, 58, 72));
        ico_floppy(x, 3 + dip);
        if (ms >= 350u) {
            sparks(30 + (int32_t)arg * 20 + 7, 33, ms - 350u, 400, 26, sg_color(arg));
            fmt_u(n, arg + 1u); cv_text(206, 5, &FONT_L, n, R_WHITE);
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
        uint32_t idx = arg & 255u, chip = (arg >> 8) & 3u;
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
        static const uint16_t COL[4] = {RGB(240, 150, 40), RGB(60, 190, 170), RGB(150, 110, 235), RGB(235, 100, 170)};
        static const char *const WORD[4] = {"RACK", "CHIP", "SONG", "FX"};
        uint32_t v = arg == 1u ? 0u : arg == 0u ? 1u : arg == 2u ? 2u : 3u;
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
    case AN_LARP: {                                 /* the live arp: a bar for each key held, by pitch, a ball on the one sounding; ARP and its state when it was just switched */
        cv_text(100, 5, &FONT_L, "ARP", arp_live ? RGB(80, 220, 120) : RGB(235, 90, 80));
        for (i = 0; i < 4; i++) { int32_t h = arp_live ? 4 + (int32_t)((ms / 60u + (uint32_t)i * 3u) % 8u) * 2 : 3; cv_rect(166 + i * 12, 34 - h, 8, h, arp_live ? RGB(80, 220, 120) : RGB(90, 90, 108)); }
        break; }
    default: {                                      /* a critter walks by: a bug, a hopping note, a saucer */
        int32_t x = (int32_t)(ms * 280u / 4000u) - 20, y;
        uint16_t c = sg_color(arg * 3u + 1u);
        if (arg & 1u) x = 240 - x;
        if ((arg % 3u) == 0u) {                     /* chip bug */
            int32_t hop = ms % 1000u < 300u ? swing(ms % 1000u, 600) * 6 / 127 : 0;
            if (hop < 0) hop = -hop;
            y = 24 - hop;
            cv_rect(x, y, 14, 9, c); cv_rect(x + 2, y - 2, 10, 2, c);
            cv_rect(x + 3, y + 2, 3, 3, R_WHITE); cv_rect(x + 8, y + 2, 3, 3, R_WHITE);
            cv_rect(x + 4, y + 3, 1, 2, R_BG); cv_rect(x + 9, y + 3, 1, 2, R_BG);
            cv_line(x + 3, y - 2, x + 1, y - 6, c); cv_line(x + 10, y - 2, x + 12, y - 6, c);
            for (k = 0; k < 3; k++) { int32_t l = (int32_t)((ms / 90u + (uint32_t)k) & 1u) * 2; cv_rect(x + 2 + k * 4, y + 9, 1, 4 + l, c); cv_rect(x + 3 + k * 4, y + 9, 1, 3, c); }
        } else if ((arg % 3u) == 1u) {              /* a note hopping, a dotted trail behind */
            int32_t hop;
            hop = swing(ms * 2u, 1000); if (hop < 0) hop = -hop;
            y = 31 - hop * 22 / 127;
            for (k = 1; k < 5; k++) cv_rect(x - (arg & 1u ? -k : k) * 9, 35, 2, 2, mix16(R_BG, c, 200u - (uint32_t)k * 40u));
            disc(x, y, 4, c); cv_rect(x + 3, y - 14, 2, 14, c); cv_rect(x + 3, y - 14, 7, 3, c);
        } else {                                    /* a saucer, wobbling, a beam that flickers */
            y = 9 + swing(ms, 700) * 3 / 127;
            cv_rect(x + 4, y - 4, 12, 4, mix16(R_BG, c, 180)); cv_rect(x, y, 20, 4, c); cv_rect(x + 3, y + 4, 14, 2, mix16(R_BG, c, 150));
            cv_rect(x + 3, y + 1, 2, 2, R_WHITE); cv_rect(x + 9, y + 1, 2, 2, R_WHITE); cv_rect(x + 15, y + 1, 2, 2, R_WHITE);
            if ((ms / 120u) & 1u) for (k = 0; k < 6; k++) cv_rect(x + 5 - k, y + 6 + k * 3, 10 + 2 * k, 1, mix16(R_BG, c, 90));
        }
        break; }
    }
}
