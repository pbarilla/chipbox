/* SPDX-License-Identifier: GPL-3.0-only */
/* Windows: the drawing the popups share (tips, menus, questions). A window is drawn in bands of 80 px (win_band), with mR / mT / mRing at window
 * coordinates; the footer says HOME  Close: HOME closes every window. Included by chipbox.c after rack.c. */
#define M_ACC RGB(240, 170, 50)
#define W_BG RGB(22, 22, 30)
#define W_SHADOW RGB(0, 0, 0)
static int32_t man_off;                                 /* the band being drawn */
static uint32_t man_dry;                                /* measuring: the text is wrapped but not drawn */

static void mR(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x, y - man_off, w, h, c); }
static void mT(int32_t x, int32_t y, const char *s, uint16_t c) { if (!man_dry) cv_text(x, y - man_off, &FONT_S, s, c); }
static void mRing(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    mR(x1, y1, x2 - x1, 2, M_ACC); mR(x1, y2 - 2, x2 - x1, 2, M_ACC); mR(x1, y1, 2, y2 - y1, M_ACC); mR(x2 - 2, y1, 2, y2 - y1, M_ACC);
}
static int32_t mLines(int32_t x, int32_t y, const char *s, int32_t maxw)       /* wrapped on spaces, 17 px a line; returns the next y */
{
    const char *ls = s, *le = s, *p = s;
    char t[64];
    for (;;) {
        const char *we;
        uint32_t n;
        while (*p == ' ') p++;
        we = p;
        while (*we && *we != ' ') we++;
        if (we == p) break;
        n = (uint32_t)(we - ls);
        if (n > 63u) n = 63u;
        memcpy(t, ls, n); t[n] = 0;
        if (le != ls && text_w(&FONT_S, t) > maxw) {
            n = (uint32_t)(le - ls); memcpy(t, ls, n); t[n] = 0;
            mT(x, y, t, RGB(225, 225, 232)); y += 17;
            ls = p;
        }
        le = we; p = we;
    }
    if (le != ls) { uint32_t n = (uint32_t)(le - ls); memcpy(t, ls, n); t[n] = 0; mT(x, y, t, RGB(225, 225, 232)); y += 17; }
    return y;
}
static int32_t mWords(int32_t x, int32_t y, const char *s, int32_t maxw)       /* "\n" starts a new line; returns the next y */
{
    char seg[192];
    for (;;) {
        uint32_t n = 0;
        while (s[n] && s[n] != '\n' && n < 191u) n++;
        memcpy(seg, s, n); seg[n] = 0;
        y = mLines(x, y, seg, maxw);
        if (s[n] != '\n') break;
        s += n + 1u;
    }
    return y;
}
static void win_band(uint32_t b, int32_t w, int32_t h)
{
    int32_t bh = h - (int32_t)b * 80;
    man_off = (int32_t)b * 80;
    cv_begin((uint32_t)w, (uint32_t)(bh < 80 ? bh : 80), W_BG);
}
static void win_chrome(int32_t w, int32_t h, const char *title, uint16_t col)        /* the border and the title bar */
{
    mR(0, 0, w, 2, col); mR(0, h - 2, w, 2, col); mR(0, 0, 2, h, col); mR(w - 2, 0, 2, h, col);
    mR(2, 2, w - 4, 20, col);
    mT(8, 3, title, R_BG);
}
static void win_foot(int32_t w, int32_t h, const char *left)       /* the bottom row: the confirm button at the left (if the window has one), HOME: Close at the right */
{
    mR(10, h - 25, w - 20, 1, RGB(60, 60, 74));
    if (left) mT(10, h - 21, left, RGB(150, 150, 164));
    mT(w - 10 - text_w(&FONT_S, "HOME  Close"), h - 21, "HOME  Close", RGB(150, 150, 164));
}
static void win_shadow(int32_t x, int32_t y, int32_t w, int32_t h)       /* a dark strip at the right and the bottom */
{
    int32_t t;
    win_rx = x; win_ry = y; win_rw = w + 4 < 240 - x ? w + 4 : 240 - x; win_rh = h + 4 < 240 - y ? h + 4 : 240 - y; win_sig = view_sig;
    for (t = 0; t < h; t += 80) { int32_t th = h - t < 80 ? h - t : 80; cv_begin(4, (uint32_t)th, W_SHADOW); cv_blit((uint32_t)(x + w), (uint32_t)(y + 4 + t)); }
    cv_begin((uint32_t)w, 4, W_SHADOW); cv_blit((uint32_t)(x + 4), (uint32_t)(y + h));
}
