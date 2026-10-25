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
static uint32_t rack_tails;                     /* ARP: show a line under the long notes (kept in the settings) */
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
    case 'C': return G35(7, 4, 4, 4, 7);
    case 'E': return G35(7, 4, 6, 4, 7);
    case 'D': return G35(6, 5, 5, 5, 6);
    case 'M': return G35(5, 7, 7, 5, 5);
    case 'G': return G35(7, 4, 5, 5, 7);
    case 'H': return G35(5, 5, 7, 5, 5);
    case 'S': return G35(7, 4, 7, 1, 7);
    case '0': return G35(7, 5, 5, 5, 7);
    case '4': return G35(5, 5, 7, 1, 1);
    case '5': return G35(7, 4, 7, 1, 7);
    case '6': return G35(7, 4, 7, 5, 7);
    case '7': return G35(7, 1, 2, 2, 2);
    case '8': return G35(7, 5, 7, 5, 7);
    case '9': return G35(7, 5, 7, 1, 7);
    case 'A': return G35(2, 5, 7, 5, 5);
    case 'V': return G35(5, 5, 5, 5, 2);
    case 'B': return G35(6, 5, 6, 5, 6);
    case 'K': return G35(5, 5, 6, 5, 5);
    case 'L': return G35(4, 4, 4, 4, 7);
    case 'O': return G35(7, 5, 5, 5, 7);
    case 'Y': return G35(5, 5, 2, 2, 2);
    case 'F': return G35(7, 4, 6, 4, 4);
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
static void rack_digit(int32_t x, int32_t y, uint32_t d, int32_t sc, uint16_t col)     /* a number after the name: 2x when there is room, 1x after a long name */
{
    uint16_t g = glyph35((uint32_t)'0' + d);
    int32_t r, c;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 3; c++)
            if (g >> (3 * (4 - r)) >> (2 - c) & 1u) rk_rect(x + c * sc, y + r * sc, sc, sc, col);
}
static void rack_block(uint32_t chip, int32_t x, int32_t y, uint16_t c, uint32_t lit)     /* a step, in each chip's style */
{
    if (chip == CHIP_GB) { rk_rect(x + 1, y, 8, 15, c); rk_rect(x, y + 1, 10, 13, c); }                      /* rounded */
    else if (chip == CHIP_PCE) { rk_rect(x + 2, y, 6, 15, c); rk_rect(x, y + 2, 10, 11, c); }                /* corners cut */
    else if (chip == CHIP_VRC6) { rk_rect(x + 3, y, 4, 15, c); rk_rect(x + 1, y + 3, 8, 9, c); rk_rect(x, y + 5, 10, 5, c); }     /* a gem */
    else if (chip == CHIP_MD) { rk_rect(x, y, 8, 3, c); rk_rect(x, y + 3, 10, 9, c); rk_rect(x + 3, y + 12, 7, 3, c); }          /* slanted, like the 16-bit mark */
    else if (chip == CHIP_VB) { rk_rect(x, y, 10, 15, c); rk_rect(x, y + 7, 10, 1, R_BG); }                   /* two halves */
    else if (chip == CHIP_TIA) { uint32_t r; rk_rect(x, y, 10, 15, c); for (r = 0; r < 3u; r++) rk_rect(x, y + 3 + (int32_t)r * 4, 10, 1, R_BG); }     /* scan lines */
    else if (chip == CHIP_AY) { rk_rect(x, y, 10, 15, c); rk_rect(x, y, 10, 2, lit ? R_WHITE : c); }         /* a bright top */
    else if (chip == CHIP_POKEY) { rk_rect(x, y + 4, 10, 11, c); rk_rect(x, y, 7, 4, c); rk_rect(x + 7, y + 2, 3, 2, c); }                         /* a slanted top */
    else if (chip == CHIP_SPK) rk_rect(x, y, 5, 15, c);                                                       /* half a block: one bit */
    else if (chip == CHIP_OPL) { rk_rect(x + 3, y, 4, 15, c); rk_rect(x + 1, y + 2, 8, 11, c); rk_rect(x, y + 4, 10, 7, c); }      /* a capsule */
    else if (chip == CHIP_WSG) { rk_rect(x, y, 10, 15, c); if (lit) rk_rect(x + 6, y + 5, 4, 5, R_BG); }      /* Pac-Man's mouth */
    else if (chip == CHIP_S5B) { rk_rect(x + 3, y, 4, 15, c); rk_rect(x, y + 5, 10, 5, c); }                 /* a cross */
    else if (chip == CHIP_MMC5) { rk_rect(x, y, 4, 15, c); rk_rect(x + 6, y + 4, 4, 11, c); }                  /* two pulses */
    else if (chip == CHIP_FDS) { rk_rect(x, y, 10, 15, c); rk_rect(x + 3, y, 4, 4, R_BG); }                   /* a disk's shutter */
    else if (chip == CHIP_SCC) { rk_rect(x, y + 10, 4, 5, c); rk_rect(x + 3, y + 5, 4, 10, c); rk_rect(x + 6, y, 4, 15, c); }      /* steps */
    else if (chip == CHIP_OPLL) { rk_rect(x, y, 10, 15, c); rk_rect(x + 3, y + 11, 4, 4, R_BG); }              /* a notch at the foot */
    else if (chip == CHIP_VRC7) { rk_rect(x, y, 10, 15, c); rk_rect(x + 6, y, 4, 4, R_BG); }                  /* a cut corner */
    else if (chip == CHIP_OPM) { rk_rect(x, y, 4, 15, c); rk_rect(x + 6, y, 4, 15, c); }                        /* two towers */
    else if (chip == CHIP_OPL3) { rk_rect(x, y + 3, 10, 9, c); rk_rect(x + 1, y, 2, 3, c); rk_rect(x + 5, y, 2, 3, c); rk_rect(x + 1, y + 12, 2, 3, c); rk_rect(x + 5, y + 12, 2, 3, c); }      /* a chip with legs */
    else if (chip == CHIP_WS) { rk_rect(x, y + 3, 10, 9, c); if (lit) rk_rect(x + 3, y + 5, 4, 5, R_BG); }      /* a handheld's screen */
    else if (chip == CHIP_SAA) { uint32_t r; rk_rect(x, y, 10, 15, c); for (r = 0; r < 4u; r++) rk_rect(x, y + 2 + (int32_t)r * 4, 10, 1, r & 1u ? R_BG : c); rk_rect(x, y + 6, 10, 1, R_BG); rk_rect(x, y + 10, 10, 1, R_BG); }      /* a stripe set */
    else if (chip == CHIP_VIC) { rk_rect(x, y + 4, 10, 11, c); rk_rect(x + 2, y, 6, 4, c); }                  /* a slab with a lid */
    else if (chip == CHIP_TED) { rk_rect(x, y, 10, 15, c); rk_rect(x + 2, y + 11, 6, 2, R_BG); }              /* a bar at the foot */
    else if (chip == CHIP_PC98) { rk_rect(x, y, 10, 15, c); rk_rect(x + 2, y + 2, 6, 5, R_BG); }                 /* a monitor */
    else if (chip == CHIP_NEO) { rk_rect(x, y + 3, 10, 12, c); rk_rect(x + 2, y, 6, 3, c); }                   /* a slab with a lid */
    else if (chip == CHIP_NGP) { rk_rect(x, y + 4, 10, 8, c); rk_rect(x + 2, y + 1, 6, 3, c); rk_rect(x + 2, y + 12, 6, 3, c); }      /* a pocket */
    else if (chip == CHIP_OPL1) { rk_rect(x, y + 5, 10, 10, c); rk_rect(x + 2, y, 2, 5, c); rk_rect(x + 6, y, 2, 5, c); }      /* a card with its contacts */
    else if (chip == CHIP_OPZ) { rk_rect(x, y, 10, 15, c); rk_rect(x + 1, y + 3, 8, 1, R_BG); rk_rect(x + 1, y + 7, 8, 1, R_BG); rk_rect(x + 1, y + 11, 8, 1, R_BG); }      /* three lines */
    else if (chip == CHIP_SN77) { rk_rect(x + 2, y, 6, 15, c); rk_rect(x, y + 2, 2, 2, c); rk_rect(x, y + 6, 2, 2, c); rk_rect(x, y + 10, 2, 2, c); rk_rect(x + 8, y + 2, 2, 2, c); rk_rect(x + 8, y + 6, 2, 2, c); rk_rect(x + 8, y + 10, 2, 2, c); }      /* a chip with legs */
    else if (chip == CHIP_C30) { rk_rect(x, y, 10, 15, c); rk_rect(x, y, 10, 3, lit ? R_WHITE : c); rk_rect(x + 2, y + 5, 6, 5, R_BG); }      /* a cabinet */
    else if (chip == CHIP_MSM) { rk_rect(x, y + 6, 3, 9, c); rk_rect(x + 4, y + 2, 3, 13, c); rk_rect(x + 8, y + 9, 2, 6, c); }      /* drawbars */
    else if (chip == CHIP_ZX) { rk_rect(x, y, 10, 15, c); rk_rect(x + 6, y, 1, 15, R_BG); rk_rect(x + 8, y, 1, 15, R_BG); }      /* a stripe set */
    else if (chip == CHIP_LYNX) { rk_rect(x, y + 2, 10, 11, c); rk_rect(x + 2, y, 6, 2, c); rk_rect(x + 2, y + 13, 6, 2, c); }      /* a rounded handheld */
    else {
        rk_rect(x, y, 10, 15, c);
        if (lit && chip == CHIP_NES) { uint16_t hi = mix16(c, R_WHITE, 110); rk_rect(x, y, 10, 1, hi); rk_rect(x, y, 1, 15, hi); }     /* bevelled */
        if (lit && chip == CHIP_SMS) rk_rect(x, y + 12, 10, 3, mix16(c, R_BG, 120));                         /* a shaded base */
    }
}
static const uint16_t DRUM_LC[11] = {RGB(255, 80, 120), RGB(236, 232, 236), RGB(230, 120, 40), RGB(255, 150, 60), RGB(255, 180, 90), RGB(180, 140, 255), RGB(255, 150, 190), RGB(250, 215, 70), RGB(255, 240, 150), RGB(80, 210, 220), RGB(110, 220, 120)};     /* BD SD LT MT HT RS CP CH OH CR RD */
static const int8_t DOT_X[4] = {0, 3, 5, 8};
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
            uint32_t nth = 0, tot = 0, q;
            uint16_t on = rec_on && !on_row ? mix16(R_BG, t->acc, 110) : t->acc, off;     /* recording: the others dim */
            for (q = 0; q < chan_n; q++)
                if (chan[q].kind == chan[row].kind) { tot++; if (q < row) nth++; }
            if (nth) on = mix16(R_BG, on, 255u - (nth > 3u ? 3u : nth) * 40u);          /* the same kind again: a little darker each time */
            off = mix16(R_BG, on, 56);
            if (on_row) rk_rect(0, y, 240, R_ROW, rack_rm ? RGB(90, 24, 20) : R_SEL);
            rack_label(3, y + 5, KIND_NAME[chan[row].kind], on_row ? on : mix16(R_BG, on, 150));
            if (tot > 1u) {
                uint32_t len = 0;
                while (KIND_NAME[chan[row].kind][len]) len++;
                if (len < 3u) rack_digit(3 + (int32_t)len * 8, y + 5, nth + 1u > 9u ? 9u : nth + 1u, 2, on_row ? on : mix16(R_BG, on, 150));
                else rack_digit(26, y + 10, nth + 1u > 9u ? 9u : nth + 1u, 1, on_row ? on : mix16(R_BG, on, 150));
            }
            for (i = 0; i < SEQ_LEN; i++) {
                const step_t *s = &pat[seq_view][row][i];
                int32_t x = rack_x(i);
                if (IS_DRUM(chan[row].kind)) {                        /* a drum step: a dot a lane, four across, three down; an accent is whiter */
                    uint32_t ln, h = dstep_hits(s), ac = dstep_acc(s);
                    for (ln = 0; ln < 11u; ln++) {
                        uint16_t lc = h >> ln & 1u ? (ac >> ln & 1u ? mix16(DRUM_LC[ln], R_WHITE, 150) : DRUM_LC[ln]) : mix16(on_row ? R_SEL : R_BG, DRUM_LC[ln], 40);
                        rk_rect(x + DOT_X[ln & 3u], y + 2 + (int32_t)(ln >> 2) * 5, 2, 4, lc);
                    }
                } else rack_block(chip, x, y + 2, s->note ? on : off, s->note != 0);
                if (rack_tails && !IS_DRUM(chan[row].kind) && s->note && s->gate > 6u) {            /* a long note: a line under the row, out to the step where it ends */
                    uint32_t e = i + (s->gate + 5u) / 6u - 1u;
                    if (e > 15u) e = 15u;
                    rk_rect(x, y + 17, rack_x(e) + 10 - x, 2, on);
                }
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
