/* SPDX-License-Identifier: GPL-3.0-only */
/* The punch-in screen (the FX button): 16 pads in a 4 x 4 grid, in the order of the white keys, a colour for each family of effects.
 * A pad lights while its key is held; each effect has its own animation in the top bar. Included by chipbox.c after header.c. */
static const uint8_t FX_KEY[FXN] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};     /* the white keys, F3 up */
static const uint8_t FX_GRP[FXN] = {0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 5, 7, 7};              /* tape, repeat, filter, echo + reverb, lo-fi, gate + solo, pitch, kill + latch */
static const char *const FX_NAME[FXN] = {"TAPE", "RPT", "RPT", "RPT", "LPF", "HPF", "ECHO", "VERB", "CRSH", "DECI", "GATE", "DIVE", "WOB", "SOLO", "KILL", "HOLD"};
static uint32_t fx_layer, fx_view_force, fx_raw, fx_tog, fx_lat;       /* fx_raw: keys down; fx_tog, fx_lat: the pads switched on and the latch mode */
static void fx_off(void) { fx_raw = fx_tog = fx_lat = 0; fx_apply(0); }

static void fx_icon(uint32_t i, int32_t x, int32_t y, uint16_t c)            /* a pad's picture, in 16 x 16 at x, y */
{
    int32_t k;
    switch (i) {
    case FX_TAPE: disc(x + 3, y + 6, 3, c); disc(x + 13, y + 6, 3, c); cv_rect(x + 3, y + 11, 11, 2, c); break;
    case FX_R8: case FX_R16: case FX_R32: { int32_t n = i == FX_R8 ? 2 : i == FX_R16 ? 3 : 5; for (k = 0; k < n; k++) cv_rect(x + 1 + k * 15 / n, y + 2, 2, 12, c); break; }
    case FX_LPF: cv_rect(x, y + 3, 8, 2, c); cv_line(x + 8, y + 4, x + 15, y + 14, c); cv_line(x + 9, y + 4, x + 16, y + 14, c); break;
    case FX_HPF: cv_rect(x + 8, y + 3, 8, 2, c); cv_line(x + 8, y + 4, x, y + 14, c); cv_line(x + 9, y + 4, x + 1, y + 14, c); break;
    case FX_ECHO: cv_rect(x, y + 2, 5, 12, c); cv_rect(x + 7, y + 4, 4, 8, mix16(R_BG, c, 170)); cv_rect(x + 13, y + 6, 3, 4, mix16(R_BG, c, 100)); break;
    case FX_VERB: for (k = 0; k < 4; k++) cv_rect(x + k * 4, y + 8 - (4 - k) * 2 + (k & 1), 2, 4 + (4 - k) * 4 - (k & 1) * 2, mix16(R_BG, c, 255 - (uint32_t)k * 50)); break;
    case FX_CRUSH: for (k = 0; k < 4; k++) cv_rect(x + k * 4, y + (k == 0 || k == 3 ? 8 : 2) + (k == 3 ? 0 : 0), 4, 4 + (k == 1 || k == 2 ? 0 : 4), c); break;
    case FX_DECI: for (k = 0; k < 5; k++) cv_rect(x + k * 3 + 1, y + 7 - VIB[(k * 6) & 31] * 6 / 127, 2, 2, c); break;
    case FX_GATE: cv_rect(x, y + 3, 4, 10, c); cv_rect(x + 8, y + 3, 4, 10, c); cv_rect(x, y + 13, 16, 2, c); break;
    case FX_DIVE: cv_rect(x + 6, y + 1, 4, 9, c); for (k = 0; k < 5; k++) cv_rect(x + 3 + k, y + 9 + k, 10 - 2 * k, 1, c); break;
    case FX_WOB: for (k = 0; k < 16; k++) cv_rect(x + k, y + 7 + VIB[(k * 2) & 31] * 5 / 127, 1, 2, c); break;
    case FX_SOLO: cv_rect(x + 5, y + 2, 6, 12, c); cv_rect(x, y + 6, 3, 4, mix16(R_BG, c, 90)); cv_rect(x + 13, y + 6, 3, 4, mix16(R_BG, c, 90)); break;
    case FX_LATCH: cv_rect(x + 3, y + 7, 10, 8, c); cv_rect(x + 5, y + 2, 2, 6, c); cv_rect(x + 9, y + 2, 2, 6, c); cv_rect(x + 5, y + 2, 6, 2, c); break;
    default: ico_cross(x + 1, y + 1, 12, c); break;                                         /* kill */
    }
}

/* the bar's animation for a pad: ms since it was pressed, reels turn with the tape's speed */
static void fx_anim_frame(uint32_t pad, uint32_t ms)
{
    uint16_t c = sg_color(FX_GRP[pad]);
    int32_t i, k;
    switch (pad) {
    case FX_TAPE: {                                                                  /* the tape: reels turn at the speed the dial gives, a needle on the scale */
        int32_t r = seq_run ? (int32_t)seq_rate : 2, spin = (int32_t)(fm1_ms / 45u) * (seq_hold ? 0 : r == 0 ? 0 : r), px = 120 + (r - 0) * 0;
        reel(34, 21, spin); reel(206, 21, spin);
        cv_line(34, 36, 106, 39, RGB(90, 62, 44)); cv_line(206, 36, 134, 39, RGB(90, 62, 44));
        cv_rect(106, 36, 28, 5, RGB(150, 152, 162));
        for (i = -3; i <= 3; i++) cv_rect(120 + i * 12 - 1, 6, 2, i == 0 ? 8 : 5, RGB(150, 152, 162));
        px = 120 + r * 12 - 24;
        cv_rect(px - 2, 4, 4, 16, c);
        break; }
    case FX_R8: case FX_R16: case FX_R32: {                                          /* the same block over and over, faster for the shorter repeat */
        uint32_t per = (pad == FX_R8 ? 250u : pad == FX_R16 ? 125u : 62u);
        uint32_t cnt = ms / per;
        for (i = 0; i < 9; i++) cv_rect(14 + i * 24, 8, 18, 26, (uint32_t)i < cnt % 10u ? c : mix16(R_BG, c, 60));
        break; }
    case FX_LPF: case FX_HPF: {                                                      /* the filter's curve: the knee moves as the sweep goes */
        int32_t kx = pad == FX_LPF ? 200 - (int32_t)fx_lp_i * 9 : 40 + (int32_t)fx_hp_i * 9;
        cv_rect(20, 36, 200, 1, RGB(70, 70, 85));
        if (pad == FX_LPF) { cv_rect(20, 12, kx - 20, 3, c); cv_line(kx, 13, kx + 22, 36, c); cv_line(kx, 14, kx + 22, 37, c); }
        else { cv_rect(kx, 12, 220 - kx, 3, c); cv_line(kx, 13, kx - 22, 36, c); cv_line(kx, 14, kx - 22, 37, c); }
        break; }
    case FX_ECHO: {                                                                  /* a pulse and its echoes, each quieter, each further on */
        for (i = 0; i < 5; i++) {
            int32_t h = 30 - i * 6;
            if (ms + 40u < (uint32_t)i * 140u) continue;
            cv_rect(24 + i * 44, 21 - h / 2, 22, h, mix16(R_BG, c, 255 - (uint32_t)i * 45));
        }
        break; }
    case FX_VERB: {                                                                  /* rings spreading out, fading */
        for (i = 0; i < 4; i++) {
            int32_t rr = 6 + (int32_t)((ms / 14u + (uint32_t)i * 28u) % 112u) / 3;
            int32_t w = rr * 2;
            uint16_t cc = mix16(R_BG, c, 255 - (uint32_t)(rr - 6) * 6);
            cv_rect(120 - w, 21 - rr / 2, 2, rr, cc); cv_rect(120 + w - 2, 21 - rr / 2, 2, rr, cc);
        }
        cv_rect(116, 15, 8, 12, c);
        break; }
    case FX_SOLO: {                                                                  /* one bar left standing, the rest low */
        for (i = 0; i < 9; i++) cv_rect(24 + i * 24, i == 4 ? 4 : 28, 16, i == 4 ? 34 : 8, i == 4 ? c : mix16(R_BG, c, 70));
        break; }
    case FX_LATCH: {                                                                 /* a lock that clicks shut */
        int32_t lift = ms < 300u ? 8 - (int32_t)ms / 40 : 0;
        cv_rect(104, 18, 32, 20, c); cv_rect(110, 8 - lift, 5, 12 + lift, c); cv_rect(125, 8 - lift, 5, 12 + lift, c); cv_rect(110, 6 - lift, 20, 5, c);
        cv_rect(118, 24, 4, 8, R_BG);
        break; }
    case FX_CRUSH: case FX_DECI: {                                                   /* a wave in coarse steps (crush) or in dots (decimate) */
        int32_t sh = (int32_t)(ms / 40u);
        for (i = 0; i < 20; i++) {
            int32_t v = VIB[((i * 2 + sh) & 31)] * 14 / 127;
            if (pad == FX_CRUSH) cv_rect(20 + i * 10, 21 + (v / 5) * 5 - 2, 10, 5, c);
            else cv_rect(20 + i * 10 + 3, 21 + v - 2, 4, 4, c);
        }
        break; }
    case FX_GATE: {                                                                  /* a chopped sound: bars on, bars off, going by */
        for (i = 0; i < 12; i++) {
            int32_t x = 12 + i * 20 - (int32_t)(ms / 8u) % 40;
            if (((i + (int32_t)(ms / 125u)) & 1) == 0) cv_rect(x, 8, 12, 26, c); else cv_rect(x, 19, 12, 4, mix16(R_BG, c, 80));
        }
        break; }
    case FX_DIVE: {                                                                  /* a note falling, leaving a trail */
        int32_t y = (int32_t)ease(ms > 700u ? 700u : ms, 700, 4, 32);
        for (i = 0; i < 6; i++) cv_rect(112, y - i * 5 > 4 ? y - i * 5 : 4, 18, 5, mix16(R_BG, c, 255 - (uint32_t)i * 40));
        for (k = 0; k < 3; k++) cv_rect(60 + k * 60, 6 + (int32_t)(ms / 6u + (uint32_t)k * 20u) % 30, 3, 6, mix16(R_BG, c, 120));
        break; }
    case FX_WOB: {                                                                   /* a wavering line */
        int32_t sh = (int32_t)(ms / 25u);
        for (i = 0; i < 200; i++) cv_rect(20 + i, 21 + VIB[((i / 3 + sh) & 31)] * (6 + VIB[((sh / 2) & 31)] * 4 / 127) / 127 - 1, 1, 3, c);
        break; }
    default: {                                                                       /* kill: the sound is cut, the bar flashes */
        uint16_t f = (ms / 100u) & 1u ? c : mix16(R_BG, c, 140);
        cv_rect(0, 0, 240, 3, f); cv_rect(0, HDR_H - 4, 240, 3, f);
        ico_cross(104, 6, 30, f); ico_cross(105, 6, 30, f);
        break; }
    }
}

static void fx_view_draw(void)
{
    uint32_t b, i;
    for (b = 0; b < 2u; b++) {
        cv_begin(240, 82, R_BG);
        for (i = b * 8u; i < b * 8u + 8u; i++) {
            uint32_t col = i % 4u, row = (i / 4u) % 2u, on = fx_mask >> i & 1u;
            int32_t x = 4 + (int32_t)col * 59, y = (int32_t)row * 41;
            uint16_t c = sg_color(FX_GRP[i]);
            cv_rect(x, y, 57, 38, on ? c : mix16(R_BG, c, 46));
            cv_rect(x, y, 57, 3, c);
            fx_icon(i, x + 5, y + 14, on ? R_BG : mix16(R_BG, c, 230));
            cv_text(x + 25, y + 16, &FONT_S, FX_NAME[i], on ? R_BG : mix16(R_BG, c, 200));
        }
        cv_blit(0, 48u + b * 82u);
    }
}
static void fx_view_update(void)
{
    static uint32_t mask_d = ~0u;
    if (fx_view_force || fx_mask != mask_d) {
        if (fx_view_force) lcd_fill(0, 44, 240, 172, R_BG);
        fx_view_force = 0; mask_d = fx_mask;
        fx_view_draw();
    }
}
