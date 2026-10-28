/* SPDX-License-Identifier: GPL-3.0-only */
/* The top bar of the rack and the chip screens: 42 px that say what is going on. Idle it shows the transport (play, or pause),
 * a note, the tempo, the page. An action plays a short animation over it: play, pause, save, a channel added or removed,
 * a page flipped, the tempo turned, and a channel waiting to be deleted. Included by chipbox.c after rack.c. */
#define HDR_H 42
enum { AN_NONE, AN_PLAY, AN_PAUSE, AN_SAVE, AN_ADD, AN_REMOVE, AN_PAGE, AN_TEMPO, AN_DELETE, AN_COPY, AN_REC, AN_TAPE, AN_FX, AN_OPEN, AN_OCT, AN_CHAN, AN_VIEW, AN_CRIT, AN_WAVE, AN_UNDO, AN_KINDS };
static const uint16_t AN_DUR[AN_KINDS] = {0, 650, 550, 1000, 850, 850, 500, 1100, 0, 800, 1100, 0, 0, 1000, 900, 650, 640, 4000, 1200, 600};
static uint32_t an_kind, an_t0, an_end, an_arg;
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
/* the chips' logos, made up: the chip's own number in the console's colours. Returns the width; rack_label is the little 3 x 5 font from rack.c */
static int32_t ico_logo(uint32_t chip, int32_t x, int32_t y)
{
    static const char *const NUM[CHIPS] = {"2A03", "DMG", "SN76489", "HUC6280", "TIA", "VRC6", "AY8910", "YM2612", "VSU", "POKEY", "8253", "YM3812", "WSG", "N163", "5B", "MMC5", "RP2C33", "SCC", "YM2413", "VRC7", "YM2151", "YMF262", "ASWAN", "SAA1099", "6560", "7360", "MIKEY", "YM2608", "YM2610", "T6W28", "Y8950", "YM2414", "SN76477", "C30", "MSM5232", "ULA", "YM2203", "6581"};
    const char *s = NUM[chip < CHIPS ? chip : 0];
    int32_t n = 0, w;
    while (s[n]) n++;
    w = n * 8 - 2 + 10;
    rk_ox = 0;
    if (chip == CHIP_NES) {                     /* a red pill with a white rim */
        cv_rect(x, y, w, 14, R_WHITE); cv_rect(x + 1, y + 1, w - 2, 12, RGB(190, 40, 34));
        rack_label(x + 5, y + 2, s, R_WHITE);
    } else if (chip == CHIP_GB) {               /* dark green with the light green, a slanted stripe */
        cv_rect(x, y, w, 14, RGB(48, 98, 48)); cv_rect(x + w - 8, y, 3, 14, RGB(155, 188, 15)); cv_rect(x + w - 4, y, 2, 14, RGB(110, 150, 30));
        rack_label(x + 4, y + 2, s, RGB(155, 188, 15));
    } else if (chip == CHIP_SMS) {              /* blue with a white underline */
        cv_rect(x, y, w, 14, RGB(40, 90, 210)); cv_rect(x + 3, y + 11, w - 6, 1, R_WHITE);
        rack_label(x + 5, y + 1, s, R_WHITE);
    } else if (chip == CHIP_PCE) {              /* orange, corners cut */
        cv_rect(x + 2, y, w - 4, 14, RGB(240, 130, 32)); cv_rect(x, y + 2, w, 10, RGB(240, 130, 32));
        rack_label(x + 5, y + 2, s, RGB(40, 24, 20));
    } else if (chip == CHIP_TIA) {              /* woodgrain brown, the Atari stripes under the letters */
        cv_rect(x, y, w, 14, RGB(112, 66, 34));
        cv_rect(x + 3, y + 10, w - 6, 1, RGB(236, 130, 40)); cv_rect(x + 3, y + 11, w - 6, 1, RGB(214, 64, 44)); cv_rect(x + 3, y + 12, w - 6, 1, RGB(150, 70, 170));
        rack_label(x + 5, y + 1, s, RGB(244, 226, 196));
    } else if (chip == CHIP_VRC6) {             /* gold, corners cut */
        cv_rect(x + 2, y, w - 4, 14, RGB(236, 184, 40)); cv_rect(x, y + 2, w, 10, RGB(236, 184, 40));
        cv_rect(x + 2, y + 2, w - 4, 1, RGB(255, 226, 120));
        rack_label(x + 5, y + 2, s, RGB(36, 20, 44));
    } else if (chip == CHIP_AY) {               /* black with a white rim, the rainbow underneath */
        cv_rect(x, y, w, 14, R_WHITE); cv_rect(x + 1, y + 1, w - 2, 12, RGB(10, 10, 12));
        cv_rect(x + 1, y + 9, w - 2, 1, RGB(224, 40, 40)); cv_rect(x + 1, y + 10, w - 2, 1, RGB(236, 210, 40)); cv_rect(x + 1, y + 11, w - 2, 1, RGB(40, 200, 60)); cv_rect(x + 1, y + 12, w - 2, 1, RGB(40, 200, 220));
        rack_label(x + 5, y + 1, s, R_WHITE);
    } else if (chip == CHIP_VB) {               /* red on black, two scan lines through it */
        cv_rect(x, y, w, 14, RGB(255, 36, 30)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(10, 0, 0));
        cv_rect(x + 1, y + 6, w - 2, 1, RGB(90, 10, 8));
        rack_label(x + 5, y + 2, s, RGB(255, 36, 30));
    } else if (chip == CHIP_POKEY) {            /* plum with a pink rim, the Atari stripes under the letters */
        cv_rect(x, y, w, 14, RGB(236, 88, 140)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(30, 14, 26));
        cv_rect(x + 1, y + 10, w - 2, 1, RGB(236, 130, 40)); cv_rect(x + 1, y + 11, w - 2, 1, RGB(236, 88, 140)); cv_rect(x + 1, y + 12, w - 2, 1, RGB(150, 70, 170));
        rack_label(x + 5, y + 1, s, RGB(244, 226, 236));
    } else if (chip == CHIP_SPK) {              /* a BIOS: blue, white letters, a grey shadow */
        cv_rect(x + 2, y + 2, w, 14, RGB(70, 70, 80)); cv_rect(x, y, w, 14, RGB(0, 0, 168));
        rack_label(x + 5, y + 2, s, R_WHITE);
    } else if (chip == CHIP_OPL) {              /* a card: dark green with gold contacts along the bottom */
        cv_rect(x, y, w, 14, RGB(10, 44, 36)); cv_rect(x, y, w, 1, RGB(236, 184, 60));
        { int32_t k; for (k = 2; k < w - 2; k += 3) cv_rect(x + k, y + 12, 2, 2, RGB(236, 184, 60)); }
        rack_label(x + 5, y + 2, s, RGB(236, 184, 60));
    } else if (chip == CHIP_WSG) {              /* the arcade: black in a blue maze wall, yellow letters */
        cv_rect(x, y, w, 14, RGB(40, 40, 222)); cv_rect(x + 2, y + 2, w - 4, 10, RGB(4, 4, 18));
        rack_label(x + 5, y + 2, s, RGB(255, 226, 0));
    } else if (chip == CHIP_N163) {             /* coral, corners cut */
        cv_rect(x + 2, y, w - 4, 14, RGB(255, 112, 96)); cv_rect(x, y + 2, w, 10, RGB(255, 112, 96));
        rack_label(x + 5, y + 2, s, RGB(22, 26, 40));
    } else if (chip == CHIP_S5B) {              /* night blue with an amber sun on the left */
        cv_rect(x, y, w, 14, RGB(255, 196, 36)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(10, 14, 34));
        rack_label(x + 5, y + 2, s, RGB(255, 196, 36));
    } else if (chip == CHIP_MMC5) {             /* slate with a mint rim */
        cv_rect(x, y, w, 14, RGB(96, 214, 168)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(18, 30, 30));
        rack_label(x + 5, y + 2, s, RGB(96, 214, 168));
    } else if (chip == CHIP_FDS) {              /* a disk label: blue, an orange stripe at the top */
        cv_rect(x, y, w, 14, RGB(30, 60, 170)); cv_rect(x, y, w, 3, RGB(255, 142, 40));
        rack_label(x + 5, y + 5, s, R_WHITE);
    } else if (chip == CHIP_SCC) {              /* lime, corners cut, dark letters */
        cv_rect(x + 2, y, w - 4, 14, RGB(190, 232, 48)); cv_rect(x, y + 2, w, 10, RGB(190, 232, 48));
        rack_label(x + 5, y + 2, s, RGB(10, 14, 22));
    } else if (chip == CHIP_OPLL) {             /* ocean blue with a cyan rim and a cyan underline */
        cv_rect(x, y, w, 14, RGB(78, 218, 255)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(8, 26, 40));
        cv_rect(x + 3, y + 11, w - 6, 1, RGB(78, 218, 255));
        rack_label(x + 5, y + 2, s, RGB(228, 242, 248));
    } else if (chip == CHIP_VRC7) {             /* vermilion with a dark rim, corners cut */
        cv_rect(x + 2, y, w - 4, 14, RGB(255, 98, 66)); cv_rect(x, y + 2, w, 10, RGB(255, 98, 66));
        cv_rect(x + 2, y + 2, w - 4, 1, RGB(255, 170, 140));
        rack_label(x + 5, y + 2, s, RGB(28, 22, 22));
    } else if (chip == CHIP_OPM) {              /* black violet with a magenta rim and a magenta underline */
        cv_rect(x, y, w, 14, RGB(255, 70, 210)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(30, 8, 32));
        cv_rect(x + 3, y + 11, w - 6, 1, RGB(255, 70, 210));
        rack_label(x + 5, y + 2, s, RGB(244, 226, 244));
    } else if (chip == CHIP_OPL3) {             /* a silver chip on a blue board: pins along the bottom */
        cv_rect(x, y, w, 14, RGB(30, 60, 150)); cv_rect(x + 2, y + 2, w - 4, 8, RGB(18, 18, 24));
        { int32_t k; for (k = 3; k < w - 3; k += 3) cv_rect(x + k, y + 11, 2, 3, RGB(210, 226, 255)); }
        rack_label(x + 5, y + 3, s, RGB(210, 226, 255));
    } else if (chip == CHIP_WS) {               /* indigo with a lavender swan stripe */
        cv_rect(x, y, w, 14, RGB(150, 160, 255)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(22, 18, 62));
        cv_rect(x + 1, y + 1, 3, 12, RGB(150, 160, 255));
        rack_label(x + 8, y + 2, s, RGB(236, 234, 250));
    } else if (chip == CHIP_SAA) {              /* black with a yellow rim and a rainbow underneath */
        cv_rect(x, y, w, 14, RGB(255, 226, 60)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(24, 18, 8));
        cv_rect(x + 1, y + 9, w - 2, 1, RGB(230, 50, 50)); cv_rect(x + 1, y + 10, w - 2, 1, RGB(255, 190, 40)); cv_rect(x + 1, y + 11, w - 2, 1, RGB(60, 200, 80)); cv_rect(x + 1, y + 12, w - 2, 1, RGB(60, 140, 240));
        rack_label(x + 5, y + 1, s, RGB(250, 244, 226));
    } else if (chip == CHIP_VIC) {              /* a blue chip with a pale cyan edge */
        cv_rect(x, y, w, 14, RGB(160, 230, 255)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(30, 44, 120));
        rack_label(x + 5, y + 2, s, RGB(236, 244, 255));
    } else if (chip == CHIP_TED) {              /* grey with a green stripe along the bottom */
        cv_rect(x, y, w, 14, RGB(70, 72, 80)); cv_rect(x, y + 11, w, 3, RGB(140, 255, 120));
        rack_label(x + 5, y + 2, s, R_WHITE);
    } else if (chip == CHIP_LYNX) {             /* black with an orange rim, corners cut */
        cv_rect(x + 2, y, w - 4, 14, RGB(255, 150, 30)); cv_rect(x, y + 2, w, 10, RGB(255, 150, 30));
        cv_rect(x + 2, y + 2, w - 4, 10, RGB(10, 14, 12));
        rack_label(x + 5, y + 2, s, RGB(255, 150, 30));
    } else if (chip == CHIP_PC98) {             /* peach with a beige rim, a dark underline */
        cv_rect(x, y, w, 14, RGB(246, 238, 228)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(255, 170, 120));
        cv_rect(x + 3, y + 11, w - 6, 1, RGB(120, 60, 40));
        rack_label(x + 5, y + 2, s, RGB(60, 26, 20));
    } else if (chip == CHIP_NEO) {              /* black with a crimson rim and a gold line */
        cv_rect(x, y, w, 14, RGB(255, 40, 70)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(24, 8, 10));
        cv_rect(x + 3, y + 11, w - 6, 1, RGB(255, 200, 40));
        rack_label(x + 5, y + 2, s, RGB(246, 232, 232));
    } else if (chip == CHIP_NGP) {              /* violet with a pink band at the top */
        cv_rect(x, y, w, 14, RGB(60, 40, 110)); cv_rect(x, y, w, 3, RGB(255, 110, 170));
        rack_label(x + 5, y + 5, s, R_WHITE);
    } else if (chip == CHIP_OPL1) {             /* a green card with a gold edge */
        cv_rect(x, y, w, 14, RGB(16, 70, 46)); cv_rect(x, y, w, 1, RGB(120, 230, 120));
        { int32_t k; for (k = 2; k < w - 2; k += 3) cv_rect(x + k, y + 12, 2, 2, RGB(236, 184, 60)); }
        rack_label(x + 5, y + 2, s, RGB(180, 250, 180));
    } else if (chip == CHIP_OPZ) {              /* a synth's LCD: cream on charcoal */
        cv_rect(x, y, w, 14, RGB(255, 235, 170)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(30, 26, 22));
        rack_label(x + 5, y + 2, s, RGB(255, 235, 170));
    } else if (chip == CHIP_SN77) {             /* a cyan vector line on black */
        cv_rect(x, y, w, 14, RGB(60, 255, 255)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(4, 8, 16));
        cv_rect(x + 3, y + 11, w - 6, 1, RGB(30, 130, 140));
        rack_label(x + 5, y + 2, s, RGB(60, 255, 255));
    } else if (chip == CHIP_C30) {              /* gold, corners cut, dark red letters */
        cv_rect(x + 2, y, w - 4, 14, RGB(255, 186, 60)); cv_rect(x, y + 2, w, 10, RGB(255, 186, 60));
        rack_label(x + 5, y + 2, s, RGB(36, 10, 12));
    } else if (chip == CHIP_MSM) {              /* black with a raspberry rim, three little drawbars underneath */
        cv_rect(x, y, w, 14, RGB(255, 60, 120)); cv_rect(x + 1, y + 1, w - 2, 12, RGB(24, 18, 26));
        cv_rect(x + 3, y + 11, 3, 1, RGB(255, 60, 120)); cv_rect(x + 8, y + 10, 3, 2, RGB(255, 60, 120)); cv_rect(x + 13, y + 9, 3, 3, RGB(255, 60, 120));
        rack_label(x + 5, y + 1, s, RGB(246, 234, 240));
    } else if (chip == CHIP_SID) {              /* a chip's label: dark blue, a cyan line above and below, light letters */
        cv_rect(x, y, w, 14, RGB(40, 30, 96)); cv_rect(x, y, w, 1, RGB(120, 208, 222)); cv_rect(x, y + 13, w, 1, RGB(120, 208, 222));
        rack_label(x + 5, y + 2, s, RGB(214, 208, 255));
    } else if (chip == CHIP_OPN) {              /* steel blue on charcoal, a slanted sports stripe */
        cv_rect(x, y, w, 14, RGB(26, 34, 46)); cv_rect(x, y, w, 1, RGB(96, 176, 255)); cv_rect(x, y + 13, w, 1, RGB(96, 176, 255));
        cv_rect(x + w - 8, y + 1, 2, 12, RGB(96, 176, 255)); cv_rect(x + w - 5, y + 1, 1, 12, RGB(96, 176, 255));
        rack_label(x + 4, y + 2, s, RGB(226, 232, 240));
    } else if (chip == CHIP_ZX) {               /* black, the four rainbow stripes slanting up at the right */
        cv_rect(x, y, w, 14, RGB(10, 10, 12)); cv_rect(x, y, w, 1, R_WHITE); cv_rect(x, y + 13, w, 1, R_WHITE);
        { int32_t k; static const uint16_t RB[4] = {RGB(236, 60, 56), RGB(250, 204, 40), RGB(60, 200, 80), RGB(60, 190, 230)};
          for (k = 0; k < 4; k++) cv_rect(x + w - 12 + k * 2, y + 1, 2, 12, RB[k]); }
        rack_label(x + 4, y + 2, s, R_WHITE);
    } else {                                    /* Sega black, a blue edge */
        cv_rect(x, y, w, 14, RGB(10, 10, 14)); cv_rect(x, y, w, 1, R_WHITE); cv_rect(x, y + 13, w, 1, R_WHITE);
        cv_rect(x, y + 1, 3, 12, RGB(0, 98, 210));
        rack_label(x + 6, y + 2, s, R_WHITE);
    }
    return w;
}
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
    } else if (chip == CHIP_PCE) {              /* the PC Engine: a flat white box, ribs either side of a raised card slot, an orange logo, a green slider and a round port on the front */
        cv_rect(x + 1, y, 36, 1, RGB(244, 243, 240)); cv_rect(x, y + 1, 38, 11, RGB(236, 235, 230));
        { int32_t r; for (r = 0; r < 4; r++) { cv_rect(x + 3, y + 3 + r * 2, 7, 1, RGB(190, 189, 184)); cv_rect(x + 28, y + 3 + r * 2, 7, 1, RGB(190, 189, 184)); } }
        cv_rect(x + 11, y + 1, 16, 10, RGB(246, 245, 242)); cv_rect(x + 11, y + 10, 16, 1, RGB(206, 205, 200));
        cv_rect(x + 14, y + 2, 10, 2, RGB(232, 96, 72));
        cv_rect(x + 13, y + 6, 12, 2, RGB(34, 40, 36)); cv_rect(x + 15, y + 7, 8, 1, RGB(70, 90, 70));
        cv_rect(x, y + 12, 38, 8, RGB(222, 221, 215)); cv_rect(x + 1, y + 20, 36, 1, RGB(170, 169, 162));
        cv_rect(x + 2, y + 13, 5, 6, RGB(44, 160, 100)); cv_rect(x + 4, y + 13, 1, 6, RGB(30, 120, 76));
        cv_rect(x + 21, y + 13, 4, 1, RGB(26, 26, 30)); cv_rect(x + 20, y + 14, 6, 3, RGB(26, 26, 30)); cv_rect(x + 21, y + 17, 4, 1, RGB(26, 26, 30)); cv_rect(x + 22, y + 15, 2, 1, RGB(90, 90, 98));
    } else if (chip == CHIP_TIA) {              /* Atari 2600: black top with the four switches, a woodgrain front */
        cv_rect(x + 1, y, 36, 1, RGB(70, 70, 74)); cv_rect(x, y + 1, 38, 8, RGB(30, 30, 34));
        cv_rect(x + 3, y + 3, 3, 3, RGB(170, 170, 176)); cv_rect(x + 8, y + 3, 3, 3, RGB(170, 170, 176)); cv_rect(x + 27, y + 3, 3, 3, RGB(170, 170, 176)); cv_rect(x + 32, y + 3, 3, 3, RGB(170, 170, 176));
        cv_rect(x + 14, y + 2, 10, 5, RGB(52, 52, 58));
        cv_rect(x, y + 9, 38, 11, RGB(136, 82, 44));
        cv_rect(x, y + 11, 38, 1, RGB(104, 60, 30)); cv_rect(x, y + 14, 38, 1, RGB(104, 60, 30)); cv_rect(x, y + 17, 38, 1, RGB(104, 60, 30));
        cv_rect(x, y + 20, 38, 1, RGB(60, 34, 18));
    } else if (chip == CHIP_VRC6) {             /* the Famicom: cream top, a deep red front, a gold strip */
        cv_rect(x + 2, y, 34, 1, RGB(240, 236, 224)); cv_rect(x, y + 1, 38, 9, RGB(232, 228, 214));
        cv_rect(x + 8, y + 3, 22, 3, RGB(196, 190, 176)); cv_rect(x + 9, y + 4, 20, 1, RGB(120, 116, 106));
        cv_rect(x, y + 10, 38, 10, RGB(150, 30, 38)); cv_rect(x, y + 10, 38, 1, RGB(196, 60, 60));
        cv_rect(x + 4, y + 13, 8, 2, RGB(236, 184, 40)); cv_rect(x + 14, y + 13, 8, 2, RGB(236, 184, 40));
        cv_rect(x + 28, y + 12, 6, 6, RGB(36, 20, 24)); cv_rect(x + 29, y + 13, 4, 1, RGB(120, 70, 70));
    } else if (chip == CHIP_AY) {               /* a ZX Spectrum: a black slab, rows of grey keys, the rainbow in the corner */
        cv_rect(x + 1, y, 36, 1, RGB(50, 50, 56)); cv_rect(x, y + 1, 38, 19, RGB(18, 18, 22));
        { int32_t r, k; for (r = 0; r < 4; r++) for (k = 0; k < 10; k++) cv_rect(x + 3 + k * 3 + (r & 1), y + 3 + r * 4, 2, 2, RGB(150, 150, 158)); }
        cv_rect(x + 29, y + 15, 2, 4, RGB(224, 40, 40)); cv_rect(x + 31, y + 14, 2, 5, RGB(236, 210, 40)); cv_rect(x + 33, y + 13, 2, 6, RGB(40, 200, 60)); cv_rect(x + 35, y + 12, 2, 7, RGB(40, 200, 220));
    } else if (chip == CHIP_VB) {               /* the Virtual Boy: a black visor with a red top and two lenses, on a stand with two legs */
        cv_rect(x + 3, y, 32, 1, RGB(210, 30, 24)); cv_rect(x + 1, y + 1, 36, 2, RGB(255, 36, 30)); cv_rect(x, y + 3, 38, 11, RGB(18, 14, 16));
        cv_rect(x + 7, y + 6, 8, 6, RGB(60, 8, 6)); cv_rect(x + 23, y + 6, 8, 6, RGB(60, 8, 6)); cv_rect(x + 9, y + 7, 4, 4, RGB(255, 36, 30)); cv_rect(x + 25, y + 7, 4, 4, RGB(255, 36, 30));
        cv_rect(x + 16, y + 6, 6, 5, RGB(40, 36, 38)); cv_rect(x + 17, y + 14, 4, 3, RGB(50, 46, 48));
        cv_rect(x + 8, y + 17, 3, 5, RGB(30, 28, 30)); cv_rect(x + 27, y + 17, 3, 5, RGB(30, 28, 30)); cv_rect(x + 11, y + 17, 16, 2, RGB(30, 28, 30));
    } else if (chip == CHIP_MD) {               /* Mega Drive: black, a round lid with a silver ring, the power slider and 16-BIT */
        cv_rect(x + 2, y, 34, 1, RGB(54, 54, 62)); cv_rect(x, y + 1, 38, 19, RGB(14, 14, 18));
        cv_rect(x + 4, y + 3, 16, 12, RGB(30, 30, 36)); cv_rect(x + 5, y + 2, 14, 14, RGB(30, 30, 36)); cv_rect(x + 6, y + 4, 12, 10, RGB(8, 8, 10));
        cv_rect(x + 6, y + 4, 12, 1, RGB(170, 170, 178));
        cv_rect(x + 24, y + 4, 10, 2, RGB(160, 160, 168)); cv_rect(x + 24, y + 9, 6, 1, RGB(0, 98, 210)); cv_rect(x + 24, y + 12, 10, 3, RGB(52, 52, 58)); cv_rect(x + 25, y + 13, 4, 1, RGB(210, 40, 44));
        cv_rect(x, y + 19, 38, 1, RGB(70, 70, 78));
    } else if (chip == CHIP_POKEY) {            /* an Atari 800XL: a dark wedge, rows of keys, the three stripes */
        cv_rect(x + 2, y, 34, 1, RGB(96, 94, 104)); cv_rect(x, y + 1, 38, 6, RGB(44, 42, 50));
        cv_rect(x + 22, y + 3, 4, 2, RGB(236, 130, 40)); cv_rect(x + 26, y + 3, 4, 2, RGB(214, 64, 44)); cv_rect(x + 30, y + 3, 4, 2, RGB(150, 70, 170));
        cv_rect(x, y + 7, 38, 13, RGB(30, 28, 36));
        { int32_t r, k; for (r = 0; r < 3; r++) for (k = 0; k < 11; k++) cv_rect(x + 2 + k * 3 + r, y + 9 + r * 3, 2, 2, RGB(176, 174, 184)); }
        cv_rect(x + 10, y + 18, 18, 1, RGB(176, 174, 184));
    } else if (chip == CHIP_SPK) {              /* an IBM PC: a beige monitor on a beige box with a floppy slot */
        cv_rect(x + 7, y, 24, 15, RGB(214, 206, 184)); cv_rect(x + 9, y + 2, 20, 10, RGB(8, 8, 10));
        cv_rect(x + 11, y + 4, 8, 1, RGB(170, 170, 176)); cv_rect(x + 11, y + 7, 12, 1, RGB(170, 170, 176)); cv_rect(x + 11, y + 9, 3, 1, RGB(170, 170, 176));
        cv_rect(x + 16, y + 15, 6, 1, RGB(170, 162, 142));
        cv_rect(x, y + 16, 38, 6, RGB(224, 216, 194)); cv_rect(x, y + 21, 38, 1, RGB(150, 144, 126));
        cv_rect(x + 3, y + 18, 12, 2, RGB(40, 40, 46)); cv_rect(x + 18, y + 18, 12, 2, RGB(40, 40, 46)); cv_rect(x + 33, y + 18, 2, 2, RGB(70, 190, 90));
    } else if (chip == CHIP_OPL) {              /* a sound card: a dark green board, two black chips, a gold edge and a grey bracket */
        cv_rect(x + 4, y + 2, 34, 15, RGB(14, 70, 54)); cv_rect(x + 4, y + 2, 34, 1, RGB(30, 110, 84));
        cv_rect(x + 8, y + 5, 12, 6, RGB(16, 16, 20)); cv_rect(x + 24, y + 6, 8, 5, RGB(16, 16, 20));
        cv_rect(x + 9, y + 6, 10, 1, RGB(70, 70, 80)); cv_rect(x + 25, y + 7, 6, 1, RGB(70, 70, 80));
        cv_rect(x + 10, y + 13, 6, 2, RGB(236, 184, 60));
        { int32_t k; for (k = 6; k < 38; k += 2) cv_rect(x + k, y + 17, 1, 3, RGB(236, 184, 60)); }
        cv_rect(x, y, 4, 22, RGB(176, 176, 184)); cv_rect(x + 1, y + 3, 2, 3, RGB(70, 70, 80));
    } else if (chip == CHIP_WSG) {              /* an arcade cabinet: a yellow marquee, a black screen with Pac-Man, a red control panel */
        cv_rect(x + 9, y, 20, 3, RGB(255, 226, 0)); cv_rect(x + 7, y + 3, 24, 19, RGB(28, 36, 140));
        cv_rect(x + 10, y + 4, 18, 9, RGB(2, 2, 8)); cv_rect(x + 13, y + 7, 4, 4, RGB(255, 226, 0)); cv_rect(x + 16, y + 8, 2, 2, RGB(2, 2, 8));
        cv_rect(x + 20, y + 8, 2, 2, RGB(255, 255, 255)); cv_rect(x + 24, y + 8, 2, 2, RGB(255, 255, 255));
        cv_rect(x + 5, y + 14, 28, 3, RGB(208, 48, 48)); cv_rect(x + 14, y + 14, 3, 1, RGB(255, 226, 0)); cv_rect(x + 21, y + 14, 2, 1, RGB(255, 255, 255));
        cv_rect(x + 12, y + 18, 14, 3, RGB(14, 20, 90));
    } else if (chip == CHIP_N163 || chip == CHIP_S5B) {     /* a cartridge: grey for Namco, black for Sunsoft, a label and the ribbed grip */
        uint16_t body = chip == CHIP_N163 ? RGB(160, 160, 170) : RGB(40, 40, 46), lab = chip == CHIP_N163 ? RGB(22, 26, 40) : RGB(255, 196, 36);
        cv_rect(x + 7, y, 24, 22, body); cv_rect(x + 9, y, 20, 1, RGB(210, 210, 218)); cv_rect(x + 7, y + 20, 24, 2, RGB(30, 30, 34));
        cv_rect(x + 10, y + 3, 18, 10, lab);
        if (chip == CHIP_N163) { cv_rect(x + 10, y + 3, 18, 2, RGB(255, 112, 96)); cv_rect(x + 12, y + 7, 10, 1, RGB(236, 238, 246)); cv_rect(x + 12, y + 9, 6, 1, RGB(236, 238, 246)); }
        else { cv_rect(x + 15, y + 5, 8, 6, RGB(10, 14, 34)); cv_rect(x + 17, y + 6, 4, 4, RGB(255, 196, 36)); }
        { int32_t k; for (k = 0; k < 4; k++) cv_rect(x + 10, y + 15 + k, 18, 1, chip == CHIP_N163 ? RGB(120, 120, 130) : RGB(70, 70, 78)); }
    } else if (chip == CHIP_MMC5 || chip == CHIP_OPLL || chip == CHIP_VRC7) {      /* a cartridge: slate for the MMC5 (a 72 pin NES game), blue for the FM one, brown for the VRC7 */
        uint16_t body = chip == CHIP_MMC5 ? RGB(124, 132, 130) : chip == CHIP_VRC7 ? RGB(104, 84, 80) : RGB(30, 70, 96), lab = chip == CHIP_MMC5 ? RGB(18, 30, 30) : chip == CHIP_VRC7 ? RGB(28, 22, 22) : RGB(8, 26, 40), hi = chip == CHIP_MMC5 ? RGB(96, 214, 168) : chip == CHIP_VRC7 ? RGB(255, 98, 66) : RGB(78, 218, 255);
        cv_rect(x + 5, y, 28, 22, body); cv_rect(x + 7, y, 24, 1, RGB(200, 208, 206)); cv_rect(x + 5, y + 20, 28, 2, RGB(30, 34, 34));
        cv_rect(x + 8, y + 3, 22, 11, lab); cv_rect(x + 8, y + 3, 22, 2, hi); cv_rect(x + 11, y + 8, 12, 1, R_WHITE); cv_rect(x + 11, y + 10, 7, 1, R_WHITE);
        { int32_t k; for (k = 0; k < 3; k++) cv_rect(x + 9, y + 16 + k, 20, 1, RGB(70, 78, 76)); }
    } else if (chip == CHIP_FDS) {              /* the Disk System: a cream drive with a red front, a yellow disk sticking out of the slot */
        cv_rect(x + 2, y, 34, 1, RGB(236, 232, 218)); cv_rect(x, y + 1, 38, 8, RGB(226, 222, 206));
        cv_rect(x + 7, y + 3, 24, 4, RGB(150, 146, 134));
        cv_rect(x + 9, y + 6, 20, 9, RGB(240, 200, 40)); cv_rect(x + 12, y + 6, 6, 3, RGB(170, 170, 176)); cv_rect(x + 22, y + 11, 5, 2, RGB(236, 238, 246));
        cv_rect(x, y + 9, 38, 11, RGB(150, 30, 38)); cv_rect(x, y + 9, 38, 1, RGB(196, 60, 60)); cv_rect(x + 5, y + 12, 28, 2, RGB(20, 12, 14)); cv_rect(x + 31, y + 16, 4, 2, RGB(236, 184, 40));
    } else if (chip == CHIP_SCC) {              /* an MSX: a dark wedge with rows of keys, a lime game cartridge in the slot on top */
        cv_rect(x + 10, y, 18, 6, RGB(36, 40, 50)); cv_rect(x + 12, y + 1, 14, 3, RGB(190, 232, 48)); cv_rect(x + 14, y + 2, 7, 1, RGB(10, 14, 22));
        cv_rect(x + 2, y + 6, 34, 2, RGB(84, 90, 104)); cv_rect(x, y + 8, 38, 13, RGB(40, 44, 56));
        { int32_t r, k; for (r = 0; r < 3; r++) for (k = 0; k < 11; k++) cv_rect(x + 2 + k * 3 + r, y + 10 + r * 3, 2, 2, RGB(186, 192, 204)); }
        cv_rect(x, y + 20, 38, 1, RGB(190, 232, 48));
    } else if (chip == CHIP_SAA) {              /* a Sam Coupe: a black wedge, a row of yellow keys on top, a red, yellow, green and blue stripe */
        cv_rect(x + 4, y, 30, 5, RGB(30, 28, 24)); cv_rect(x, y + 5, 38, 14, RGB(36, 32, 26));
        { int32_t k; for (k = 0; k < 10; k++) cv_rect(x + 3 + k * 3, y + 7, 2, 2, RGB(255, 226, 60)); for (k = 0; k < 9; k++) cv_rect(x + 4 + k * 3, y + 11, 2, 2, RGB(190, 186, 170)); }
        cv_rect(x, y + 16, 10, 2, RGB(230, 50, 50)); cv_rect(x + 10, y + 16, 10, 2, RGB(255, 190, 40)); cv_rect(x + 20, y + 16, 10, 2, RGB(60, 200, 80)); cv_rect(x + 30, y + 16, 8, 2, RGB(60, 140, 240));
    } else if (chip == CHIP_VIC) {              /* a VIC-20: a cream slab, brown keys, a dark brown stripe */
        cv_rect(x + 3, y, 32, 3, RGB(214, 204, 176)); cv_rect(x, y + 3, 38, 16, RGB(222, 212, 184));
        cv_rect(x, y + 6, 38, 2, RGB(110, 70, 40));
        { int32_t r, k; for (r = 0; r < 2; r++) for (k = 0; k < 10; k++) cv_rect(x + 3 + k * 3 + r, y + 10 + r * 4, 2, 3, RGB(120, 84, 56)); }
        cv_rect(x, y + 19, 38, 2, RGB(170, 160, 134));
    } else if (chip == CHIP_TED) {              /* a Plus/4: a grey wedge with four cursor keys at the right, a green light */
        cv_rect(x + 4, y, 30, 4, RGB(128, 130, 138)); cv_rect(x, y + 4, 38, 15, RGB(150, 152, 160));
        { int32_t r, k; for (r = 0; r < 3; r++) for (k = 0; k < 7; k++) cv_rect(x + 3 + k * 3 + r, y + 7 + r * 3, 2, 2, RGB(70, 72, 80)); }
        cv_rect(x + 28, y + 10, 2, 2, RGB(70, 72, 80)); cv_rect(x + 31, y + 12, 2, 2, RGB(70, 72, 80)); cv_rect(x + 28, y + 14, 2, 2, RGB(70, 72, 80)); cv_rect(x + 25, y + 12, 2, 2, RGB(70, 72, 80));
        cv_rect(x + 3, y + 17, 5, 1, RGB(140, 255, 120)); cv_rect(x, y + 19, 38, 2, RGB(100, 102, 110));
    } else if (chip == CHIP_LYNX) {             /* a Lynx on its side: a black body, a screen, a cross on the left, two orange buttons on the right */
        cv_rect(x + 3, y + 2, 32, 17, RGB(34, 36, 34)); cv_rect(x + 1, y + 5, 36, 11, RGB(34, 36, 34));
        cv_rect(x + 12, y + 4, 14, 13, RGB(8, 10, 8)); cv_rect(x + 13, y + 5, 12, 11, RGB(120, 150, 110));
        cv_rect(x + 4, y + 10, 5, 2, RGB(90, 94, 90)); cv_rect(x + 5, y + 8, 3, 6, RGB(90, 94, 90));
        cv_rect(x + 29, y + 8, 3, 3, RGB(255, 150, 30)); cv_rect(x + 32, y + 12, 3, 3, RGB(255, 150, 30));
    } else if (chip == CHIP_PC98) {             /* a PC-98: a beige desktop, a monitor on top, two floppy slots */
        cv_rect(x + 8, y, 22, 11, RGB(220, 214, 200)); cv_rect(x + 10, y + 2, 18, 7, RGB(24, 40, 36)); cv_rect(x + 12, y + 4, 8, 1, RGB(120, 255, 160));
        cv_rect(x, y + 11, 38, 10, RGB(232, 224, 208)); cv_rect(x + 4, y + 14, 12, 2, RGB(70, 66, 60)); cv_rect(x + 20, y + 14, 12, 2, RGB(70, 66, 60)); cv_rect(x, y + 20, 38, 1, RGB(160, 150, 134));
    } else if (chip == CHIP_NEO) {              /* a Neo Geo AES: a black slab, a crimson stripe, a gold one, the cartridge slot */
        cv_rect(x, y + 3, 38, 17, RGB(26, 26, 30)); cv_rect(x + 3, y, 32, 3, RGB(40, 40, 46));
        cv_rect(x, y + 8, 38, 3, RGB(255, 40, 70)); cv_rect(x, y + 11, 38, 1, RGB(255, 200, 40));
        cv_rect(x + 8, y + 14, 22, 3, RGB(8, 8, 10)); cv_rect(x + 3, y + 15, 3, 2, RGB(70, 190, 90));
    } else if (chip == CHIP_NGP) {              /* a Neo Geo Pocket on its side: a violet body, a small screen, a stick and two buttons */
        cv_rect(x + 3, y + 3, 32, 16, RGB(90, 60, 150)); cv_rect(x + 1, y + 6, 36, 10, RGB(90, 60, 150));
        cv_rect(x + 12, y + 5, 14, 12, RGB(10, 8, 20)); cv_rect(x + 13, y + 6, 12, 10, RGB(130, 170, 210));
        cv_rect(x + 5, y + 10, 4, 3, RGB(240, 234, 248)); cv_rect(x + 29, y + 8, 3, 3, RGB(255, 110, 170)); cv_rect(x + 32, y + 12, 3, 3, RGB(255, 110, 170));
    } else if (chip == CHIP_OPL1) {             /* a card: a green board, a big chip, gold contacts */
        cv_rect(x + 4, y + 2, 34, 15, RGB(14, 90, 54)); cv_rect(x + 4, y + 2, 34, 1, RGB(60, 170, 100));
        cv_rect(x + 8, y + 5, 14, 7, RGB(16, 16, 20)); cv_rect(x + 9, y + 6, 12, 1, RGB(70, 70, 80)); cv_rect(x + 26, y + 6, 6, 4, RGB(16, 16, 20));
        { int32_t k; for (k = 6; k < 38; k += 2) cv_rect(x + k, y + 17, 1, 3, RGB(236, 184, 60)); }
        cv_rect(x, y, 4, 22, RGB(176, 176, 184)); cv_rect(x + 1, y + 3, 2, 3, RGB(70, 70, 80));
    } else if (chip == CHIP_OPZ) {              /* a rack synth: a black slab, a cream LCD, rows of little buttons */
        cv_rect(x, y + 3, 38, 16, RGB(34, 32, 30)); cv_rect(x, y + 3, 38, 1, RGB(90, 86, 80));
        cv_rect(x + 3, y + 6, 16, 6, RGB(255, 235, 170)); cv_rect(x + 5, y + 8, 10, 1, RGB(60, 52, 40)); cv_rect(x + 5, y + 10, 6, 1, RGB(60, 52, 40));
        { int32_t k; for (k = 0; k < 6; k++) cv_rect(x + 22 + (k % 3) * 5, y + 6 + (k / 3) * 4, 3, 2, RGB(190, 186, 176)); for (k = 0; k < 8; k++) cv_rect(x + 3 + k * 4, y + 14, 3, 3, RGB(110, 106, 98)); }
    } else if (chip == CHIP_SN77) {             /* a 28 pin chip: a black body, a notch, pins down both sides */
        cv_rect(x + 8, y + 1, 22, 19, RGB(24, 24, 28)); cv_rect(x + 8, y + 1, 22, 1, RGB(80, 80, 90)); cv_rect(x + 16, y + 1, 6, 2, RGB(6, 6, 8));
        cv_rect(x + 12, y + 9, 14, 1, RGB(60, 255, 255)); cv_rect(x + 12, y + 12, 9, 1, RGB(60, 255, 255));
        { int32_t k; for (k = 0; k < 7; k++) { cv_rect(x + 4, y + 2 + k * 3, 4, 1, RGB(190, 190, 200)); cv_rect(x + 30, y + 2 + k * 3, 4, 1, RGB(190, 190, 200)); } }
    } else if (chip == CHIP_C30) {              /* a red arcade cabinet: a gold marquee, a black screen, a dark control panel */
        cv_rect(x + 9, y, 20, 3, RGB(255, 186, 60)); cv_rect(x + 7, y + 3, 24, 19, RGB(160, 28, 34));
        cv_rect(x + 10, y + 4, 18, 9, RGB(4, 4, 8)); cv_rect(x + 14, y + 7, 4, 3, RGB(255, 186, 60)); cv_rect(x + 21, y + 8, 3, 3, RGB(240, 240, 250));
        cv_rect(x + 5, y + 14, 28, 3, RGB(40, 12, 14)); cv_rect(x + 14, y + 14, 3, 1, RGB(255, 186, 60)); cv_rect(x + 21, y + 14, 2, 1, RGB(240, 240, 250));
        cv_rect(x + 12, y + 18, 14, 3, RGB(90, 14, 18));
    } else if (chip == CHIP_MSM) {              /* a Poly-800: a black slab, a red and a yellow stripe, little buttons and an LCD, a keyboard along the bottom */
        cv_rect(x, y, 38, 22, RGB(30, 28, 34)); cv_rect(x, y, 38, 1, RGB(100, 96, 106));
        cv_rect(x + 3, y + 3, 10, 5, RGB(255, 200, 80)); cv_rect(x + 5, y + 5, 6, 1, RGB(60, 40, 20));
        { int32_t k; for (k = 0; k < 6; k++) cv_rect(x + 17 + k * 3, y + 4, 2, 3, k % 2 ? RGB(255, 60, 120) : RGB(190, 186, 196)); }
        cv_rect(x, y + 10, 38, 1, RGB(255, 60, 120)); cv_rect(x, y + 11, 38, 1, RGB(255, 200, 80));
        cv_rect(x + 1, y + 13, 36, 8, RGB(236, 234, 238));
        { int32_t k; for (k = 0; k < 12; k++) cv_rect(x + 3 + k * 3, y + 13, 1, 8, RGB(60, 58, 64)); }
        { int32_t k; for (k = 0; k < 8; k++) if (k != 2 && k != 6) cv_rect(x + 3 + k * 3 + 2, y + 13, 2, 5, RGB(24, 22, 28)); }
    } else if (chip == CHIP_SID) {              /* a Commodore 64: a beige breadbin, rows of brown keys, four function keys at the right */
        cv_rect(x + 2, y + 1, 34, 4, RGB(196, 188, 164)); cv_rect(x, y + 5, 38, 16, RGB(224, 216, 192)); cv_rect(x, y + 5, 38, 1, RGB(150, 142, 120));
        { int32_t r, k; for (r = 0; r < 3; r++) for (k = 0; k < 7; k++) cv_rect(x + 3 + k * 3 + (r == 1 ? 1 : 0), y + 8 + r * 4, 2, 3, RGB(112, 92, 72)); }
        { int32_t k; for (k = 0; k < 4; k++) cv_rect(x + 28 + k * 2, y + 8, 1, 11, RGB(150, 130, 100)); }
    } else if (chip == CHIP_OPN) {              /* a PC-8801: a grey-beige slab, a keyboard row, two floppy slots on the right, a blue stripe */
        cv_rect(x + 2, y, 34, 7, RGB(206, 204, 196)); cv_rect(x + 4, y + 2, 12, 3, RGB(20, 30, 34)); cv_rect(x + 22, y + 3, 11, 2, RGB(60, 58, 54));
        cv_rect(x, y + 7, 38, 13, RGB(220, 216, 206)); cv_rect(x, y + 7, 38, 1, RGB(96, 176, 255));
        { int32_t k; for (k = 0; k < 9; k++) cv_rect(x + 3 + k * 3, y + 11, 2, 2, RGB(90, 94, 100)); for (k = 0; k < 9; k++) cv_rect(x + 3 + k * 3, y + 15, 2, 2, RGB(90, 94, 100)); }
    } else if (chip == CHIP_ZX) {               /* a ZX Spectrum 48K: a black slab, four rows of grey rubber keys, the rainbow in the corner */
        cv_rect(x + 2, y + 2, 34, 18, RGB(24, 24, 28)); cv_rect(x, y + 4, 38, 14, RGB(24, 24, 28)); cv_rect(x + 2, y + 2, 34, 1, RGB(70, 70, 78));
        { int32_t r, k; for (r = 0; r < 4; r++) for (k = 0; k < 10; k++) cv_rect(x + 3 + k * 3 + (r & 1), y + 5 + r * 3, 2, 2, RGB(190, 190, 196)); }
        cv_rect(x + 28, y + 3, 2, 2, RGB(236, 60, 56)); cv_rect(x + 30, y + 3, 2, 2, RGB(250, 204, 40)); cv_rect(x + 32, y + 3, 2, 2, RGB(60, 200, 80)); cv_rect(x + 34, y + 3, 2, 2, RGB(60, 190, 230));
    } else if (chip == CHIP_OPM) {              /* an X68000: two black towers side by side, a magenta stripe, floppy slots */
        cv_rect(x + 2, y, 15, 21, RGB(30, 30, 36)); cv_rect(x + 20, y, 15, 21, RGB(30, 30, 36));
        cv_rect(x + 2, y, 15, 1, RGB(90, 90, 100)); cv_rect(x + 20, y, 15, 1, RGB(90, 90, 100));
        cv_rect(x + 4, y + 4, 11, 2, RGB(8, 8, 10)); cv_rect(x + 4, y + 8, 11, 2, RGB(8, 8, 10)); cv_rect(x + 22, y + 4, 11, 2, RGB(8, 8, 10));
        cv_rect(x + 2, y + 13, 15, 2, RGB(255, 70, 210)); cv_rect(x + 20, y + 13, 15, 2, RGB(255, 70, 210));
        cv_rect(x + 22, y + 17, 3, 2, RGB(70, 190, 90)); cv_rect(x + 5, y + 17, 3, 2, RGB(70, 190, 90));
    } else if (chip == CHIP_OPL3) {             /* a sound card: a blue board, a big black chip, a silver edge and bracket */
        cv_rect(x + 4, y + 2, 34, 15, RGB(20, 52, 130)); cv_rect(x + 4, y + 2, 34, 1, RGB(60, 100, 190));
        cv_rect(x + 8, y + 5, 16, 8, RGB(16, 16, 20)); cv_rect(x + 9, y + 6, 14, 1, RGB(70, 70, 80)); cv_rect(x + 28, y + 6, 6, 4, RGB(16, 16, 20));
        { int32_t k; for (k = 6; k < 38; k += 2) cv_rect(x + k, y + 17, 1, 3, RGB(210, 226, 255)); }
        cv_rect(x, y, 4, 22, RGB(176, 176, 184)); cv_rect(x + 1, y + 3, 2, 3, RGB(70, 70, 80));
    } else if (chip == CHIP_WS) {               /* a handheld on its side: a pearl body, a small black screen, two button groups */
        cv_rect(x + 3, y + 3, 32, 16, RGB(226, 228, 242)); cv_rect(x + 5, y + 1, 28, 2, RGB(226, 228, 242)); cv_rect(x + 5, y + 19, 28, 2, RGB(226, 228, 242));
        cv_rect(x + 13, y + 5, 12, 12, RGB(40, 44, 40)); cv_rect(x + 14, y + 6, 10, 10, RGB(150, 164, 140));
        cv_rect(x + 6, y + 9, 5, 2, RGB(70, 76, 120)); cv_rect(x + 7, y + 7, 2, 6, RGB(70, 76, 120));
        cv_rect(x + 27, y + 7, 3, 3, RGB(150, 160, 255)); cv_rect(x + 30, y + 11, 3, 3, RGB(150, 160, 255));
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
static void hdr_beat(void)                      /* playing: four dots under the tempo, the beat's lit (over an animation too) */
{
    uint32_t b = (uint32_t)seq_step / 4u & 3u, i;
    if (!seq_run || seq_hold) return;
    for (i = 0; i < 4u; i++) cv_rect(190 + (int32_t)i * 11, 32, 8, 4, i == b ? (b == 0u ? RGB(80, 220, 120) : R_WHITE) : RGB(60, 60, 74));
}
/* the rule under the bar on the chip screens, each console's own */
static void hdr_rule(uint32_t chip, const theme_t *t)
{
    int32_t x;
    if (chip == CHIP_NES || chip == CHIP_VRC6) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 40, 240, 1, t->fg); }       /* red and white; gold and cream */
    else if (chip == CHIP_GB) { for (x = 0; x < 240; x += 6) cv_rect(x, 38, 4, 3, t->fg); }                  /* a dotted rule */
    else if (chip == CHIP_PCE) cv_rect(0, 38, 240, 3, t->acc);                                               /* an orange bar */
    else if (chip == CHIP_TIA) { cv_rect(0, 38, 240, 1, RGB(236, 130, 40)); cv_rect(0, 39, 240, 1, RGB(214, 64, 44)); cv_rect(0, 40, 240, 1, RGB(150, 70, 170)); cv_rect(0, 41, 240, 1, RGB(0, 98, 210)); }
    else if (chip == CHIP_AY) { cv_rect(0, 38, 240, 1, RGB(224, 40, 40)); cv_rect(0, 39, 240, 1, RGB(236, 210, 40)); cv_rect(0, 40, 240, 1, RGB(40, 200, 60)); cv_rect(0, 41, 240, 1, RGB(40, 200, 220)); }
    else if (chip == CHIP_MD) { cv_rect(0, 38, 240, 3, t->acc); cv_rect(0, 41, 240, 1, t->fg); }
    else if (chip == CHIP_VB) { cv_rect(0, 38, 240, 1, t->acc); cv_rect(0, 40, 240, 1, t->dim); cv_rect(0, 41, 240, 1, t->acc); }
    else if (chip == CHIP_POKEY) { cv_rect(0, 38, 240, 1, RGB(236, 130, 40)); cv_rect(0, 39, 240, 1, RGB(236, 88, 140)); cv_rect(0, 40, 240, 1, RGB(150, 70, 170)); }
    else if (chip == CHIP_SPK) { cv_rect(0, 38, 240, 1, t->fg); cv_rect(0, 40, 240, 1, t->fg); }
    else if (chip == CHIP_OPL) { for (x = 0; x < 240; x += 4) cv_rect(x, 38, 2, 3, t->acc); }                      /* gold contacts */
    else if (chip == CHIP_WSG) { for (x = 4; x < 240; x += 12) cv_rect(x, 39, 4, 3, t->acc); cv_rect(0, 38, 240, 1, RGB(40, 40, 222)); }       /* a maze wall, dots */
    else if (chip == CHIP_N163) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->dim); }
    else if (chip == CHIP_S5B) { cv_rect(0, 38, 240, 1, t->acc); cv_rect(0, 41, 240, 1, t->acc); }                  /* thin red lines */
    else if (chip == CHIP_MMC5) { for (x = 0; x < 240; x += 8) cv_rect(x, 38, 6, 2, t->acc); }                      /* a dashed mint line */
    else if (chip == CHIP_FDS) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->fg); }                  /* orange over white */
    else if (chip == CHIP_SCC) { for (x = 0; x < 240; x += 12) { cv_rect(x, 41, 4, 1, t->acc); cv_rect(x + 4, 40, 4, 1, t->acc); cv_rect(x + 8, 39, 4, 1, t->acc); cv_rect(x, 38, 12, 1, t->dim); } }      /* lime steps */
    else if (chip == CHIP_OPLL) { for (x = 0; x < 240; x += 6) cv_rect(x, 38 + (((x / 6) & 3) == 1 || ((x / 6) & 3) == 2 ? 0 : 2), 4, 2, t->acc); }       /* a cyan wave */
    else if (chip == CHIP_VRC7) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->dim); }               /* a vermilion bar */
    else if (chip == CHIP_OPM) { for (x = 0; x < 240; x += 8) cv_rect(x, 38 + (((x / 8) & 1) ? 2 : 0), 6, 2, t->acc); }       /* a magenta zigzag */
    else if (chip == CHIP_OPL3) { cv_rect(0, 38, 240, 1, t->acc); for (x = 2; x < 240; x += 6) cv_rect(x, 40, 2, 2, t->acc); }       /* a silver line over pins */
    else if (chip == CHIP_WS) { cv_rect(0, 38, 240, 1, t->acc); cv_rect(0, 41, 240, 1, t->dim); }             /* lavender lines */
    else if (chip == CHIP_SAA) { cv_rect(0, 38, 240, 1, RGB(230, 50, 50)); cv_rect(0, 39, 240, 1, RGB(255, 190, 40)); cv_rect(0, 40, 240, 1, RGB(60, 200, 80)); cv_rect(0, 41, 240, 1, RGB(60, 140, 240)); }       /* the rainbow */
    else if (chip == CHIP_VIC) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->fg); }                 /* cyan over white */
    else if (chip == CHIP_TED) { cv_rect(0, 38, 240, 1, t->dim); cv_rect(0, 39, 240, 3, t->acc); }               /* a green band */
    else if (chip == CHIP_LYNX) { for (x = 0; x < 240; x += 10) cv_rect(x, 38, 6, 3, t->acc); }                  /* orange dashes */
    else if (chip == CHIP_PC98) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->dim); }               /* a peach bar */
    else if (chip == CHIP_NEO) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 40, 240, 1, RGB(255, 200, 40)); }       /* crimson over gold */
    else if (chip == CHIP_NGP) { cv_rect(0, 38, 240, 1, t->acc); for (x = 0; x < 240; x += 4) cv_rect(x, 40, 2, 1, t->acc); }       /* pink over dots */
    else if (chip == CHIP_OPL1) { for (x = 0; x < 240; x += 4) cv_rect(x, 38, 2, 3, t->acc); cv_rect(0, 41, 240, 1, t->dim); }       /* green contacts */
    else if (chip == CHIP_OPZ) { cv_rect(0, 38, 240, 1, t->acc); cv_rect(0, 40, 240, 1, t->acc); }               /* two cream lines */
    else if (chip == CHIP_SN77) { for (x = 0; x < 240; x += 2) cv_rect(x, 38 + (int32_t)(((x * 7) / 5) % 5 > 2 ? 2 : 0), 2, 2, t->acc); }       /* a cyan scribble */
    else if (chip == CHIP_C30) { cv_rect(0, 38, 240, 3, t->acc); cv_rect(0, 41, 240, 1, t->dim); }               /* a gold marquee bar */
    else if (chip == CHIP_MSM) { cv_rect(0, 38, 240, 1, t->acc); for (x = 4; x < 240; x += 12) cv_rect(x, 39, 2, 1 + ((x / 12) % 3), t->acc); }       /* drawbars hanging off a line */
    else if (chip == CHIP_SID) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->dim); }               /* a cyan line on the blue */
    else if (chip == CHIP_OPN) { cv_rect(0, 38, 240, 2, t->acc); cv_rect(0, 41, 240, 1, t->dim); }               /* a steel bar */
    else if (chip == CHIP_ZX) { for (x = 0; x < 240; x += 1) { int32_t q = (x / 6) & 3; cv_rect(x, 38 + q, 1, 1, q == 0 ? RGB(236, 60, 56) : q == 1 ? RGB(250, 204, 40) : q == 2 ? RGB(60, 200, 80) : RGB(60, 190, 230)); } }       /* a slanting rainbow */
    else { cv_rect(0, 38, 240, 1, t->fg); cv_rect(0, 41, 240, 1, t->fg); }                                   /* a double line */
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
    uint32_t note = view == 1u || view == 3u ? (sel < chan_n && !IS_DRUM(chan[sel].kind) ? pat[seq_view][sel][rack_cur].note : 0u) : view == 0u ? last_note : 0u;
    uint16_t fg = view == 0u ? t->fg : RGB(230, 230, 235);
    /* the same places on every screen: the chip's name on the left, the page in the middle with the note beside it, the tempo on the right */
    if (view != 2u && sel < chan_n) ico_system(chip, 6, console_y(chip), 0, 0);
    ico_file(100, 6, seq_view, R_BG);
    if (note) { note_name(s, note); cv_text(134, 14, &FONT_S, s, fg); }
    fmt_u(s, seq_bpm);
    cv_text(234 - text_w(&FONT_L, s), 5, &FONT_L, s, fg);
    if (view == 0u) hdr_rule(chip, t);              /* the chip screens keep their console's rule */
    else cv_rect(0, HDR_H - 1, 240, 1, rec_on ? RGB(235, 50, 40) : RGB(50, 50, 60));
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
    const int32_t w = 216, h = 134, x = 12, y0 = 52;
    uint32_t b;
    win_shadow(x, y0, w, h);
    for (b = 0; b * 80u < (uint32_t)h; b++) {
        win_band(b, w, h);
        win_chrome(w, h, "Confirm", RGB(235, 64, 52));
        mT((w - text_w(&FONT_S, q)) / 2, 30, q, R_WHITE);
        mR(14, 56, w - 28, 22, RGB(24, 70, 40)); mT(24, 59, "OCT+  Yes", RGB(130, 235, 150));
        mR(14, 82, w - 28, 22, RGB(80, 26, 22)); mT(24, 85, "OCT-  No", RGB(245, 130, 120));
        win_foot(w, h, 0);
        cv_blit(x, y0 + b * 80u);
    }
}
static void hdr_update(uint32_t view, uint32_t sel, const theme_t *t, uint32_t chip, uint32_t last_note)
{
    static uint32_t sig[16], nav_d = 9;
    uint32_t cur[16], on = anim_on();
    cur[0] = view; cur[1] = sel; cur[2] = view == 1u || view == 3u ? (sel < chan_n && !IS_DRUM(chan[sel].kind) ? pat[seq_view][sel][rack_cur].note : 0u) : view == 2u ? 0u : last_note;
    cur[3] = seq_run; cur[4] = seq_bpm; cur[5] = seq_view; cur[6] = page_n; cur[7] = chip; cur[8] = on ? an_kind : 0u;
    cur[9] = on ? (fm1_ms - an_t0) / 50u : 0u; cur[10] = rack_rm; cur[11] = chan_n; cur[12] = rec_on ? 1u + ((fm1_ms / 350u) & 1u) : 0u; cur[13] = (uint32_t)((int32_t)seq_rate + 8) | ((uint32_t)seq_hold << 8);
    cur[14] = on ? an_arg : 0u; cur[15] = seq_run && !seq_hold ? 1u + ((uint32_t)seq_step / 4u & 3u) : 0u;
    uint32_t full = hdr_force;
    if (!hdr_force && !memcmp(cur, sig, sizeof cur)) return;
    memcpy(sig, cur, sizeof cur);
    hdr_force = 0;
    hdr_bg = view ? R_BG : t->bg;
    cv_begin(240, HDR_H, hdr_bg);
    rk_ox = 0;
    if (on) {
        anim_frame(t, chip);
        if (view == 0u && chip != CHIP_GB) hdr_rule(chip, t);       /* keep the chip's rule under an animation */
    } else hdr_idle(t, view, chip, sel, last_note);
    if (!on || an_kind != AN_CRIT) hdr_beat();
    cv_blit(0, 0);
    if (full || view != nav_d) { nav_draw(view); nav_d = view; }
}
