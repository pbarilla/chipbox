/* SPDX-License-Identifier: GPL-3.0-only */
/* The chip screen: the selected channel's sound. A tab with its name, its envelope drawn, and a card for each knob
 * showing what it is set to. Three pages of cards (the ALGORITHM knob flips them): the tone (duty or wave, volume,
 * vibrato, sweep or pan), the envelope (attack, decay, sustain, release) and the arp (chord, speed, direction).
 * Included by chipbox.c. */
#define CV_TABS_Y 44
#define CV_ENV_Y 74
#define CV_ENV_H 76
#define CV_CARDS_Y 154
#define CV_CARDS_H 58
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
    } else if (chip == CHIP_PCE) {                              /* a light card, corners cut, an orange stripe along the top */
        ring(x, y, w, h, 2, t->fg);
        cv_rect(x, y, 3, 3, t->bg); cv_rect(x + w - 3, y, 3, 3, t->bg); cv_rect(x, y + h - 3, 3, 3, t->bg); cv_rect(x + w - 3, y + h - 3, 3, 3, t->bg);
        cv_rect(x + 3, y + 2, w - 6, 3, t->acc);
    } else if (chip == CHIP_GB) {                               /* a thick rounded border with a drop shadow, like the DMG's dialogs */
        cv_rect(x + 2, y + h, w - 1, 2, t->dim);
        cv_rect(x + w, y + 2, 2, h - 1, t->dim);
        ring(x, y, w, h, 3, t->fg);
        cv_rect(x, y, 2, 2, t->bg); cv_rect(x + w - 2, y, 2, 2, t->bg); cv_rect(x, y + h - 2, 2, 2, t->bg); cv_rect(x + w - 2, y + h - 2, 2, 2, t->bg);
        ring(x + 3, y + 3, w - 6, h - 6, 1, t->bg);
    } else if (chip == CHIP_VRC6) {                             /* a gold line round a plum one, corners cut deep */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x, y, 4, 4, t->bg); cv_rect(x + w - 4, y, 4, 4, t->bg); cv_rect(x, y + h - 4, 4, 4, t->bg); cv_rect(x + w - 4, y + h - 4, 4, 4, t->bg);
        cv_rect(x + 4, y, 1, 1, t->acc); cv_rect(x + 3, y + 1, 1, 1, t->acc); cv_rect(x + 2, y + 2, 1, 1, t->acc); cv_rect(x + 1, y + 3, 1, 1, t->acc); cv_rect(x, y + 4, 1, 1, t->acc);
    } else if (chip == CHIP_VB) {                               /* a thin red frame, notched at the corners, a dim line inside */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x, y, 2, 2, t->bg); cv_rect(x + w - 2, y, 2, 2, t->bg); cv_rect(x, y + h - 2, 2, 2, t->bg); cv_rect(x + w - 2, y + h - 2, 2, 2, t->bg);
    } else if (chip == CHIP_TIA) {                              /* a tan frame with the three Atari stripes along the top */
        ring(x, y, w, h, 2, t->fg);
        cv_rect(x + 3, y + 3, w - 6, 1, RGB(236, 130, 40)); cv_rect(x + 3, y + 4, w - 6, 1, RGB(214, 64, 44)); cv_rect(x + 3, y + 5, w - 6, 1, RGB(150, 70, 170));
    } else if (chip == CHIP_AY) {                               /* a thin white frame, the rainbow under the top edge */
        ring(x, y, w, h, 1, t->fg);
        cv_rect(x + 1, y + 1, w - 2, 1, RGB(224, 40, 40)); cv_rect(x + 1, y + 2, w - 2, 1, RGB(236, 210, 40)); cv_rect(x + 1, y + 3, w - 2, 1, RGB(40, 200, 60)); cv_rect(x + 1, y + 4, w - 2, 1, RGB(40, 200, 220));
    } else if (chip == CHIP_MD) {                               /* a white frame with a thick blue left edge, top-right and bottom-left corners cut */
        ring(x, y, w, h, 2, t->fg);
        cv_rect(x + 2, y + 2, 3, h - 4, RGB(0, 98, 210));
        cv_rect(x + w - 4, y, 4, 4, t->bg); cv_rect(x, y + h - 4, 4, 4, t->bg);
    } else if (chip == CHIP_POKEY) {                            /* a pink frame, the three Atari stripes along the bottom */
        ring(x, y, w, h, 2, t->acc);
        cv_rect(x + 3, y + h - 6, w - 6, 1, RGB(236, 130, 40)); cv_rect(x + 3, y + h - 5, w - 6, 1, RGB(236, 88, 140)); cv_rect(x + 3, y + h - 4, w - 6, 1, RGB(150, 70, 170));
    } else if (chip == CHIP_SPK) {                              /* a DOS window: a double line, a dark shadow to the right and below */
        cv_rect(x + 3, y + h, w, 3, RGB(0, 0, 24)); cv_rect(x + w, y + 3, 3, h, RGB(0, 0, 24));
        ring(x, y, w, h, 1, t->fg); ring(x + 2, y + 2, w - 4, h - 4, 1, t->fg);
    } else if (chip == CHIP_OPL) {                              /* a gold edge with the contacts along the bottom */
        ring(x, y, w, h, 2, t->acc);
        { int32_t k; for (k = 6; k < w - 6; k += 4) cv_rect(x + k, y + h - 6, 2, 4, t->acc); }
    } else if (chip == CHIP_WSG) {                              /* a maze wall in two blue lines, the corners rounded */
        ring(x, y, w, h, 2, RGB(40, 40, 222)); ring(x + 4, y + 4, w - 8, h - 8, 1, RGB(40, 40, 222));
        cv_rect(x, y, 3, 3, t->bg); cv_rect(x + w - 3, y, 3, 3, t->bg); cv_rect(x, y + h - 3, 3, 3, t->bg); cv_rect(x + w - 3, y + h - 3, 3, 3, t->bg);
    } else if (chip == CHIP_N163) {                             /* a coral line, corners cut on the slant */
        ring(x, y, w, h, 1, t->acc);
        cv_rect(x, y, 3, 1, t->bg); cv_rect(x, y, 1, 3, t->bg); cv_rect(x + w - 3, y + h - 1, 3, 1, t->bg); cv_rect(x + w - 1, y + h - 3, 1, 3, t->bg);
        cv_rect(x + 3, y + 1, 1, 1, t->acc); cv_rect(x + 1, y + 3, 1, 1, t->acc); cv_rect(x + w - 4, y + h - 2, 1, 1, t->acc); cv_rect(x + w - 2, y + h - 4, 1, 1, t->acc);
    } else if (chip == CHIP_S5B) {                              /* an amber line, a little sun in each top corner */
        ring(x, y, w, h, 1, t->acc);
        cv_rect(x + 3, y + 3, 3, 3, t->acc); cv_rect(x + w - 6, y + 3, 3, 3, t->acc);
    } else if (chip == CHIP_MMC5) {                             /* a mint line, corners cut like the NES's boxes, a dim line inside */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x, y, 2, 2, t->bg); cv_rect(x + w - 2, y, 2, 2, t->bg); cv_rect(x, y + h - 2, 2, 2, t->bg); cv_rect(x + w - 2, y + h - 2, 2, 2, t->bg);
    } else if (chip == CHIP_FDS) {                              /* an orange frame, the top right corner cut like a disk's */
        ring(x, y, w, h, 2, t->acc);
        cv_rect(x + w - 7, y, 7, 7, t->bg); cv_rect(x + w - 7, y + 5, 7, 2, t->acc); cv_rect(x + w - 7, y + 5, 2, 2, t->bg);
    } else if (chip == CHIP_SCC) {                              /* a lime line, three little steps in the bottom right corner */
        ring(x, y, w, h, 1, t->acc);
        cv_rect(x + w - 10, y + h - 4, 7, 1, t->acc); cv_rect(x + w - 7, y + h - 6, 4, 1, t->acc); cv_rect(x + w - 4, y + h - 8, 1, 1, t->acc);
    } else if (chip == CHIP_OPLL) {                             /* a cyan line, rounded, a dim line inside */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x, y, 3, 3, t->bg); cv_rect(x + w - 3, y, 3, 3, t->bg); cv_rect(x, y + h - 3, 3, 3, t->bg); cv_rect(x + w - 3, y + h - 3, 3, 3, t->bg);
        cv_rect(x + 2, y + 1, 1, 1, t->acc); cv_rect(x + 1, y + 2, 1, 1, t->acc);
    } else if (chip == CHIP_VRC7) {                             /* a vermilion line, the top right corner cut deep, a dim line inside */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x + w - 6, y, 6, 6, t->bg); cv_rect(x + w - 6, y + 5, 6, 1, t->acc); cv_rect(x + w - 6, y + 5, 1, 1, t->bg);
    } else if (chip == CHIP_OPM) {                              /* a magenta double line, a notch in the middle of the top and bottom */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x + w / 2 - 4, y, 8, 3, t->bg); cv_rect(x + w / 2 - 4, y + h - 3, 8, 3, t->bg);
    } else if (chip == CHIP_OPL3) {                             /* a silver line, little pins along the bottom edge */
        ring(x, y, w, h, 1, t->acc);
        { int32_t k; for (k = 4; k < w - 4; k += 4) cv_rect(x + k, y + h - 3, 2, 3, t->acc); }
    } else if (chip == CHIP_WS) {                               /* a lavender line, rounded corners like a handheld's screen */
        ring(x, y, w, h, 2, t->acc);
        cv_rect(x, y, 3, 3, t->bg); cv_rect(x + w - 3, y, 3, 3, t->bg); cv_rect(x, y + h - 3, 3, 3, t->bg); cv_rect(x + w - 3, y + h - 3, 3, 3, t->bg);
    } else if (chip == CHIP_SAA) {                              /* a yellow line over a four colour stripe along the bottom */
        ring(x, y, w, h, 1, t->acc);
        cv_rect(x + 2, y + h - 3, w / 4, 2, RGB(230, 50, 50)); cv_rect(x + 2 + w / 4, y + h - 3, w / 4, 2, RGB(255, 190, 40)); cv_rect(x + 2 + w / 2, y + h - 3, w / 4, 2, RGB(60, 200, 80)); cv_rect(x + 2 + w * 3 / 4, y + h - 3, w / 4 - 4, 2, RGB(60, 140, 240));
    } else if (chip == CHIP_VIC) {                              /* a pale cyan line inside a blue border, like the screen */
        ring(x, y, w, h, 3, t->dim); ring(x + 3, y + 3, w - 6, h - 6, 1, t->acc);
    } else if (chip == CHIP_TED) {                              /* a green line, a grey line inside it, square corners */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
    } else if (chip == CHIP_LYNX) {                             /* an orange line, corners cut like a handheld's bezel */
        ring(x, y, w, h, 2, t->acc);
        cv_rect(x, y, 4, 2, t->bg); cv_rect(x, y, 2, 4, t->bg); cv_rect(x + w - 4, y, 4, 2, t->bg); cv_rect(x + w - 2, y, 2, 4, t->bg);
        cv_rect(x, y + h - 2, 4, 2, t->bg); cv_rect(x, y + h - 4, 2, 4, t->bg); cv_rect(x + w - 4, y + h - 2, 4, 2, t->bg); cv_rect(x + w - 2, y + h - 4, 2, 4, t->bg);
    } else if (chip == CHIP_PC98) {                             /* a peach line, a dim line inside, a little tab at the top left like a floppy label */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x + 4, y, 10, 3, t->acc);
    } else if (chip == CHIP_NEO) {                              /* a crimson double line, a gold line at the bottom */
        ring(x, y, w, h, 2, t->acc);
        cv_rect(x + 2, y + h - 4, w - 4, 1, RGB(255, 200, 40));
    } else if (chip == CHIP_NGP) {                              /* a pink line, the corners rounded, a dim line inside */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        cv_rect(x, y, 4, 4, t->bg); cv_rect(x + w - 4, y, 4, 4, t->bg); cv_rect(x, y + h - 4, 4, 4, t->bg); cv_rect(x + w - 4, y + h - 4, 4, 4, t->bg);
        cv_rect(x + 3, y + 1, 1, 2, t->acc); cv_rect(x + 1, y + 3, 2, 1, t->acc);
    } else if (chip == CHIP_OPL1) {                             /* a green line, contacts along the bottom edge, like the AdLib's */
        ring(x, y, w, h, 1, t->acc);
        { int32_t k; for (k = 3; k < w - 3; k += 3) cv_rect(x + k, y + h - 3, 2, 3, t->acc); }
    } else if (chip == CHIP_OPZ) {                              /* a cream line like an LCD's bezel, a darker line inside */
        ring(x, y, w, h, 2, t->dim); ring(x + 2, y + 2, w - 4, h - 4, 1, t->acc);
    } else if (chip == CHIP_SN77) {                             /* a cyan line, a short bright stroke in each corner like a vector screen */
        ring(x, y, w, h, 1, t->dim);
        cv_rect(x, y, 8, 1, t->acc); cv_rect(x, y, 1, 8, t->acc); cv_rect(x + w - 8, y, 8, 1, t->acc); cv_rect(x + w - 1, y, 1, 8, t->acc);
        cv_rect(x, y + h - 1, 8, 1, t->acc); cv_rect(x, y + h - 8, 1, 8, t->acc); cv_rect(x + w - 8, y + h - 1, 8, 1, t->acc); cv_rect(x + w - 1, y + h - 8, 1, 8, t->acc);
    } else if (chip == CHIP_C30) {                              /* a gold frame, two little lights in the top corners like a marquee's bulbs */
        ring(x, y, w, h, 2, t->acc);
        cv_rect(x + 4, y + 4, 3, 3, t->acc); cv_rect(x + w - 7, y + 4, 3, 3, t->acc);
    } else if (chip == CHIP_MSM) {                              /* a raspberry double line, six drawbar notches along the bottom */
        ring(x, y, w, h, 1, t->acc); ring(x + 2, y + 2, w - 4, h - 4, 1, t->dim);
        { int32_t k; for (k = 0; k < 6; k++) cv_rect(x + w / 2 - 18 + k * 7, y + h - 3, 3, 3, t->acc); }
    } else if (chip == CHIP_SID) {                              /* a C64 screen: a light blue border round the dark blue */
        ring(x, y, w, h, 2, t->dim); ring(x + 2, y + 2, w - 4, h - 4, 1, t->acc);
    } else if (chip == CHIP_OPN) {                              /* a steel line, a short bright bar at the top left like a drive's light */
        ring(x, y, w, h, 1, t->acc); cv_rect(x + 4, y, 12, 2, t->acc); cv_rect(x + 2, y + 2, w - 4, 1, t->dim);
    } else if (chip == CHIP_ZX) {                               /* a white line, four rainbow stripes in the top right corner */
        ring(x, y, w, h, 1, t->fg);
        cv_rect(x + w - 10, y, 2, 4, RGB(236, 60, 56)); cv_rect(x + w - 8, y, 2, 4, RGB(250, 204, 40)); cv_rect(x + w - 6, y, 2, 4, RGB(60, 200, 80)); cv_rect(x + w - 4, y, 2, 4, RGB(60, 190, 230));
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

static void chip_tabs(const theme_t *t, uint32_t c)
{
    uint32_t chip = KIND_CHIP[chan[c].kind];
    char nm[16];
    cv_begin(240, 26, t->bg);
    skin_box(chip, t, 3, 0, 234, 26, t->acc);                   /* one wide tab: the channel's picture, its name, its place in the list */
    channel_glyph(c, 12, 6, 22, 14, t->bg);
    cv_text(42, 5, &FONT_S, KIND_NAME[chan[c].kind], t->bg);
    fmt_u(nm, c + 1u);
    cv_text(228 - text_w(&FONT_S, nm), 5, &FONT_S, nm, t->bg);
    cv_blit(0, CV_TABS_Y - 1);
}

/* the YM2151's eight algorithms (0..7) and the OPL3's four connections (8..11): four operators, who modulates whom, who reaches the output */
static void fm4_diagram(const theme_t *t, uint32_t alg)
{
    static const int8_t POS[12][4][2] = {
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}}, {{0, 0}, {0, 2}, {1, 1}, {2, 1}}, {{0, 0}, {0, 2}, {1, 2}, {2, 1}}, {{0, 0}, {1, 0}, {1, 2}, {2, 1}},
        {{0, 0}, {1, 0}, {0, 2}, {1, 2}}, {{0, 1}, {1, 0}, {1, 1}, {1, 2}}, {{0, 0}, {1, 0}, {1, 1}, {1, 2}}, {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}}, {{0, 0}, {0, 2}, {1, 2}, {2, 2}}, {{0, 0}, {1, 0}, {2, 0}, {2, 2}}, {{1, 0}, {0, 2}, {1, 2}, {1, 3}}};
    static const int8_t WIRE[12][3][2] = {                       /* from, to; the operator numbers count from 0; -1 none */
        {{0, 1}, {1, 2}, {2, 3}}, {{0, 2}, {1, 2}, {2, 3}}, {{1, 2}, {0, 3}, {2, 3}}, {{0, 1}, {1, 3}, {2, 3}},
        {{0, 1}, {2, 3}, {-1, 0}}, {{0, 1}, {0, 2}, {0, 3}}, {{0, 1}, {-1, 0}, {-1, 0}}, {{-1, 0}, {-1, 0}, {-1, 0}},
        {{0, 1}, {1, 2}, {2, 3}}, {{1, 2}, {2, 3}, {-1, 0}}, {{0, 1}, {1, 2}, {-1, 0}}, {{1, 2}, {-1, 0}, {-1, 0}}};
    static const uint8_t CARRY[12] = {8, 8, 8, 8, 10, 14, 14, 15, 8, 9, 12, 13};     /* a bit an operator */
    int32_t bx[4], by[4], i, lo = 99, hi = 0, mid;
    if (alg > 11u) alg = 0;
    for (i = 0; i < 4; i++) { bx[i] = 10 + POS[alg][i][0] * 48; by[i] = 4 + POS[alg][i][1] * 15; }
    for (i = 0; i < 3; i++) {
        int32_t f = WIRE[alg][i][0], g = WIRE[alg][i][1];
        if (f < 0) continue;
        {   int32_t xm = bx[g] - 8, y0 = by[f] + 5, y1 = by[g] + 5;                 /* out of f, across, up or down, into g */
            cv_rect(bx[f] + 26, y0, xm - bx[f] - 26 + 2, 2, t->fg);
            cv_rect(xm, y0 < y1 ? y0 : y1, 2, (y0 < y1 ? y1 - y0 : y0 - y1) + 2, t->fg);
            cv_rect(xm, y1, bx[g] - xm - 3, 2, t->fg);
            cv_rect(bx[g] - 4, y1 - 2, 1, 6, t->fg); cv_rect(bx[g] - 3, y1 - 1, 1, 4, t->fg); cv_rect(bx[g] - 2, y1, 1, 2, t->fg);
        }
    }
    for (i = 0; i < 4; i++) {
        char l[2];
        l[0] = (char)('1' + i); l[1] = 0;
        cv_rect(bx[i], by[i], 26, 12, t->acc);
        cv_text(bx[i] + 9, by[i] - 1, &FONT_S, l, t->bg);
        if (CARRY[alg] >> i & 1u) {
            cv_rect(bx[i] + 26, by[i] + 5, 202 - bx[i] - 26, 2, t->fg);
            if (by[i] < lo) lo = by[i];
            if (by[i] > hi) hi = by[i];
        }
    }
    mid = (lo + hi) / 2 + 6;
    cv_rect(202, lo + 5, 2, hi - lo + 2, t->fg);
    cv_rect(204, mid - 1, 4, 2, t->fg);
    cv_text(210, mid - 6, &FONT_S, "OUT", t->fg);
}

/* the OPL2's two operators: A modulates C (or both play, in the last pair); C's wave is drawn above it: sine, half sine, absolute sine */
static void opl_diagram(const theme_t *t, uint32_t alg)
{
    int32_t ax = alg == 3u ? 158 : 92, ay = alg == 3u ? 10 : 30, cx = 158, cy = alg == 3u ? 46 : 30, i, mid = alg == 3u ? 38 : 39;
    cv_rect(ax, ay, 30, 16, t->acc); cv_text(ax + 12, ay + 2, &FONT_S, "A", t->bg);
    cv_rect(cx, cy, 30, 16, t->acc); cv_text(cx + 12, cy + 2, &FONT_S, "C", t->bg);
    if (alg != 3u) {
        cv_rect(ax + 30, ay + 7, cx - ax - 34, 2, t->fg);
        cv_rect(cx - 4, cy + 5, 1, 6, t->fg); cv_rect(cx - 3, cy + 6, 1, 4, t->fg); cv_rect(cx - 2, cy + 7, 1, 2, t->fg);
        cv_rect(cx + 30, cy + 7, 14, 2, t->fg); cv_rect(cx + 44, cy + 7, 4, 2, t->fg); cv_text(210, cy + 1, &FONT_S, "OUT", t->fg);
        for (i = 0; i < 30; i++) {                               /* C's wave */
            static const uint8_t H[15] = {0, 1, 2, 3, 4, 5, 5, 5, 5, 4, 3, 2, 1, 0, 0};
            int32_t hump = H[i % 15], v = alg == 0u && i >= 15 ? -hump : alg == 1u && i >= 15 ? 0 : hump;
            cv_rect(cx + i, cy - 9 - v, 1, 2, t->dim);
        }
    } else {
        cv_rect(ax + 30, ay + 7, 14, 2, t->fg); cv_rect(cx + 30, cy + 7, 14, 2, t->fg);
        cv_rect(202, ay + 7, 2, cy - ay + 2, t->fg); cv_rect(204, mid, 4, 2, t->fg); cv_text(210, mid - 6, &FONT_S, "OUT", t->fg);
    }
    cv_rect(ax + 5, ay - 5, 2, 5, t->fg); cv_rect(ax + 5, ay - 5, 20, 2, t->fg); cv_rect(ax + 23, ay - 5, 2, 5, t->fg);        /* feedback on A */
}

/* the SID's waveform at a place in its cycle (12 bits), as the chip makes it */
static int32_t sid_shape(const inst_t *in, uint32_t ph, uint32_t noise)
{
    const uint32_t wf = SID_WF[sid_get(in, SP_WAVE) & 7u], mdl = sid_get(in, SP_MODEL) & 1u, pw = SID_PWK[sid_get(in, SP_PW)];
    uint32_t w;
    if (wf == 1u) w = ((ph & 0x800u ? ~ph : ph) << 1) & 0xffeu;
    else if (wf == 2u) w = ph;
    else if (wf == 4u) w = ph >= pw ? 0xfffu : 0u;
    else if (wf == 8u) w = (noise * 2654435761u) >> 20;
    else {
        w = (uint32_t)SID_WAVE[(wf == 3u ? 0u : wf == 5u ? 1u : wf == 6u ? 2u : 3u) * 4096u + mdl * 16384u + ph] << 4;
        if (wf & 4u) w &= ph >= pw ? 0xfffu : 0u;
    }
    return (int32_t)w;
}
static uint32_t isqrt64(uint64_t v)
{
    uint64_t r = 0, b = (uint64_t)1 << 62;
    while (b > v) b >>= 2;
    while (b) { if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; }
    return (uint32_t)r;
}
static void sid_scope(const theme_t *t, const inst_t *in, int32_t top, int32_t base)       /* two cycles of the wave, as it will sound */
{
    int32_t i, px = 12, py = base;
    for (i = 0; i < 108; i++) {
        const int32_t x = 12 + i * 2, y = base - sid_shape(in, (uint32_t)(i * 2 * 4096 / 108) & 4095u, (uint32_t)i) * (base - top) / 4095;
        if (i) { cv_line(px, py, x, y, t->acc); cv_line(px, py + 1, x, y + 1, t->acc); }
        px = x; py = y;
    }
}
static void sid_curve(const theme_t *t, const inst_t *in, int32_t top, int32_t base)       /* the filter's response over the cutoff table's range, 250 Hz to 12 kHz */
{
    const uint32_t mode = sid_get(in, SP_MODE) & 7u, res = sid_get(in, SP_RES), mdl = sid_get(in, SP_MODEL) & 1u;
    const uint32_t p = sid_get(in, SP_CUT) * 8u + 3u, g = SID_G[p];
    const uint32_t q = SID_Q[mdl * 256u + res * 16u + (p >> 3)], k = 1048576u / q;       /* k = 1 / Q, Q14 */
    int32_t i, px = 12, py = base;
    if (!mode) { cv_rect(12, base - (base - top) * 3 / 5, 216, 2, t->dim); return; }
    for (i = 0; i < 108; i++) {
        const int64_t r = (int64_t)(((uint32_t)SID_G[i * 127 / 107] << 12) / (g ? g : 1u));        /* the frequency over the cutoff, Q12 (tan from the same table) */
        const int64_t r2 = (r * r) >> 12;
        const int64_t nr = (mode & 1u ? 4096 : 0) - (mode & 4u ? r2 : 0), ni = mode & 2u ? r : 0;
        const int64_t dr = 4096 - r2, di = (r * (int64_t)k) >> 14;
        uint32_t nm = isqrt64((uint64_t)(nr * nr + ni * ni)), dm = isqrt64((uint64_t)(dr * dr + di * di)) | 1u;
        int32_t h, x = 12 + i * 2, y;                                                           /* h: the Q12 magnitude */
        if (nm >= 3u * dm) h = 12288;
        else { while (dm >= (1u << 18)) { nm >>= 1; dm >>= 1; } h = (int32_t)((nm << 12) / (dm | 1u)); }
        y = base - h * (base - top) * 3 / 5 / 4096;                                           /* a gain of 1 sits at three fifths */
        if (y < top) y = top;
        if (i) { cv_line(px, py, x, y, t->acc); cv_line(px, py + 1, x, y + 1, t->acc); }
        px = x; py = y;
    }
}
/* the envelope as it plays: rise, fall to the held level, hold, fade; the time axis is squeezed to fit */
static void chip_env(const theme_t *t, uint32_t chip, const inst_t *in, uint32_t page, uint32_t np, uint32_t fm, uint32_t drum)
{
    int32_t base = CV_ENV_H - 10, top = 12, w[4], total, x, ys, i;
    int32_t px[5], py[5];
    cv_begin(240, CV_ENV_H, t->bg);
    skin_box(chip, t, 3, 0, 234, CV_ENV_H, t->bar);
    if (drum) {                                                 /* the kit: its eight lanes, each in its colour */
        static const char *const LN[11] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD"};
        int32_t k;
        for (k = 0; k < 11; k++) {
            cv_rect(12 + k * 20, 14, 19, 48, DRUM_LC[k]);
            rk_ox = 0; rack_label(13 + k * 20, 32, LN[k], RGB(28, 26, 30));
        }
        cv_blit(0, CV_ENV_Y);
        return;
    }
    { uint32_t d; for (d = 0; d < np; d++) cv_rect(204 - (int32_t)(np - 3u) * 10 + (int32_t)d * 10, 6, 6, 6, d == page ? t->acc : t->dim); }       /* which page of knobs */
    cv_rect(12, base + 1, 216, 1, t->dim);
    if (chip == CHIP_SID && (page == 0u || page == 3u)) { sid_scope(t, in, top, base); cv_blit(0, CV_ENV_Y); return; }
    if (chip == CHIP_SID && page == 4u) { sid_curve(t, in, top, base); cv_blit(0, CV_ENV_Y); return; }
    if (page == 0u && fm) {                                     /* the FM: the operators and how they connect */
        if (chip == CHIP_OPL) opl_diagram(t, in->duty); else if (chip == CHIP_OPL1) opl_diagram(t, in->duty ? 3u : 0u); else if (chip == CHIP_OPM || chip == CHIP_OPL3 || chip == CHIP_PC98 || chip == CHIP_NEO || chip == CHIP_OPZ || chip == CHIP_OPN || chip == CHIP_MD) fm4_diagram(t, in->duty + (chip == CHIP_OPL3 ? 8u : 0u));
        cv_blit(0, CV_ENV_Y);
        return;
    }
    if (page == 3u && fm) {                                     /* the FM: how hard the modulator hits, and how fast it dies away over two seconds */
        for (i = 0; i < 72; i++) {
            int32_t h = 2 + (int32_t)in->wf * 52 / 15 * 16 / (16 + i * 40 / 72 * (int32_t)in->wt);
            cv_rect(12 + i * 3, base - h, 2, h, mix16(t->bar, t->acc, 90));
            cv_rect(12 + i * 3, base - h, 2, 2, t->acc);
        }
        cv_blit(0, CV_ENV_Y);
        return;
    }
    if (page == 3u) {                                           /* the wave: 32 steps of 4 bits, as the chip plays them */
        uint8_t wv[32];
        wave_make(in, wv);
        for (i = 0; i < 32; i++) {
            int32_t h = 2 + wv[i] * 52 / 15, bx = 12 + i * 27 / 4;
            cv_rect(bx, base - h, 6, h, mix16(t->bar, t->acc, 90));
            cv_rect(bx, base - h, 6, 3, t->acc);
        }
        cv_blit(0, CV_ENV_Y);
        return;
    }
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
    } else if (chip == CHIP_PCE) {                              /* six slim pills */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) cv_rect(x + 8 + i * 7, 34, 5, 14, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_VB) {                               /* ten thin red lines, like its LEDs */
        int32_t lit = (v * 10 + max - 1) / max;
        for (i = 0; i < 10; i++) cv_rect(x + 8 + i * 4, 34, 2, 14, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_VRC6) {                             /* five gems */
        int32_t lit = (v * 5 + max - 1) / max;
        for (i = 0; i < 5; i++) { int32_t gx = x + 8 + i * 8; uint16_t c = i < lit ? t->acc : t->dim; cv_rect(gx + 2, 34, 3, 14, c); cv_rect(gx, 37, 7, 8, c); }
    } else if (chip == CHIP_TIA) {                              /* eight steps going up, like scan lines */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 48 - 4 - i * 2, 3, 4 + i * 2, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_AY) {                               /* eight segments, a colour each */
        static const uint16_t RB[4] = {RGB(224, 40, 40), RGB(236, 210, 40), RGB(40, 200, 60), RGB(40, 200, 220)};
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 36, 4, 10, i < lit ? RB[i >> 1] : t->dim);
    } else if (chip == CHIP_MD) {                               /* six slanted blocks */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) { int32_t bx = x + 8 + i * 7; uint16_t c = i < lit ? t->acc : t->dim; cv_rect(bx, 34, 5, 4, c); cv_rect(bx + 1, 38, 5, 4, c); cv_rect(bx + 2, 42, 4, 4, c); }
    } else if (chip == CHIP_POKEY) {                            /* eight dots, two rows */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 10 + (i & 3) * 10, 34 + (i >> 2) * 8, 7, 6, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_SPK) {                              /* a DOS bar: solid, then shaded */
        int32_t w = 40 * v / max, px, py;
        cv_rect(x + 8, 36, w, 10, t->fg);
        for (py = 0; py < 10; py++) for (px = w; px < 40; px++) if (((px + py) & 1) == 0) cv_rect(x + 8 + px, 36 + py, 1, 1, t->dim);
    } else if (chip == CHIP_OPL) {                              /* a strip of ten little lights */
        int32_t lit = (v * 10 + max - 1) / max;
        for (i = 0; i < 10; i++) cv_rect(x + 8 + i * 4, 38, 3, 6, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_WSG) {                              /* a row of dots, Pac-Man at the start */
        int32_t lit = (v * 6 + max - 1) / max;
        cv_rect(x + 8, 34, 8, 8, t->acc); cv_rect(x + 12, 36, 4, 4, t->bg);
        for (i = 0; i < 6; i++) cv_rect(x + 19 + i * 5, 37, 3, 3, i < lit ? t->fg : t->dim);
    } else if (chip == CHIP_S5B) {                              /* seven rays, short and long */
        int32_t lit = (v * 7 + max - 1) / max;
        for (i = 0; i < 7; i++) cv_rect(x + 8 + i * 6, i & 1 ? 36 : 32, 3, i & 1 ? 12 : 16, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_MMC5) {                             /* six blocks, two rows */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) cv_rect(x + 8 + (i % 3) * 13, 32 + (i / 3) * 10, 11, 8, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_FDS) {                              /* eight bars along a sine */
        static const uint8_t SH[8] = {10, 15, 17, 15, 10, 5, 3, 5};
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 50 - SH[i], 4, SH[i], i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_SCC) {                              /* a staircase of eight */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 46 - i * 2 - 2, 4, 4 + i * 2, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_OPLL) {                             /* twelve thin ticks, every third one tall */
        int32_t lit = (v * 12 + max - 1) / max;
        for (i = 0; i < 12; i++) cv_rect(x + 8 + i * 3, i % 3 ? 38 : 32, 2, i % 3 ? 8 : 14, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_VRC7) {                             /* eight bars, rising */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 48 - i * 2 - 4, 4, 4 + i * 2, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_OPM) {                              /* a zigzag of ten */
        int32_t lit = (v * 10 + max - 1) / max;
        for (i = 0; i < 10; i++) cv_rect(x + 8 + i * 4, i & 1 ? 32 : 40, 3, 10, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_OPL3) {                             /* eight pins, a chip's legs */
        int32_t lit = (v * 8 + max - 1) / max;
        cv_rect(x + 8, 34, 40, 4, t->dim);
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 38, 3, 10, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_WS) {                               /* a row of seven dots, the middle high */
        int32_t lit = (v * 7 + max - 1) / max;
        for (i = 0; i < 7; i++) cv_rect(x + 8 + i * 6, 40 - (i < 4 ? i : 6 - i) * 3, 4, 4, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_SAA) {                              /* six bars, the pairs of the chip's three groups */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) cv_rect(x + 8 + i * 7 - (i & 1 ? 0 : 0), 34 + (i & 1) * 4, 5, 12 - (i & 1) * 4, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_VIC) {                              /* seven square steps, left to right */
        int32_t lit = (v * 7 + max - 1) / max;
        for (i = 0; i < 7; i++) cv_rect(x + 8 + i * 6, 34, 4, 12, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_TED) {                              /* nine thin bars */
        int32_t lit = (v * 9 + max - 1) / max;
        for (i = 0; i < 9; i++) cv_rect(x + 8 + i * 5, 36, 3, 10, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_LYNX) {                             /* eight cells in a row, a bar of noise-ish heights */
        static const uint8_t HH[8] = {6, 12, 8, 14, 5, 11, 9, 13};
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) cv_rect(x + 8 + i * 5, 48 - HH[i], 4, HH[i], i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_PC98) {                             /* eight floppy-shaped cells */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) { cv_rect(x + 8 + i * 5, 36, 4, 10, i < lit ? t->acc : t->dim); cv_rect(x + 8 + i * 5, 36, 1, 1, t->bg); }
    } else if (chip == CHIP_NEO) {                              /* six wide blocks, crimson */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) cv_rect(x + 8 + i * 7, 34, 6, 14, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_NGP) {                              /* ten tiny dots in two rows */
        int32_t lit = (v * 10 + max - 1) / max;
        for (i = 0; i < 10; i++) cv_rect(x + 8 + (i % 5) * 8, 34 + (i / 5) * 8, 6, 6, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_OPL1) {                             /* a strip of nine little lights */
        int32_t lit = (v * 9 + max - 1) / max;
        for (i = 0; i < 9; i++) cv_rect(x + 8 + i * 4, 40, 3, 5, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_OPZ) {                              /* a row of LCD segments */
        int32_t lit = (v * 8 + max - 1) / max;
        for (i = 0; i < 8; i++) { cv_rect(x + 8 + i * 5, 34, 4, 3, i < lit ? t->acc : t->dim); cv_rect(x + 8 + i * 5, 40, 4, 3, i < lit ? t->acc : t->dim); cv_rect(x + 8 + i * 5, 46, 4, 3, i < lit ? t->acc : t->dim); }
    } else if (chip == CHIP_SN77) {                             /* a cyan scribble of ten bars */
        static const uint8_t HS[10] = {4, 9, 14, 8, 3, 7, 13, 10, 5, 11};
        int32_t lit = (v * 10 + max - 1) / max;
        for (i = 0; i < 10; i++) cv_rect(x + 8 + i * 4, 48 - HS[i], 3, HS[i], i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_C30) {                              /* a row of gold bulbs */
        int32_t lit = (v * 7 + max - 1) / max;
        for (i = 0; i < 7; i++) cv_rect(x + 9 + i * 6, 38, 5, 5, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_MSM) {                              /* six drawbars, pulled out further as it rises */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) cv_rect(x + 9 + i * 6, 52 - (4 + i * 3), 4, 4 + i * 3, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_SID) {                              /* ten blocks, like a text screen's */
        int32_t lit = (v * 10 + max - 1) / max;
        for (i = 0; i < 10; i++) cv_rect(x + 8 + i * 4, 34, 3, 14, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_OPN) {                              /* six bars, three tall and three short: FM and SSG */
        int32_t lit = (v * 6 + max - 1) / max;
        for (i = 0; i < 6; i++) cv_rect(x + 8 + i * 7, i < 3 ? 34 : 38, 6, i < 3 ? 14 : 10, i < lit ? t->acc : t->dim);
    } else if (chip == CHIP_ZX) {                               /* the four rainbow stripes as blocks, lit one by one */
        static const uint16_t RB[4] = {RGB(236, 60, 56), RGB(250, 204, 40), RGB(60, 200, 80), RGB(60, 190, 230)};
        int32_t lit = (v * 4 + max - 1) / max;
        for (i = 0; i < 4; i++) cv_rect(x + 8 + i * 10, 34, 8, 14, i < lit ? RB[i] : t->dim);
    } else if (chip == CHIP_GB) {                               /* five chunky cells */
        int32_t lit = (v * 5 + max - 1) / max;
        for (i = 0; i < 5; i++) cv_rect(x + 8 + i * 8, 34, 7, 14, i < lit ? t->acc : t->dim);
    } else {                                                    /* a plain bar in a white outline */
        ring(x + 7, 35, 42, 12, 1, t->fg);
        cv_rect(x + 8, 36, 40, 10, t->bg);
        if (v > 0) cv_rect(x + 8, 36, 40 * v / max, 10, t->acc);
    }
}
static void tri_icon(int32_t x, int32_t y, int32_t up, uint16_t col)               /* a small triangle, 11 wide */
{
    int32_t i;
    for (i = 0; i < 6; i++) cv_rect(x + 5 - i, y + (up ? i : 5 - i), 1 + 2 * i, 1, col);
}
static void sid_card(const theme_t *t, uint32_t chip, const inst_t *in, uint32_t page, uint32_t k, int32_t x)       /* the SID's own cards */
{
    int32_t i;
    if (page == 0u && k == 0u) {                                            /* the wave, small */
        for (i = 0; i < 40; i++) cv_rect(x + 8 + i, 50 - sid_shape(in, (uint32_t)(i * 4096 / 40) & 4095u, (uint32_t)i) * 22 / 4095, 1, 2, t->acc);
    } else if (page == 0u) {                                                /* which chip */
        const uint32_t m = sid_get(in, SP_MODEL) & 1u;
        cv_text(x + 10, 22, &FONT_S, "6581", m ? t->dim : t->acc);
        cv_text(x + 10, 36, &FONT_S, "8580", m ? t->acc : t->dim);
    } else if (page == 4u && k == 2u) {                                     /* low, band, high: which of the three pass */
        static const char *const MN[3] = {"L", "B", "H"};
        const uint32_t m = sid_get(in, SP_MODE) & 7u;
        for (i = 0; i < 3; i++) { cv_rect(x + 5 + i * 16, 34, 14, 15, m >> i & 1u ? t->acc : t->dim); cv_text(x + 8 + i * 16, 36, &FONT_S, MN[i], t->bg); }
    } else {
        const uint32_t n = page == 3u ? SP_PW + k : k == 0u ? SP_CUT : k == 1u ? SP_RES : SP_FENV;
        card_bar(t, chip, x, (int32_t)sid_get(in, n), 15);
    }
}
static void chip_cards(const theme_t *t, uint32_t c, uint32_t page)
{
    static const char *const ENVL[4] = {"ATK", "DEC", "SUS", "REL"};
    static const char *const ARPL[4] = {"SHAPE", "RATE", "DIR", ""};
    static const char *const WAVL[4] = {"SHAPE", "FOLD", "STEP", "TWIST"};
    static const char *const FML[4] = {"RATIO", "INDEX", "FB", "DECAY"};
    static const uint8_t BARS[4] = {2, 4, 6, 9};
    const inst_t *in = &chan[c].in;
    uint32_t chip = KIND_CHIP[chan[c].kind], k, k1 = has_k1(c), fm = has_fm(c);
    int32_t sw = in->sweep;
    cv_begin(240, CV_CARDS_H, t->bg);
    for (k = 0; k < 4u; k++) {
        int32_t x = CV_CARD_X(k), i;
        const char *lab = IS_DRUM(chan[c].kind) && k >= 2u ? "" : page == 3u ? (fm ? FML[k] : WAVL[k]) : page == 2u ? ARPL[k] : page == 1u ? ENVL[k] : k == 0u ? (has_wave(c) ? "WAVE" : chan[c].kind == KD_TIA ? "BUZZ" : chan[c].kind == KD_POKEY ? "DIST" : chan[c].kind == KD_AY || chan[c].kind == KD_S5B || chan[c].kind == KD_PC98_SSG || chan[c].kind == KD_NEO_SSG ? "NOISE" : chan[c].kind == KD_SN77 ? "MIX" : chan[c].kind == KD_MSM ? "FOOT" : IS_OPLL(chan[c].kind) ? "INST" : chan[c].kind == KD_SAA || chan[c].kind == KD_TED || chan[c].kind == KD_VIC ? "NOISE" : chan[c].kind == KD_LYNX ? "TYPE" : fm ? "ALGO" : "DUTY") : k == 1u ? "VOL" : k == 2u ? "VIB" : has_pan(c) ? "PAN" : chan[c].kind == KD_S5B ? "BUZZ" : chan[c].kind == KD_FDS ? "MOD" : chan[c].kind == KD_OPL3 || chan[c].kind == KD_OPZ ? "WAVE" : chan[c].kind == KD_SN77 ? "SLF" : chan[c].kind == KD_MSM ? "QUINT" : "SWEEP";
        uint16_t lc;
        if (chan[c].kind == KD_SID) { static const char *const SL3[4] = {"PW", "PWM", "SYNC", "RING"}, *const SL4[4] = {"CUT", "RES", "MODE", "FENV"}; if (page == 3u) lab = SL3[k]; else if (page == 4u) lab = SL4[k]; else if (page == 0u && k == 0u) lab = "WAVE"; else if (page == 0u && k == 3u) lab = "CHIP"; }
        lc = page == 0u && ((k == 0u && !k1) || (k == 1u && !has_vol(c))) ? t->dim : t->fg;
        skin_box(chip, t, x, 0, CV_CARD_W, CV_CARDS_H, t->bar);
        if (!*lab) continue;
        cv_text(x + (CV_CARD_W - text_w(&FONT_S, lab)) / 2, 4, &FONT_S, lab, lc);
        if (chan[c].kind == KD_SID && (page >= 3u || (page == 0u && (k == 0u || k == 3u)))) { sid_card(t, chip, in, page, k, x); continue; }
        if (page == 3u) {
            if (fm) card_bar(t, chip, x, k == 0u ? in->wave : k == 1u ? in->wf : k == 2u ? in->wc : in->wt, 15);
            else if (k == 0u) {                                                 /* the shape alone, small */
                inst_t base = *in; uint8_t wv[32];
                base.wf = base.wc = base.wt = 0; wave_make(&base, wv);
                for (i = 0; i < 32; i++) cv_rect(x + 8 + i * 40 / 32, 50 - wv[i] * 20 / 15, 2, 2, t->acc);
            } else card_bar(t, chip, x, k == 1u ? in->wf : k == 2u ? in->wc : in->wt, 15);
        } else if (page == 2u) {
            if (k == 0u) {                                                      /* the chord's notes as dots, low to high; a dash is off */
                if (!in->arp) cv_rect(x + 18, 42, 20, 3, t->dim);
                else for (i = 0; i < (int32_t)ARP_N[in->arp - 1u]; i++) cv_rect(x + 8 + i * 11, 49 - ARP_IV[in->arp - 1u][i] * 3 / 2, 8, 4, t->acc);
            } else if (k == 1u) {                                               /* faster: more bars */
                int32_t n = BARS[in->arp_rate & 3u];
                for (i = 0; i < n; i++) cv_rect(x + 8 + i * 40 / n, 32, 3, 18, in->arp ? t->acc : t->dim);
            } else {                                                            /* up, down, up and down */
                uint32_t d = in->arp_dir;
                uint16_t on = in->arp ? t->acc : t->dim;
                tri_icon(x + 7, 32, 1, d == 0u ? on : t->dim);
                tri_icon(x + 23, 32, 0, d == 1u ? on : t->dim);
                tri_icon(x + 39, 28, 1, d == 2u ? on : t->dim); tri_icon(x + 39, 35, 0, d == 2u ? on : t->dim);
            }
        } else if (page == 1u) card_bar(t, chip, x, k == 0u ? in->atk : k == 1u ? in->dec : k == 2u ? in->sus : in->rel, 15);
        else if (k == 0u) {
            if (chan[c].kind == KD_TIA) card_bar(t, chip, x, in->wave, 10);
            else if (chan[c].kind == KD_POKEY) card_bar(t, chip, x, in->wave, 7);
            else if (chan[c].kind == KD_AY || chan[c].kind == KD_S5B) card_bar(t, chip, x, in->wave, 15);
            else if (IS_OPLL(chan[c].kind)) card_bar(t, chip, x, in->wave, 14);
            else if (chan[c].kind == KD_SAA || chan[c].kind == KD_PC98_SSG || chan[c].kind == KD_NEO_SSG || chan[c].kind == KD_OPN_SSG) card_bar(t, chip, x, in->wave, 15);
            else if (chan[c].kind == KD_SN77) card_bar(t, chip, x, in->wave, 3);
            else if (chan[c].kind == KD_MSM) { for (i = 0; i < 4; i++) cv_rect(x + 8 + i * 10, in->wave >> i & 1u ? 30 : 42, 7, in->wave >> i & 1u ? 20 : 8, in->wave >> i & 1u ? t->acc : t->dim); }       /* 16' 8' 4' 2' as drawbars */
            else if (chan[c].kind == KD_TED || chan[c].kind == KD_VIC) card_bar(t, chip, x, in->wave, 1);
            else if (chan[c].kind == KD_LYNX) card_bar(t, chip, x, in->wave, 7);
            else if (fm && (chan[c].kind == KD_OPM || chan[c].kind == KD_PC98_FM || chan[c].kind == KD_NEO_FM || chan[c].kind == KD_OPN_FM || chan[c].kind == KD_OPZ || chan[c].kind == KD_MD)) { for (i = 0; i < 8; i++) cv_rect(x + 6 + i * 6, 34, 5, 12, i == (int32_t)in->duty ? t->acc : t->dim); }       /* which of eight */
            else if (fm) { for (i = 0; i < 4; i++) cv_rect(x + 6 + i * 12, 34, 10, 12, i == (int32_t)in->duty ? t->acc : t->dim); }       /* which of four */
            else if (k1) channel_glyph(c, x + 8, 26, 40, 26, t->acc);
        }
        else if (k == 1u) { if (has_vol(c)) card_bar(t, chip, x, in->vol, 15); }
        else if (k == 2u) card_bar(t, chip, x, in->vib_dep, 15);
        else if (has_buzz(c) && chan[c].kind == KD_MSM) { for (i = 0; i < 2; i++) cv_rect(x + 14 + i * 14, in->pan >> i & 1u ? 30 : 42, 10, in->pan >> i & 1u ? 20 : 8, in->pan >> i & 1u ? t->acc : t->dim); }       /* 5 1/3' and 2 2/3' */
        else if (has_buzz(c) && (chan[c].kind == KD_OPL3 || chan[c].kind == KD_OPZ)) { for (i = 0; i < 8; i++) cv_rect(x + 6 + i * 6, 34, 5, 12, i == (int32_t)in->pan ? t->acc : t->dim); }       /* the OPL3's eight waves */
        else if (has_buzz(c)) { for (i = 0; i < 4; i++) cv_rect(x + 6 + i * 12, 34, 10, 12, i == (int32_t)in->pan ? t->acc : t->dim); }       /* the buzz: off, down, up, triangle; the modulator: off to deep */
        else if (has_pan(c)) {                                      /* left, centre, right */
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
static void chip_panels(const theme_t *t, uint32_t c, uint32_t page)
{
    chip_tabs(t, c);
    chip_env(t, KIND_CHIP[chan[c].kind], &chan[c].in, page, pages(c), has_fm(c), IS_DRUM(chan[c].kind));
    chip_cards(t, c, page);
}
