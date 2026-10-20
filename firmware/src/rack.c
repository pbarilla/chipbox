/* SPDX-License-Identifier: GPL-3.0-only */
/* The rack (HOME): every channel's 16 steps at once, a row each, grouped and coloured by chip.
 * A white line is the playhead. It moves in 2 px slices (six a step): only the column it leaves and the
 * one it enters are redrawn. Included by chipbox.c. */
#define R_BG RGB(14, 14, 18)
#define R_SEL RGB(32, 32, 42)
#define R_WHITE RGB(255, 255, 255)
#define R_ROW 21                                /* a row: 17 px of blocks, 2 above and below */
#define R_BAND(b) (28 + (b) * 69)               /* top of a chip's three rows; 6 px between chips */
#define R_X0 30
static uint32_t rack_cur, rack_inval;           /* the cursor's step; set to repaint after an edit */

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
    case 'N': return G35(5, 7, 7, 5, 5);
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
    if (chip == CHIP_GB) { rk_rect(x + 1, y, 8, 17, c); rk_rect(x, y + 1, 10, 15, c); }                      /* rounded */
    else {
        rk_rect(x, y, 10, 17, c);
        if (lit && chip == CHIP_NES) { uint16_t hi = mix16(c, R_WHITE, 110); rk_rect(x, y, 10, 1, hi); rk_rect(x, y, 1, 17, hi); }     /* bevelled */
        if (lit && chip == CHIP_SMS) rk_rect(x, y + 14, 10, 3, mix16(c, R_BG, 120));                         /* a shaded base */
    }
}
static int32_t rack_x(uint32_t i) { return R_X0 + (int32_t)i * 12 + (int32_t)(i / 4u) * 2; }

/* a chip's three rows from x = xa, w wide; line = the playhead's x or -1 */
static void rack_band(uint32_t chip, uint32_t sel_chip, uint32_t sel_ch, int32_t xa, int32_t w, int32_t line)
{
    const theme_t *t = &THEME[chip];
    uint16_t on = t->acc, off = mix16(R_BG, on, 56);
    uint32_t c, i, k;
    cv_begin((uint32_t)w, 3 * R_ROW, R_BG);
    rk_ox = xa;
    for (c = 0; c < CHIP_NCH[chip]; c++) {
        int32_t y = (int32_t)c * R_ROW;
        uint32_t sel = chip == sel_chip && c == sel_ch;
        if (sel) rk_rect(0, y, 240, R_ROW, R_SEL);
        rack_label(3, y + 6, t->ch[c], sel ? on : mix16(R_BG, on, 150));
        for (i = 0; i < 16u; i++) {
            const step_t *s = &pat[seq_view][chip * 4u + c][i];
            int32_t x = rack_x(i);
            rack_block(chip, x, y + 2, s->note ? on : off, s->note != 0);
            if (s->note && (s->arp & 0x80u))                    /* a rising run of three ticks: this step arps */
                for (k = 0; k < 3u; k++) rk_rect(x + 2 + (int32_t)k * 2, y + 15 - (int32_t)k * 2, 2, 2, R_BG);
            if (sel && i == rack_cur) {                         /* the cursor: a white frame round the step the keys write to */
                rk_rect(x - 2, y, 14, 2, R_WHITE); rk_rect(x - 2, y + 19, 14, 2, R_WHITE);
                rk_rect(x - 2, y + 2, 2, 17, R_WHITE); rk_rect(x + 10, y + 2, 2, 17, R_WHITE);
            }
        }
    }
    if (line >= 0) rk_rect(line, 0, 2, 3 * R_ROW, R_WHITE);
    cv_blit((uint32_t)xa, (uint32_t)R_BAND(chip));
}
static void rack_gap(int32_t y, int32_t h, int32_t xa, int32_t w, int32_t line)
{
    lcd_fill((uint32_t)xa, (uint32_t)y, (uint32_t)w, (uint32_t)h, R_BG);
    if (line >= xa && line < xa + w) lcd_fill((uint32_t)line, (uint32_t)y, 2, (uint32_t)h, R_WHITE);
}
static void rack_cols(uint32_t chip, uint32_t ch, int32_t xa, int32_t w, int32_t line)    /* every band and gap, over some columns */
{
    uint32_t k;
    for (k = 0; k < CHIPS; k++) {
        rack_band(k, chip, ch, xa, w, line);
        if (k + 1u < CHIPS) rack_gap(R_BAND((int32_t)k) + 3 * R_ROW, 6, xa, w, line);
    }
}
static void rack_header(uint32_t track)
{
    uint32_t i, note = pat[seq_view][track][rack_cur].note;
    char nm[8];
    cv_begin(240, 24, R_BG);
    rk_ox = 0;
    for (i = 0; i < 11u; i++) rk_rect(8 + (int32_t)i, 5 + (int32_t)i / 2, 1, 12 - (int32_t)i, seq_run ? RGB(80, 220, 120) : RGB(90, 90, 100));
    note_name(nm, note);                        /* the note under the cursor */
    cv_text(30, 4, &FONT_S, nm, RGB(230, 230, 235));
    for (i = 0; i < SEQ_PATS; i++) {            /* the pages, the one you are on lit */
        char s[2] = {(char)('A' + i), 0};
        int32_t x = 150 + (int32_t)i * 22;
        uint32_t cur = i == seq_view;
        cv_rect(x, 2, 20, 20, cur ? RGB(230, 230, 235) : RGB(36, 36, 44));
        cv_text(x + (20 - text_w(&FONT_S, s)) / 2, 4, &FONT_S, s, cur ? R_BG : RGB(140, 140, 150));
    }
    cv_blit(0, 0);
}

static int32_t rack_line(void)                  /* the playhead's x: a step is 12 px, a clock tick 2 */
{
    uint32_t st, sub;
    if (!seq_run || seq_pat != seq_view) return -1;       /* the line only shows on the page that is playing */
    do { st = seq_step; sub = seq_sub; } while (st != seq_step);    /* the clock may tick between the two reads */
    return st < 16u ? rack_x(st) + (int32_t)sub * 2 : -1;
}

/* the whole screen when something changed (pattern, selection, play); only two slices between */
static void rack_update(uint32_t chip, uint32_t ch, uint32_t force)
{
    static uint32_t view_d, run_d, sel_d, cur_d;
    static int32_t line_d = -1;
    uint32_t sel = chip * 4u + ch;
    int32_t line = rack_line();
    if (force || rack_inval || seq_view != view_d || seq_run != run_d || sel != sel_d || rack_cur != cur_d) {
        if (force) lcd_fill(0, 0, 240, 240, R_BG);
        rack_inval = 0;
        rack_header(sel);
        rack_cols(chip, ch, 0, 240, line);
        view_d = seq_view; run_d = seq_run; sel_d = sel; cur_d = rack_cur; line_d = line;
    } else if (line != line_d) {
        if (line_d >= 0) rack_cols(chip, ch, line_d, 2, -1);
        if (line >= 0) rack_cols(chip, ch, line, 2, line);
        line_d = line;
    }
}
