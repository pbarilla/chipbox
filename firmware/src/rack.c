/* SPDX-License-Identifier: GPL-3.0-only */
/* The rack (HOME): every channel's 16 steps at once, a row each, coloured by chip; the last row adds a channel.
 * A white line is the playhead. It moves in 2 px slices (six a step): only the column it leaves and the
 * one it enters are redrawn. Included by chipbox.c. */
#define R_BG RGB(14, 14, 18)
#define R_SEL RGB(32, 32, 42)
#define R_WHITE RGB(255, 255, 255)
#define R_ROW 19                                /* a row: 15 px of blocks, 2 above and below */
#define R_BAND(b) (44 + (b) * 57)               /* top of three rows, under the 42 px bar and over the 24 px tab bar; 9 rows show, the list scrolls */
#define R_ROWS 9u
#define R_X0 30
static uint32_t hdr_force;                      /* set when a whole screen was painted over the top bar */
static void win_ask(const char *q);
static uint32_t song_cur, song_top;             /* the song's cursor (a slot; one past the last is the "add" cell) and its first row shown */
static uint32_t rec_on;                         /* recording: keys land on the step the playhead is at */
static uint32_t rack_cur, rack_inval, rack_rm, rack_top;     /* the cursor's step; set to repaint after an edit; a channel waits to be removed; the first row shown */

static uint16_t mix16(uint16_t a, uint16_t b, uint32_t k)       /* a towards b by k / 256 */
{
    uint32_t r = ((a >> 11) * (256u - k) + (b >> 11) * k) >> 8;
    uint32_t g = (((a >> 5) & 63u) * (256u - k) + ((b >> 5) & 63u) * k) >> 8;
    uint32_t l = ((a & 31u) * (256u - k) + (b & 31u) * k) >> 8;
    return (uint16_t)(r << 11 | g << 5 | l);
}

static int32_t rk_ox;                           /* the canvas shows the screen from this x: slices are drawn with the same code as the whole band */
static void rk_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x - rk_ox, y, w, h, c); }

/* labels in a 3x5 pixel font, drawn 2x: the 16 px font is too tall for a row */
#define G35(a, b, c, d, e) ((uint16_t)((a) << 12 | (b) << 9 | (c) << 6 | (d) << 3 | (e)))
static uint16_t glyph35(uint32_t ch)
{
    switch (ch) {
    case 'P': return G35(7, 5, 7, 4, 4);
    case 'U': return G35(5, 5, 5, 5, 7);
    case 'T': return G35(7, 2, 2, 2, 2);
    case 'R': return G35(7, 5, 6, 5, 5);
    case 'I': return G35(7, 2, 2, 2, 7);
    case 'N': return G35(7, 5, 5, 5, 5);
    case 'W': return G35(5, 5, 7, 7, 5);
    case 'A': return G35(2, 5, 7, 5, 5);
    case 'V': return G35(5, 5, 5, 5, 2);
    case '1': return G35(2, 6, 2, 2, 7);
    case '2': return G35(7, 1, 7, 4, 7);
    case '3': return G35(7, 1, 7, 1, 7);
    default: return 0;
    }
}
static void rack_label(int32_t x, int32_t y, const char *s, uint16_t col)
{
    for (; *s; s++, x += 8) {
        uint16_t g = glyph35((uint32_t)*s);
        int32_t r, c;
        for (r = 0; r < 5; r++)
            for (c = 0; c < 3; c++)
                if (g >> (3 * (4 - r)) >> (2 - c) & 1u) rk_rect(x + c * 2, y + r * 2, 2, 2, col);
    }
}
static void rack_block(uint32_t chip, int32_t x, int32_t y, uint16_t c, uint32_t lit)     /* a step, in each chip's style */
{
    if (chip == CHIP_GB) { rk_rect(x + 1, y, 8, 15, c); rk_rect(x, y + 1, 10, 13, c); }                      /* rounded */
    else {
        rk_rect(x, y, 10, 15, c);
        if (lit && chip == CHIP_NES) { uint16_t hi = mix16(c, R_WHITE, 110); rk_rect(x, y, 10, 1, hi); rk_rect(x, y, 1, 15, hi); }     /* bevelled */
        if (lit && chip == CHIP_SMS) rk_rect(x, y + 12, 10, 3, mix16(c, R_BG, 120));                         /* a shaded base */
    }
}
static int32_t rack_x(uint32_t i) { return R_X0 + (int32_t)i * 12 + (int32_t)(i / 4u) * 2; }

/* three rows from x = xa, w wide; rows b*3 .. b*3+2 of what shows; sel = the selected row (a channel, or one past the last: "add"); line = the playhead's x or -1 */
static void rack_band(uint32_t b, uint32_t sel, int32_t xa, int32_t w, int32_t line)
{
    uint32_t c, i;
    cv_begin((uint32_t)w, 3 * R_ROW, R_BG);
    rk_ox = xa;
    for (c = 0; c < 3u; c++) {
        uint32_t row = rack_top + b * 3u + c, on_row = row == sel;
        int32_t y = (int32_t)c * R_ROW;
        if (row < chan_n) {
            uint32_t chip = KIND_CHIP[chan[row].kind];
            const theme_t *t = &THEME[chip];
            uint16_t on = rec_on && !on_row ? mix16(R_BG, t->acc, 110) : t->acc, off = mix16(R_BG, on, 56);     /* recording: the others dim */
            if (on_row) rk_rect(0, y, 240, R_ROW, rack_rm ? RGB(90, 24, 20) : R_SEL);
            rack_label(3, y + 5, KIND_NAME[chan[row].kind], on_row ? on : mix16(R_BG, on, 150));
            for (i = 0; i < SEQ_LEN; i++) {
                const step_t *s = &pat[seq_view][row][i];
                int32_t x = rack_x(i);
                rack_block(chip, x, y + 2, s->note ? on : off, s->note != 0);
                if (on_row && i == rack_cur && (!seq_run || seq_hold)) {   /* the cursor, when the tape is stopped or paused: a white frame round the step the keys write to */
                    uint16_t f = rack_rm ? RGB(235, 64, 52) : R_WHITE;
                    rk_rect(x - 2, y, 14, 2, f); rk_rect(x - 2, y + 17, 14, 2, f);
                    rk_rect(x - 2, y + 2, 2, 15, f); rk_rect(x + 10, y + 2, 2, 15, f);
                }
            }
        } else if (row == chan_n && chan_n < MAXCH) {                   /* the last row: add a channel */
            uint16_t c1 = on_row ? R_WHITE : RGB(110, 110, 125);
            if (on_row) rk_rect(0, y, 240, R_ROW, R_SEL);
            rk_rect(4, y + 7, 14, 3, c1); rk_rect(9, y + 2, 3, 13, c1);
        }
    }
    if (line >= 0) rk_rect(line, 0, 2, 3 * R_ROW, R_WHITE);
    cv_blit((uint32_t)xa, (uint32_t)R_BAND(b));
}
static void rack_cols(uint32_t sel, int32_t xa, int32_t w, int32_t line)    /* every band, over some columns */
{
    uint32_t b;
    for (b = 0; b < 3u; b++) rack_band(b, sel, xa, w, line);
}
static uint32_t fmt_u(char *s, uint32_t v)       /* v in digits, the length */
{
    char t[8];
    uint32_t n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < 7u);
    while (n) s[i++] = t[--n];
    s[i] = 0;
    return i;
}
static int32_t rack_line(void)                  /* the playhead's x: a step is 12 px, a clock tick 2 */
{
    uint32_t st, sub;
    if (!seq_run || seq_pat != seq_view) return -1;       /* the line only shows on the page that is playing */
    do { st = seq_step; sub = seq_sub; } while (st != seq_step);    /* the clock may tick between the two reads */
    return st < 16u ? rack_x(st) + (int32_t)sub * 2 : -1;
}

/* the whole screen when something changed (pattern, selection, play); only two slices between */
static void rack_update(uint32_t sel, uint32_t force)
{
    static uint32_t view_d, run_d, sel_d, cur_d, top_d, n_d, rec_d;
    static int32_t line_d = -1;
    int32_t line;
    if (sel < rack_top) rack_top = sel;
    if (sel >= rack_top + R_ROWS) rack_top = sel - (R_ROWS - 1u);
    line = rack_line();
    if (force || rack_inval || seq_view != view_d || (seq_run | seq_hold << 1) != run_d || sel != sel_d || rack_cur != cur_d || rack_top != top_d || chan_n != n_d || rec_on != rec_d) {
        if (force) { lcd_fill(0, 0, 240, 240, R_BG); hdr_force = 1; }
        rack_inval = 0;
        rack_cols(sel, 0, 240, line);
        if (rack_rm) win_ask("Remove channel?");
        view_d = seq_view; run_d = seq_run | seq_hold << 1; sel_d = sel; cur_d = rack_cur; top_d = rack_top; n_d = chan_n; rec_d = rec_on; line_d = line;
    } else if (line != line_d && !rack_rm) {
        if (line_d >= 0) rack_cols(sel, line_d, 2, -1);
        if (line >= 0) rack_cols(sel, line, 2, line);
        line_d = line;
    }
}
