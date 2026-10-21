/* SPDX-License-Identifier: GPL-3.0-only */
/* The top bar of the rack and the chip screens: 42 px that say what is going on. Idle it shows the transport (play, or pause),
 * a note, the tempo, the page. An action plays a short animation over it: play, pause, save, a channel added or removed,
 * a page flipped, the tempo turned, and a channel waiting to be deleted. Included by chipbox.c after rack.c. */
#define HDR_H 42
enum { AN_NONE, AN_PLAY, AN_PAUSE, AN_SAVE, AN_ADD, AN_REMOVE, AN_PAGE, AN_TEMPO, AN_DELETE, AN_COPY, AN_REC, AN_TAPE, AN_FX, AN_OPEN, AN_OCT, AN_CHAN, AN_VIEW, AN_CRIT, AN_LARP, AN_WAVE, AN_UNDO, AN_KINDS };
static const uint16_t AN_DUR[AN_KINDS] = {0, 650, 550, 1000, 850, 850, 500, 1100, 0, 800, 1100, 0, 0, 1000, 900, 650, 640, 4000, 900, 1200, 600};
static uint32_t an_kind, an_t0, an_end, an_arg;
static uint32_t arp_live, arp_n, arp_cur;        /* the live arp (chipbox.c): on, notes held, the note sounding */
static uint8_t arp_h[8];
static uint16_t hdr_bg;                         /* the bar's background on this screen, for the sheet's folded corner */

static uint16_t sg_color(uint32_t page)
{
    static const uint16_t C[8] = {RGB(235, 64, 52), RGB(240, 150, 40), RGB(235, 215, 60), RGB(120, 200, 70), RGB(60, 190, 170), RGB(70, 150, 255), RGB(150, 110, 235), RGB(235, 100, 170)};
    return C[page & 7u];
}
static void anim_start(uint32_t kind, uint32_t arg) { an_kind = kind; an_t0 = fm1_ms; an_end = fm1_ms + AN_DUR[kind]; an_arg = arg; }
static uint32_t anim_on(void)
{
    if (an_kind == AN_DELETE) return rack_rm;
    if (an_kind == AN_TAPE) return fm1_ms < an_end || (seq_run && seq_rate != 2) || (fx_mask & FXB(FX_TAPE));        /* the tape stays on show while it is not playing as normal */
    if (an_kind == AN_LARP) return arp_live || fm1_ms < an_end;                     /* the live arp: while it is on */
    if (an_kind == AN_FX) return fx_mask != 0 || fm1_ms < an_end;                       /* a punch-in effect: while its key is held, and a moment after */
    return an_kind && fm1_ms < an_end;
}
static void anim_fx(uint32_t pad) { an_kind = AN_FX; an_arg = pad; an_t0 = fm1_ms; an_end = fm1_ms + 900u; }
static void anim_tape(void) { if (an_kind != AN_TAPE) an_t0 = fm1_ms; an_kind = AN_TAPE; an_end = fm1_ms + 1500u; }
static void anim_tempo(void)                    /* turning the tempo goes on with the one animation, it does not start it again at each step */
{
    if (an_kind == AN_TEMPO && fm1_ms < an_end) an_end = fm1_ms + 800u;
    else anim_start(AN_TEMPO, 0);
}
static int32_t ease(uint32_t t, uint32_t d, int32_t a, int32_t b)       /* from a to b, quick at first, slow at the end */
{
    uint32_t p = t >= d ? 256u : t * 256u / d, q = 256u - p;
    return a + (b - a) * (int32_t)(256u - q * q / 256u) / 256;
}
static int32_t swing(uint32_t t, uint32_t period)                       /* -127..127, a sine of the given period in ms */
{
    return VIB[(t % period) * 32u / period];
}

/* ---- pictures: all rectangles and lines */
static void ico_play(int32_t x, int32_t y, int32_t s, uint16_t c)       /* a triangle, s high */
{
    int32_t i, w = s * 4 / 5;
    for (i = 0; i < w; i++) cv_rect(x + i, y + s * i / (2 * w), 1, s - s * i / w, c);
}
static void ico_pause(int32_t x, int32_t y, int32_t s, uint16_t c)      /* two bars, s high */
{
    cv_rect(x, y, s / 3, s, c); cv_rect(x + s * 2 / 3, y, s / 3, s, c);
}
static void ico_chip(int32_t x, int32_t y, uint16_t body, uint16_t pin, uint16_t dark)       /* a chip with pins, 28 x 22 */
{
    int32_t p;
    for (p = 0; p < 4; p++) { cv_rect(x + 4 + p * 6, y, 3, 22, pin); }
    for (p = 0; p < 3; p++) { cv_rect(x, y + 4 + p * 6, 28, 3, pin); }
    cv_rect(x + 3, y + 3, 22, 16, body);
    cv_rect(x + 6, y + 6, 4, 4, dark);
}
/* the consoles, tiny (a Game Boy 20 x 28, an NES 38 x 22, a Master System 38 x 20); x is the left edge, bg shows through the corners */
static int32_t console_w(uint32_t chip) { return chip == CHIP_GB ? 20 : 38; }
static int32_t console_y(uint32_t chip) { return chip == CHIP_GB ? 7 : 11; }              /* where the bar draws it */
static void ico_console(uint32_t chip, int32_t x, int32_t y)
{
    if (chip == CHIP_GB) {
        cv_rect(x, y, 20, 28, RGB(196, 196, 184)); cv_rect(x + 17, y + 25, 3, 3, hdr_bg); cv_rect(x + 18, y + 24, 2, 1, hdr_bg);
        cv_rect(x + 2, y + 2, 16, 12, RGB(74, 74, 96)); cv_rect(x + 4, y + 4, 12, 8, RGB(155, 188, 15)); cv_rect(x + 4, y + 4, 12, 1, RGB(110, 150, 30));
        cv_rect(x + 2, y + 7, 1, 2, RGB(214, 48, 40));
        cv_rect(x + 4, y + 15, 12, 1, RGB(70, 70, 150));
        cv_rect(x + 2, y + 21, 7, 3, RGB(40, 40, 48)); cv_rect(x + 4, y + 19, 3, 7, RGB(40, 40, 48));
        cv_rect(x + 12, y + 22, 3, 3, RGB(150, 30, 80)); cv_rect(x + 16, y + 20, 3, 3, RGB(150, 30, 80));
        cv_rect(x + 6, y + 26, 3, 1, RGB(120, 120, 114)); cv_rect(x + 10, y + 26, 3, 1, RGB(120, 120, 114));
    } else if (chip == CHIP_NES) {              /* the front loader, in its own colours: warm grey top and door, mid grey base, charcoal port block, red lettering */
        cv_rect(x + 3, y, 32, 1, RGB(214, 212, 208)); cv_rect(x + 2, y + 1, 34, 1, RGB(214, 212, 208)); cv_rect(x + 1, y + 2, 36, 1, RGB(208, 206, 202)); cv_rect(x, y + 3, 38, 2, RGB(201, 199, 195));
        cv_rect(x + 26, y + 1, 8, 1, RGB(58, 58, 57)); cv_rect(x + 25, y + 3, 11, 1, RGB(170, 168, 164));
        cv_rect(x, y + 5, 38, 10, RGB(201, 199, 195));
        cv_rect(x + 2, y + 6, 25, 8, RGB(214, 212, 208)); cv_rect(x + 2, y + 13, 25, 1, RGB(160, 158, 154));
        cv_rect(x + 5, y + 8, 9, 1, RGB(154, 42, 38)); cv_rect(x + 5, y + 10, 16, 1, RGB(178, 96, 92));
        cv_rect(x, y + 15, 38, 7, RGB(114, 112, 111));
        cv_rect(x + 3, y + 17, 1, 1, RGB(154, 42, 38)); cv_rect(x + 5, y + 17, 6, 3, RGB(111, 100, 89)); cv_rect(x + 12, y + 17, 6, 3, RGB(111, 100, 89));
        cv_rect(x + 29, y + 5, 8, 17, RGB(58, 58, 57)); cv_rect(x + 31, y + 17, 4, 3, RGB(90, 88, 86));
    } else {                                    /* Master System: black, a slanted top with the red panel and the cartridge slot, a red pinstripe, POWER, ports, the card slot */
        cv_rect(x + 5, y, 33, 1, RGB(48, 48, 54)); cv_rect(x + 4, y + 1, 34, 1, RGB(44, 44, 50)); cv_rect(x + 3, y + 2, 35, 1, RGB(40, 40, 46)); cv_rect(x + 2, y + 3, 36, 6, RGB(36, 36, 42));
        cv_rect(x + 17, y + 1, 14, 8, RGB(196, 40, 44)); cv_rect(x + 18, y, 12, 2, RGB(86, 86, 94)); cv_rect(x + 19, y + 5, 4, 2, RGB(20, 20, 24)); cv_rect(x + 25, y + 5, 4, 2, RGB(240, 240, 244));
        cv_rect(x + 5, y + 3, 6, 1, RGB(232, 232, 238)); cv_rect(x + 12, y + 3, 8, 1, RGB(130, 130, 140));
        cv_rect(x, y + 9, 38, 11, RGB(24, 24, 28)); cv_rect(x, y + 10, 38, 1, RGB(168, 34, 38));
        cv_rect(x + 3, y + 12, 6, 2, RGB(205, 44, 44)); cv_rect(x + 3, y + 15, 7, 3, RGB(10, 10, 14));
        cv_rect(x + 17, y + 14, 5, 3, RGB(66, 66, 72)); cv_rect(x + 24, y + 14, 5, 3, RGB(66, 66, 72));
        cv_rect(x + 31, y + 13, 6, 6, RGB(10, 10, 14)); cv_rect(x + 31, y + 13, 6, 1, RGB(80, 80, 88));
    }
}
/* a console, lifted dy (the animations drop it in from above) */
static void ico_system(uint32_t chip, int32_t x, int32_t y, int32_t dy, uint16_t unused)
{
    (void)unused;
    ico_console(chip, x, y + dy);
}
static void ico_floppy(int32_t x, int32_t y)                            /* a floppy disk, 26 x 26 */
{
    cv_rect(x, y, 26, 26, RGB(170, 175, 190));
    cv_rect(x + 20, y, 6, 6, R_BG);
    cv_rect(x + 7, y, 12, 9, RGB(60, 62, 72)); cv_rect(x + 14, y + 2, 3, 5, RGB(170, 175, 190));
    cv_rect(x + 4, y + 14, 18, 12, RGB(235, 235, 240));
    cv_rect(x + 7, y + 17, 12, 1, RGB(150, 150, 160)); cv_rect(x + 7, y + 21, 12, 1, RGB(150, 150, 160));
}
static void ico_file(int32_t x, int32_t y, uint32_t page, uint16_t ink)        /* a page: a sheet with a folded corner, in the page's colour, its number in it; 24 x 30 */
{
    char n[8];
    uint16_t c = sg_color(page);
    cv_rect(x, y, 24, 30, c);
    cv_rect(x + 17, y, 7, 7, hdr_bg);                                          /* the folded corner */
    cv_rect(x + 17, y, 1, 7, mix16(c, R_BG, 110)); cv_rect(x + 17, y + 6, 7, 1, mix16(c, R_BG, 110));
    cv_rect(x + 18, y + 1, 5, 5, mix16(c, R_BG, 90));
    fmt_u(n, page + 1u);
    cv_text(x + (24 - text_w(&FONT_S, n)) / 2, y + 11, &FONT_S, n, ink);
}
static void ico_view(uint32_t i, int32_t bx, int32_t y, uint16_t c)             /* one of the three views' pictures, 16 x 16: rows (the rack), a chip, boxes (the song) */
{
    if (i == 0u) { cv_rect(bx, y + 1, 16, 3, c); cv_rect(bx, y + 6, 11, 3, c); cv_rect(bx, y + 11, 14, 3, c); }
    else if (i == 1u) { int32_t p; cv_rect(bx + 3, y + 3, 10, 10, c); for (p = 0; p < 3; p++) { cv_rect(bx + 4 + p * 3, y, 2, 16, c); cv_rect(bx, y + 4 + p * 3, 16, 2, c); } cv_rect(bx + 5, y + 5, 6, 6, R_BG); }
    else { cv_rect(bx, y + 3, 4, 10, c); cv_rect(bx + 6, y + 3, 4, 10, c); cv_rect(bx + 12, y + 3, 4, 10, c); }
}
static void ico_check(int32_t x, int32_t y, int32_t s, uint16_t c)      /* a tick, about s wide */
{
    int32_t i;
    for (i = 0; i < s / 3; i++) cv_rect(x + i, y + s / 3 + i - 1, 2, 3, c);
    for (i = 0; i < s * 2 / 3; i++) cv_rect(x + s / 3 + i, y + s * 2 / 3 - i - 1, 2, 3, c);
}
static void ico_cross(int32_t x, int32_t y, int32_t s, uint16_t c)
{
    int32_t i;
    for (i = 0; i < s; i++) { cv_rect(x + i, y + i, 3, 3, c); cv_rect(x + s - i, y + i, 3, 3, c); }
}
static void ico_trash(int32_t x, int32_t y, int32_t lid, uint16_t c)    /* a bin, 22 x 28; lid 0..8 = how far the lid is lifted */
{
    int32_t i;
    cv_rect(x + 3, y + 9, 16, 19, c);
    for (i = 0; i < 3; i++) cv_rect(x + 7 + i * 4, y + 12, 1, 13, R_BG);
    cv_rect(x + lid, y + 5 - lid / 2, 22, 3, c); cv_rect(x + 8 + lid, y + 2 - lid / 2, 6, 3, c);
}
static void ico_metro(int32_t cx, int32_t y, int32_t sw, uint16_t c)    /* a metronome, 24 wide, the arm swung sw px */
{
    int32_t i;
    for (i = 0; i < 30; i++) { int32_t hw = 3 + i * 9 / 30; cv_rect(cx - hw, y + 6 + i, 2 * hw, 1, mix16(R_BG, c, 150)); }
    cv_rect(cx - 12, y + 36, 24, 3, c);
    for (i = 0; i < 3; i++) cv_line(cx + i - 1, y + 34, cx + sw + i - 1, y + 4, R_WHITE);
    cv_rect(cx + sw / 2 - 4, y + 12, 9, 6, RGB(235, 64, 52));
}
static void sparks(int32_t cx, int32_t cy, uint32_t t, uint32_t d, int32_t reach, uint16_t c)     /* eight specks flying out and away */
{
    static const int8_t DX[8] = {10, 7, 0, -7, -10, -7, 0, 7}, DY[8] = {0, 7, 10, 7, 0, -7, -10, -7};
    uint32_t i;
    int32_t r = ease(t, d, 4, reach);
    if (t >= d) return;
    for (i = 0; i < 8u; i++) cv_rect(cx + DX[i] * r / 10 - 1, cy + DY[i] * r / 10 - 1, 3, 3, c);
}

static void disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)                 /* a filled circle */
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t hw = r, lim = r * r - dy * dy;
        while (hw * hw > lim) hw--;
        cv_rect(cx - hw, cy + dy, 2 * hw + 1, 1, c);
    }
}
static void reel(int32_t cx, int32_t cy, int32_t spin)                          /* a tape reel: tape on a hub with three spokes, spin 0..31 */
{
    int32_t i;
    disc(cx, cy, 15, RGB(90, 62, 44)); disc(cx, cy, 12, RGB(60, 44, 34)); disc(cx, cy, 7, RGB(205, 205, 212));
    for (i = 0; i < 3; i++) {
        uint32_t a = (uint32_t)(spin + i * 11) & 31u;
        int32_t x = cx + VIB[(a + 8u) & 31u] * 7 / 127, y = cy + VIB[a] * 7 / 127;
        cv_line(cx, cy, x, y, RGB(70, 72, 82)); cv_line(cx + 1, cy, x + 1, y, RGB(70, 72, 82));
    }
    disc(cx, cy, 2, RGB(70, 72, 82));
}
/* a cassette deck's keys: rewind, forward, play, record, stop. Record and play go down together and stay down. */
static void deck_sym(int32_t sym, int32_t cx, int32_t cy, uint16_t c)
{
    int32_t i;
    if (sym == 0 || sym == 1) {                     /* two triangles, left or right */
        for (i = 0; i < 2; i++) {
            int32_t x = cx - 9 + i * 9, j;
            for (j = 0; j < 8; j++) cv_rect(sym == 1 ? x + j : x + 7 - j, cy - 7 + j * 7 / 8 + 0, 1, 14 - 2 * (j * 7 / 8), c);
        }
    } else if (sym == 2) ico_play(cx - 6, cy - 8, 16, c);
    else if (sym == 3) { cv_rect(cx - 3, cy - 8, 6, 16, c); cv_rect(cx - 5, cy - 6, 10, 12, c); cv_rect(cx - 8, cy - 3, 16, 6, c); }
    else cv_rect(cx - 7, cy - 7, 14, 14, c);
}
static void deck_key(int32_t i, int32_t down)
{
    int32_t x = 34 + i * 35, y = 6 + (down ? 5 : 0);
    uint16_t face = down ? RGB(150, 150, 158) : RGB(208, 208, 215);
    cv_rect(x, 6 + 24, 32, down ? 3 : 8, RGB(70, 72, 82));                       /* the key's front edge: more of it shows when it is up */
    cv_rect(x, y, 32, 24, face);
    cv_rect(x, y, 32, 2, mix16(face, R_WHITE, 110));
    deck_sym(i, x + 16, y + 12, i == 3 ? (down ? RGB(255, 70, 60) : RGB(200, 40, 34)) : RGB(40, 40, 50));
}

/* one frame of an animation over the whole bar; t = ms since it started */
static void fx_anim_frame(uint32_t pad, uint32_t ms);
static void flair_frame(uint32_t kind, uint32_t ms, uint32_t arg, const theme_t *at);
static void anim_frame(const theme_t *t, uint32_t chip)
{
    uint32_t ms = fm1_ms - an_t0;
    const theme_t *at = &THEME[(an_kind == AN_ADD || an_kind == AN_REMOVE) && (an_arg & 255u) < CHIPS ? (an_arg & 255u) : chip];
    switch (an_kind) {
    case AN_PLAY: case AN_PAUSE: {                  /* the tape deck again: PLAY goes down; for a stop, STOP goes down and PLAY springs up */
        int32_t k;
        for (k = 0; k < 5; k++) deck_key(k, an_kind == AN_PLAY ? (k == 2 && ms >= 60) : ((k == 2 && ms < 120) || (k == 4 && ms >= 120)));
        break; }
    case AN_SAVE: {                                 /* a floppy slides down, then a tick */
        int32_t y = ms < 350 ? ease(ms, 350, -30, 8) : 8;
        ico_floppy(94, y);
        if (ms > 450) ico_check(130, 8 + (ms < 600 ? (int32_t)(600 - ms) / 15 : 0), 24, RGB(80, 220, 120));
        break; }
    case AN_ADD: {                                  /* the console drops in with sparks and a plus */
        uint32_t cc = an_arg & 255u;
        int32_t cx = 106 - console_w(cc) / 2, drop = ms < 320 ? ease(ms, 320, -26, 0) : 0;
        ico_system(cc, cx, console_y(cc), drop, 0);
        sparks(106, console_y(cc), ms > 300 ? ms - 300 : 0, 450, 30, at->acc);
        if (ms > 300) { cv_rect(152, 19, 14, 3, R_WHITE); cv_rect(157, 14, 3, 13, R_WHITE); }
        break; }
    case AN_REMOVE: {                               /* the console shakes, then bursts */
        uint32_t cc = an_arg & 255u;
        int32_t cx = 106 - console_w(cc) / 2 + (ms < 400 ? (int32_t)((ms / 40) & 1u) * 4 - 2 : 0);
        if (ms < 400) ico_system(cc, cx, console_y(cc), 0, 0);
        else sparks(106, 21, ms - 400, 450, 40, at->acc);
        if (ms > 400) cv_rect(136, 19, 14, 3, R_WHITE);
        break; }
    case AN_PAGE: {                                 /* the old page slides out, the new one slides in */
        int32_t dir = an_arg >> 8 ? -1 : 1, off = ease(ms, 380, 0, 70);
        uint32_t oldp = (an_arg & 255u) ? (an_arg & 255u) - 1u : 0u;
        ico_file(108 - dir * off, 6, oldp, R_BG);
        ico_file(108 + dir * (70 - off), 6, seq_view, R_BG);
        break; }
    case AN_TEMPO: {                                /* a metronome swings at the tempo, the tempo beside it as it changes */
        uint32_t period = 60000u / (seq_bpm ? seq_bpm : 120u) * 2u;
        char n[8];
        ico_metro(86, 0, swing(ms + period / 4u, period) * 10 / 127, RGB(190, 190, 205));
        fmt_u(n, seq_bpm);
        cv_text(124, 5, &FONT_L, n, R_WHITE);
        break; }
    case AN_REC: {                                  /* a tape deck: REC goes down, then PLAY, and they stay (arg 1: STOP is pressed and both spring up) */
        int32_t k;
        for (k = 0; k < 5; k++) {
            int32_t down = 0;
            if (!an_arg) down = (k == 3 && ms >= 160) || (k == 2 && ms >= 300);
            else down = (k == 4 && ms < 300) || ((k == 2 || k == 3) && ms < 140);
            deck_key(k, down);
        }
        break; }
    case AN_FX: fx_anim_frame(an_arg < FXN ? an_arg : 0u, ms); break;
    case AN_TAPE: {                                 /* the tape: two reels turn at the dial's speed and way, a scale with a needle, the head between */
        int32_t r = seq_rate, a = r < 0 ? -r : r, spin;
        static const uint8_t SPD[4] = {0, 1, 2, 4};
        spin = (int32_t)(fm1_ms / 45u) * SPD[a] * (r < 0 ? -1 : 1);
        reel(34, 21, spin); reel(206, 21, spin);
        cv_line(34, 36, 106, 39, RGB(90, 62, 44)); cv_line(206, 36, 134, 39, RGB(90, 62, 44));
        cv_rect(106, 36, 28, 5, RGB(150, 152, 162));                                              /* the head */
        {   int32_t i, nx = 120 + r * 14;
            for (i = -3; i <= 3; i++) cv_rect(120 + i * 14, i == 0 ? 8 : 12, 2, i == 0 ? 16 : 8, i == 0 ? RGB(235, 235, 240) : RGB(110, 110, 125));
            cv_rect(nx - 3, 27, 8, 4, r == 0 ? RGB(235, 220, 90) : RGB(80, 220, 120));              /* the needle: yellow at pause, green moving */
        }
        break; }
    case AN_COPY: {                                 /* a page is copied: a second sheet slides out of the first */
        char n[8];
        int32_t x2 = ease(ms, 380, 100, 138);
        uint16_t c = sg_color(an_arg ? an_arg - 1u : 0u);
        cv_rect(100, 7, 28, 30, RGB(110, 112, 125));
        cv_rect(x2, 7, 28, 30, RGB(215, 215, 228)); cv_rect(x2, 7, 28, 5, c);
        fmt_u(n, an_arg);
        cv_text(x2 + (28 - text_w(&FONT_S, n)) / 2, 17, &FONT_S, n, R_BG);
        break; }
    case AN_DELETE: {                               /* a bin, its lid lifting, in warning red */
        int32_t lid = 2 + swing(ms, 700) * 3 / 127;
        cv_rect(0, 0, 240, 3, (ms / 350u) & 1u ? RGB(235, 64, 52) : RGB(110, 30, 26));
        cv_rect(0, HDR_H - 4, 240, 3, (ms / 350u) & 1u ? RGB(235, 64, 52) : RGB(110, 30, 26));
        ico_trash(109, 7, lid + 3, RGB(235, 90, 80));
        break; }
    default: flair_frame(an_kind, ms, an_arg, at); break;
    }
    (void)t;
}

/* the bar's usual face */
static void hdr_idle(const theme_t *t, uint32_t view, uint32_t chip, uint32_t sel, uint32_t last_note)
{
    char s[16];
    uint32_t note = view == 1u || view == 3u ? (sel < chan_n ? pat[seq_view][sel][rack_cur].note : 0u) : view == 0u ? last_note : 0u;
    uint16_t fg = view == 0u ? t->fg : RGB(230, 230, 235);
    /* the same places on every screen: the chip's name on the left, the page in the middle with the note beside it, the tempo on the right */
    if (view != 2u && sel < chan_n) ico_system(chip, 6, console_y(chip), 0, 0);
    ico_file(100, 6, seq_view, R_BG);
    if (note) { note_name(s, note); cv_text(134, 14, &FONT_S, s, fg); }
    fmt_u(s, seq_bpm);
    cv_text(234 - text_w(&FONT_L, s), 5, &FONT_L, s, fg);
    if (view == 0u) {                               /* the chip screens keep their console's rule */
        int32_t x;
        if (chip == CHIP_NES) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 40, 240, 1, t->fg); }                 /* the Famicom's red and white */
        else if (chip == CHIP_GB) { for (x = 0; x < 240; x += 6) cv_rect(x, 38, 4, 3, t->fg); }                  /* a dotted rule */
        else { cv_rect(0, 38, 240, 1, t->fg); cv_rect(0, 41, 240, 1, t->fg); }                                   /* a double line */
    } else cv_rect(0, HDR_H - 1, 240, 1, rec_on ? RGB(235, 50, 40) : RGB(50, 50, 60));
    if (seq_run && !seq_hold) {                     /* playing: four dots under the tempo, the beat's lit */
        uint32_t b = seq_ticks / 24u & 3u, i;
        for (i = 0; i < 4u; i++) cv_rect(190 + (int32_t)i * 11, 32, 8, 4, i == b ? (b == 0u ? RGB(80, 220, 120) : R_WHITE) : RGB(60, 60, 74));
    }
    if (rec_on && view != 2u && ((fm1_ms / 350u) & 1u)) {            /* recording: a red dot that blinks */
        cv_rect(72, 15, 12, 12, RGB(235, 50, 40)); cv_rect(70, 17, 16, 8, RGB(235, 50, 40)); cv_rect(74, 13, 8, 16, RGB(235, 50, 40));
    }
}

/* the bottom bar: the three views as tabs of the same width, each in its own colour, the one you are on filled; HOME goes left to right */
#define NAV_Y 216
#define NAV_H 24
static void nav_draw(uint32_t view)
{
    static const uint8_t ORDER[3] = {1, 0, 2};
    static const char *const WORD[3] = {"RACK", "CHIP", "SONG"};
    static const uint16_t COL[3] = {RGB(240, 150, 40), RGB(60, 190, 170), RGB(150, 110, 235)};
    uint32_t i;
    cv_begin(240, NAV_H, R_BG);
    for (i = 0; i < 3u; i++) {
        int32_t x = (int32_t)i * 80;
        uint32_t on = ORDER[i] == view;
        uint16_t c = COL[i], ink = on ? R_BG : mix16(R_BG, c, 190);
        cv_rect(x + 1, 0, 78, NAV_H, on ? c : mix16(R_BG, c, 36));
        cv_rect(x + 1, 0, 78, 3, c);
        ico_view(i, x + 10, 5, ink);
        cv_text(x + 34, 4, &FONT_S, WORD[i], ink);
    }
    cv_blit(0, NAV_Y);
}

static void win_ask(const char *q)               /* a window: the question, OCT+ yes (green), OCT- no (red) */
{
    int32_t i;
    cv_begin(176, 96, RGB(22, 20, 26));
    for (i = 0; i < 2; i++) { cv_rect(i, 0, 176 - 2 * i, 2, RGB(235, 64, 52)); cv_rect(i, 94, 176 - 2 * i, 2, RGB(235, 64, 52)); cv_rect(0, i, 2, 96 - 2 * i, RGB(235, 64, 52)); cv_rect(174, i, 2, 96 - 2 * i, RGB(235, 64, 52)); }
    cv_text((176 - text_w(&FONT_S, q)) / 2, 11, &FONT_S, q, R_WHITE);
    cv_rect(14, 38, 148, 22, RGB(24, 70, 40)); cv_text(24, 41, &FONT_S, "OCT+  Yes", RGB(130, 235, 150));
    cv_rect(14, 66, 148, 22, RGB(80, 26, 22)); cv_text(24, 69, &FONT_S, "OCT-  No", RGB(245, 130, 120));
    cv_blit(32, 100);
}
static void hdr_update(uint32_t view, uint32_t sel, const theme_t *t, uint32_t chip, uint32_t last_note)
{
    static uint32_t sig[16], nav_d = 9;
    uint32_t cur[16], on = anim_on();
    cur[0] = view; cur[1] = sel; cur[2] = view == 1u || view == 3u ? (sel < chan_n ? pat[seq_view][sel][rack_cur].note : 0u) : view == 2u ? 0u : last_note;
    cur[3] = seq_run; cur[4] = seq_bpm; cur[5] = seq_view; cur[6] = page_n; cur[7] = chip; cur[8] = on ? an_kind : 0u;
    cur[9] = on ? (fm1_ms - an_t0) / 50u : 0u; cur[10] = rack_rm; cur[11] = chan_n; cur[12] = rec_on ? 1u + ((fm1_ms / 350u) & 1u) : 0u; cur[13] = (uint32_t)((int32_t)seq_rate + 8) | ((uint32_t)seq_hold << 8);
    cur[14] = on ? an_arg : 0u; cur[15] = seq_run && !seq_hold ? 1u + (seq_ticks / 24u & 3u) : 0u;
    uint32_t full = hdr_force;
    if (!hdr_force && !memcmp(cur, sig, sizeof cur)) return;
    memcpy(sig, cur, sizeof cur);
    hdr_force = 0;
    hdr_bg = view ? R_BG : t->bg;
    cv_begin(240, HDR_H, hdr_bg);
    rk_ox = 0;
    if (on) {
        anim_frame(t, chip);
        if (view == 0u) { /* keep the chip's rule under an animation */
            if (chip == CHIP_NES) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 40, 240, 1, t->fg); }
            else if (chip == CHIP_SMS) { cv_rect(0, 38, 240, 1, t->fg); cv_rect(0, 41, 240, 1, t->fg); }
        }
    } else hdr_idle(t, view, chip, sel, last_note);
    cv_blit(0, 0);
    if (full || view != nav_d) { nav_draw(view); nav_d = view; }
}
