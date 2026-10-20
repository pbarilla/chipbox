/* SPDX-License-Identifier: GPL-3.0-only */
/* The top bar of the rack and the chip screens: 42 px that say what is going on. Idle it shows the transport (play, or pause),
 * a note, the tempo, the page. An action plays a short animation over it: play, pause, save, a channel added or removed,
 * a page flipped, the tempo turned, and a channel waiting to be deleted. Included by chipbox.c after rack.c. */
#define HDR_H 42
enum { AN_NONE, AN_PLAY, AN_PAUSE, AN_SAVE, AN_ADD, AN_REMOVE, AN_PAGE, AN_TEMPO, AN_DELETE, AN_COPY, AN_KINDS };
static const uint16_t AN_DUR[AN_KINDS] = {0, 650, 550, 1000, 850, 850, 500, 1100, 0, 800};
static uint32_t an_kind, an_t0, an_end, an_arg;
static uint16_t hdr_bg;                         /* the bar's background on this screen, for the sheet's folded corner */

static uint16_t sg_color(uint32_t page)
{
    static const uint16_t C[8] = {RGB(235, 64, 52), RGB(240, 150, 40), RGB(235, 215, 60), RGB(120, 200, 70), RGB(60, 190, 170), RGB(70, 150, 255), RGB(150, 110, 235), RGB(235, 100, 170)};
    return C[page & 7u];
}
static void anim_start(uint32_t kind, uint32_t arg) { an_kind = kind; an_t0 = fm1_ms; an_end = fm1_ms + AN_DUR[kind]; an_arg = arg; }
static uint32_t anim_on(void) { return an_kind == AN_DELETE ? rack_rm : (an_kind && fm1_ms < an_end); }
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

/* one frame of an animation over the whole bar; t = ms since it started */
static void anim_frame(const theme_t *t, uint32_t chip)
{
    uint32_t ms = fm1_ms - an_t0;
    const theme_t *at = &THEME[an_arg < CHIPS ? an_arg : chip];
    switch (an_kind) {
    case AN_PLAY: {                                 /* the triangle swells in the middle, sound waves go out */
        int32_t s = ease(ms, 300, 6, 30), i;
        ico_play(112 - s / 3, 21 - s / 2, s, RGB(80, 220, 120));
        for (i = 0; i < 3; i++) {
            int32_t h = (int32_t)(ms > (uint32_t)i * 90u ? ease(ms - (uint32_t)i * 90u, 350, 2, 22) : 2);
            cv_rect(150 + i * 14, 21 - h / 2, 5, h, RGB(80, 220, 120));
            cv_rect(98 - i * 14 - 5, 21 - h / 2, 5, h, RGB(80, 220, 120));
        }
        break; }
    case AN_PAUSE: {                                /* two bars drop in and settle */
        int32_t y = ms < 250 ? ease(ms, 250, -30, 8) : 8 + (ms < 400 ? (int32_t)(400 - ms) / 40 * (ms & 1u ? 1 : -1) : 0);
        ico_pause(106, y, 26, RGB(230, 230, 235));
        break; }
    case AN_SAVE: {                                 /* a floppy slides down, then a tick */
        int32_t y = ms < 350 ? ease(ms, 350, -30, 8) : 8;
        ico_floppy(94, y);
        if (ms > 450) ico_check(130, 8 + (ms < 600 ? (int32_t)(600 - ms) / 15 : 0), 24, RGB(80, 220, 120));
        break; }
    case AN_ADD: {                                  /* a chip pops in with sparks and a plus */
        int32_t y = ms < 300 ? ease(ms, 300, -26, 10) : 10 + (ms < 450 ? (int32_t)(450 - ms) / 50 : 0);
        ico_chip(92, y, at->acc, RGB(200, 200, 210), at->bg);
        sparks(106, 21, ms > 250 ? ms - 250 : 0, 500, 34, at->acc);
        if (ms > 300) { cv_rect(138, 19, 14, 3, R_WHITE); cv_rect(143, 14, 3, 13, R_WHITE); }
        break; }
    case AN_REMOVE: {                               /* the chip shakes, then bursts */
        if (ms < 400) ico_chip(92 + (int32_t)((ms / 40) & 1u) * 4 - 2, 10, at->acc, RGB(200, 200, 210), at->bg);
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
    default: break;
    }
    (void)t;
}

/* the bar's usual face */
static void hdr_idle(const theme_t *t, uint32_t view, uint32_t chip, uint32_t sel, uint32_t last_note)
{
    char s[16];
    uint32_t note = view == 1u ? (sel < chan_n ? pat[seq_view][sel][rack_cur].note : 0u) : view == 0u ? last_note : 0u;
    uint16_t fg = view == 0u ? t->fg : RGB(230, 230, 235);
    /* the same places on every screen: the chip's name on the left, the page in the middle with the note beside it, the tempo on the right */
    if (view != 2u && sel < chan_n) cv_text(8, 5, &FONT_L, chip == CHIP_GB ? "GB" : THEME[chip].name, fg);
    ico_file(100, 6, seq_view, R_BG);
    if (note) { note_name(s, note); cv_text(134, 14, &FONT_S, s, fg); }
    fmt_u(s, seq_bpm);
    cv_text(234 - text_w(&FONT_L, s), 5, &FONT_L, s, fg);
    if (view == 0u) {                               /* the chip screens keep their console's rule */
        int32_t x;
        if (chip == CHIP_NES) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 40, 240, 1, t->fg); }                 /* the Famicom's red and white */
        else if (chip == CHIP_GB) { for (x = 0; x < 240; x += 6) cv_rect(x, 38, 4, 3, t->fg); }                  /* a dotted rule */
        else { cv_rect(0, 38, 240, 1, t->fg); cv_rect(0, 41, 240, 1, t->fg); }                                   /* a double line */
    } else cv_rect(0, HDR_H - 1, 240, 1, RGB(50, 50, 60));
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

static void hdr_popup(void)                     /* over the rack: delete this channel? a bin, OCT+ (yes, green) and OCT- (no, red) */
{
    int32_t i;
    cv_begin(176, 96, RGB(22, 20, 26));
    for (i = 0; i < 2; i++) { cv_rect(i, 0, 176 - 2 * i, 2, RGB(235, 64, 52)); cv_rect(i, 94, 176 - 2 * i, 2, RGB(235, 64, 52)); cv_rect(0, i, 2, 96 - 2 * i, RGB(235, 64, 52)); cv_rect(174, i, 2, 96 - 2 * i, RGB(235, 64, 52)); }
    ico_trash(22, 30, 6, RGB(235, 90, 80));
    cv_rect(80, 12, 80, 32, RGB(24, 70, 40)); cv_rect(88, 27, 12, 3, RGB(230, 235, 230)); cv_rect(92, 23, 3, 11, RGB(230, 235, 230)); ico_check(120, 17, 24, RGB(80, 220, 120));
    cv_rect(80, 52, 80, 32, RGB(80, 26, 22)); cv_rect(88, 67, 12, 3, RGB(235, 225, 225)); ico_cross(122, 59, 16, RGB(235, 90, 80));
    cv_blit(32, 100);
}
static void hdr_update(uint32_t view, uint32_t sel, const theme_t *t, uint32_t chip, uint32_t last_note)
{
    static uint32_t sig[14], nav_d = 9;
    uint32_t cur[14], on = anim_on();
    cur[0] = view; cur[1] = sel; cur[2] = view == 1u ? (sel < chan_n ? pat[seq_view][sel][rack_cur].note : 0u) : view == 2u ? 0u : last_note;
    cur[3] = seq_run; cur[4] = seq_bpm; cur[5] = seq_view; cur[6] = page_n; cur[7] = chip; cur[8] = on ? an_kind : 0u;
    cur[9] = on ? (fm1_ms - an_t0) / 50u : 0u; cur[10] = rack_rm; cur[11] = chan_n; cur[12] = song_cur; cur[13] = song_len;
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
