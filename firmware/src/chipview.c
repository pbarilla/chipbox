/* SPDX-License-Identifier: GPL-3.0-only */
/* The chip screen: the selected channel's sound. Tabs for its channels, its envelope drawn, and a card for each
 * knob showing what it is set to. Two pages of cards (the ALGORITHM knob flips them): the tone (duty or wave, decay,
 * vibrato, sweep or pan) and the envelope (attack, decay, sustain, release). Included by chipbox.c. */
#define CV_TABS_Y 44
#define CV_ENV_Y 76
#define CV_ENV_H 90
#define CV_CARDS_Y 170
#define CV_CARD_W 57                            /* four cards, 2 apart */
#define CV_CARD_X(k) (3 + (int32_t)(k) * (CV_CARD_W + 2))


/* ---- a look per chip: the windows, bars and pointer each console's menus had */
static void ring(int32_t x, int32_t y, int32_t w, int32_t h, int32_t n, uint16_t c)       /* an n px outline */
{
    cv_rect(x, y, w, n, c); cv_rect(x, y + h - n, w, n, c);
    cv_rect(x, y + n, n, h - 2 * n, c); cv_rect(x + w - n, y + n, n, h - 2 * n, c);
}
static void skin_box(uint32_t chip, const theme_t *t, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t fill)
{
    cv_rect(x, y, w, h, fill);
    if (chip == CHIP_NES) {                                     /* a white box, corners notched: the Final Fantasy / Zelda menus */
        ring(x, y, w, h, 2, t->fg);
        cv_rect(x, y, 2, 2, t->bg); cv_rect(x + w - 2, y, 2, 2, t->bg); cv_rect(x, y + h - 2, 2, 2, t->bg); cv_rect(x + w - 2, y + h - 2, 2, 2, t->bg);
        ring(x + 2, y + 2, w - 4, h - 4, 1, t->bg);
    } else if (chip == CHIP_GB) {                               /* a thick rounded border with a drop shadow, like the DMG's dialogs */
        cv_rect(x + 2, y + h, w - 1, 2, t->dim);
        cv_rect(x + w, y + 2, 2, h - 1, t->dim);
        ring(x, y, w, h, 3, t->fg);
        cv_rect(x, y, 2, 2, t->bg); cv_rect(x + w - 2, y, 2, 2, t->bg); cv_rect(x, y + h - 2, 2, 2, t->bg); cv_rect(x + w - 2, y + h - 2, 2, 2, t->bg);
        ring(x + 3, y + 3, w - 6, h - 6, 1, t->bg);
    } else {                                                    /* a double line, corners cut: Phantasy Star's windows */
        ring(x, y, w, h, 1, t->fg); ring(x + 2, y + 2, w - 4, h - 4, 1, t->fg);
        cv_rect(x, y, 3, 1, t->bg); cv_rect(x, y + 1, 1, 2, t->bg); cv_rect(x + w - 3, y, 3, 1, t->bg); cv_rect(x + w - 1, y + 1, 1, 2, t->bg);
        cv_rect(x, y + h - 1, 3, 1, t->bg); cv_rect(x, y + h - 3, 1, 2, t->bg); cv_rect(x + w - 3, y + h - 1, 3, 1, t->bg); cv_rect(x + w - 1, y + h - 3, 1, 2, t->bg);
    }
}
static void skin_pointer(const theme_t *t, int32_t x, int32_t y, uint16_t col)         /* a small right-pointing triangle */
{
    int32_t i;
    (void)t;
    for (i = 0; i < 5; i++) cv_rect(x + i, y + i, 1, 9 - 2 * i, col);
}

static void chip_tabs(const theme_t *t, uint32_t chip, uint32_t sel)
{
    uint32_t c;
    cv_begin(240, 26, t->bg);
    for (c = 0; c < CHIP_NCH[chip]; c++) {
        int32_t x = 3 + (int32_t)c * 79;
        uint16_t ink = c == sel ? t->bg : t->fg;
        skin_box(chip, t, x, 0, 75, 26, c == sel ? t->acc : t->bar);
        channel_glyph(chip, c, x + 8, 6, 22, 14, ink);
        cv_text(x + 34, 5, &FONT_S, t->ch[c], ink);
    }
    cv_blit(0, CV_TABS_Y - 1);
}

/* the envelope as it plays: rise, fall to the held level, hold, fade; the time axis is squeezed to fit */
static void chip_env(const theme_t *t, uint32_t chip, const inst_t *in, uint32_t page)
{
    int32_t base = CV_ENV_H - 10, top = 12, w[4], total, x, ys, i;
    int32_t px[5], py[5];
    cv_begin(240, CV_ENV_H, t->bg);
    skin_box(chip, t, 3, 0, 234, CV_ENV_H, t->bar);
    cv_rect(214, 6, 6, 6, page ? t->dim : t->acc); cv_rect(224, 6, 6, 6, page ? t->acc : t->dim);       /* which page of knobs */
    cv_rect(12, base + 1, 216, 1, t->dim);
    w[0] = 4 + (int32_t)in->atk * 4;
    w[1] = 4 + (15 - (int32_t)in->sus) * (2 * (int32_t)in->dec + 1) * 2 / 15;
    w[2] = 36;
    w[3] = 4 + (2 * (int32_t)in->rel + 1) * (int32_t)in->sus * 2 / 15;
    total = w[0] + w[1] + w[2] + w[3];
    if (total > 216) for (i = 0; i < 4; i++) w[i] = w[i] * 216 / total;
    ys = base - (base - top) * (int32_t)in->sus / 15;
    x = 12;
    px[0] = x; py[0] = base;
    x += w[0]; px[1] = x; py[1] = top;
    x += w[1]; px[2] = x; py[2] = ys;
    x += w[2]; px[3] = x; py[3] = ys;
    x += w[3]; px[4] = x; py[4] = base;
    for (i = 0; i < 4; i++) { cv_line(px[i], py[i], px[i + 1], py[i + 1], t->acc); cv_line(px[i], py[i] + 1, px[i + 1], py[i + 1] + 1, t->acc); }
    cv_blit(0, CV_ENV_Y);
}

/* a card per knob: its job in a word, its value as a shape */
static void card_bar(const theme_t *t, uint32_t chip, int32_t x, int32_t v, int32_t max)
{
    int32_t i;
    if (chip == CHIP_NES) {                                     /* eight blocks, like a health bar */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 36, 4, 10, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_GB) {                               /* five chunky cells */
        int32_t lit = (v * 5 + max - 1) / max;
        for (i = 0; i < 5; i++) cv_rect(x + 8 + i * 8, 34, 7, 14, i < lit ? t->acc : t->dim);
    } else {                                                    /* a plain bar in a white outline */
        ring(x + 7, 35, 42, 12, 1, t->fg);
        cv_rect(x + 8, 36, 40, 10, t->bg);
        if (v > 0) cv_rect(x + 8, 36, 40 * v / max, 10, t->acc);
    }
}
static void chip_cards(const theme_t *t, uint32_t chip, uint32_t c, uint32_t page)
{
    const inst_t *in = &inst[chip][c];
    uint32_t k, k1 = has_wave(chip, c) || has_duty(chip, c);
    int32_t sw = in->sweep;
    cv_begin(240, 64, t->bg);
    for (k = 0; k < 4u; k++) {
        int32_t x = CV_CARD_X(k);
        static const char *const ENVL[4] = {"ATK", "DEC", "SUS", "REL"};
        const char *lab = page ? ENVL[k] : k == 0u ? (has_wave(chip, c) ? "WAVE" : "DUTY") : k == 1u ? "VOL" : k == 2u ? "VIB" : chip == CHIP_GB ? "PAN" : "SWEEP";
        uint16_t lc = !page && ((k == 0u && !k1) || (k == 1u && !has_vol(chip, c))) ? t->dim : t->fg;
        skin_box(chip, t, x, 0, CV_CARD_W, 64, t->bar);
        cv_text(x + (CV_CARD_W - text_w(&FONT_S, lab)) / 2, 4, &FONT_S, lab, lc);
        if (page) card_bar(t, chip, x, k == 0u ? in->atk : k == 1u ? in->dec : k == 2u ? in->sus : in->rel, 15);
        else if (k == 0u) { if (k1) channel_glyph(chip, c, x + 8, 28, 40, 30, t->acc); }
        else if (k == 1u) { if (has_vol(chip, c)) card_bar(t, chip, x, in->vol, 15); }
        else if (k == 2u) card_bar(t, chip, x, in->vib_dep, 15);
        else if (chip == CHIP_GB) {                                 /* left, centre, right */
            int32_t p;
            for (p = 0; p < 3; p++) cv_rect(x + 7 + p * 15, 36, 12, 10, p == (int32_t)in->pan ? t->acc : t->dim);
        } else {                                                    /* centred: down to the left, up to the right */
            cv_rect(x + 8, 36, 40, 10, t->dim);
            cv_rect(x + 27, 32, 2, 18, t->fg);
            if (sw > 0) cv_rect(x + 28, 36, 20 * sw / 16, 10, t->acc);
            else if (sw < 0) cv_rect(x + 28 + 20 * sw / 16, 36, -20 * sw / 16, 10, t->acc);
        }
    }
    cv_blit(0, CV_CARDS_Y);
}
static void chip_panels(const theme_t *t, uint32_t chip, uint32_t c, uint32_t page)
{
    chip_tabs(t, chip, c);
    chip_env(t, chip, &inst[chip][c], page);
    chip_cards(t, chip, c, page);
}
