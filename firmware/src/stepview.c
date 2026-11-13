/* SPDX-License-Identifier: GPL-3.0-only */
/* STEP: one channel's page, a note at a time. SEQ in the rack or chip view opens it; SEQ or HOME closes it. 16 blocks with the note in each,
 * the keyboard below. A key sets the block and moves on; SELECT moves the cursor; EDIT clears the block; PLAY loops the page.
 * Included by chipbox.c after songview.c. */
static uint32_t ed_cur, ed_lane;                 /* the cursor's step; the drum picked on a drum channel */
static uint32_t mt_t;                            /* when KNOB 3 / 4 last turned: the meter strip shows for a moment */
#define MT_MS 1500u
static void mt_strip(uint32_t y, const theme_t *t)     /* the meter and the bars, in place of what is below the grid for a moment */
{
    uint32_t q, bars, mb = page_max_bars(seq_view);
    char b[8];
    cv_begin(240, 62, R_BG);
    for (q = 0; q < 6u; q++) {                                                  /* the meters in the knob's order; the page's lit */
        const int32_t x = 2 + (int32_t)q * 39;
        const uint32_t on = METER_UI[q] == page_sig[seq_view];
        cv_rect(x, 0, 37, 26, on ? t->acc : mix16(R_BG, t->acc, 40));
        cv_text(x + (37 - text_w(&FONT_S, METER_NAME[METER_UI[q]])) / 2, 5, &FONT_S, METER_NAME[METER_UI[q]], on ? t->bg : mix16(R_BG, t->acc, 170));
    }
    for (bars = 0; bars < mb; bars++) {                                         /* a box for each bar the page can have; the bars it has are lit */
        const int32_t w = 140 / (int32_t)mb;
        cv_rect(2 + (int32_t)bars * w, 34, w - 2, 22, bars < page_bars(seq_view) ? t->acc : mix16(R_BG, t->acc, 40));
    }
    fmt_u(b, page_steps(seq_view));
    cv_text(150, 36, &FONT_M, b, R_WHITE);
    cv_text(150 + text_w(&FONT_M, b) + 4, 44, &FONT_S, "STEPS", mix16(R_BG, t->acc, 170));
    cv_blit(0, y);
}
static void ed_update(uint32_t c, uint32_t base, uint32_t force)
{
    static uint32_t sig[5];
    const theme_t *t = &THEME[KIND_CHIP[chan[c].kind]];
    const uint32_t plen = page_steps(seq_view), st0 = vwin * SEQ_LEN, ov = mt_t && fm1_ms - mt_t < MT_MS;
    uint32_t cur[5], i, h = 0, ps = seq_run && !seq_hold && seq_pat == seq_view ? seq_step : 999u, cn;
    for (i = 0; i < SEQ_MAXLEN; i++) h = h * 31u + STEP_R(seq_view, c, i)->note + 7u * STEP_R(seq_view, c, i)->gate;
    cur[0] = h; cur[1] = ed_cur | (ps < 99u ? ps : 99u) << 8 | c << 16 | (uint32_t)seq_view << 24; cur[2] = fm1_in.notes; cur[3] = base;
    cur[4] = plen | (uint32_t)page_sig[seq_view] << 8 | vwin << 12 | (ov ? 1u + (uint32_t)page_xb[seq_view] : 0u) << 16;
    if (!force && !memcmp(cur, sig, sizeof cur)) return;
    memcpy(sig, cur, sizeof cur);
    if (force) { view_clear(R_BG); hdr_force = 1; }
    cn = STEP_R(seq_view, c, ed_cur)->note;
    cv_begin(240, 88, R_BG);                                                     /* the 16 blocks of this screen: two rows of eight */
    for (i = 0; i < SEQ_LEN; i++) {
        int32_t x = 1 + (int32_t)(i % 8u) * 30, y = (int32_t)(i / 8u) * 44;
        const uint32_t st = st0 + i, n = STEP_R(seq_view, c, st)->note;
        char s[8];
        if (st >= plen) {                                                        /* past the page's end: dark, crossed out, a kept note a ghost */
            int32_t k;
            cv_rect(x, y, 28, 40, mix16(R_BG, t->acc, 22));
            for (k = 4; k < 36; k += 2) { const int32_t dx = 4 + (k - 4) * 20 / 32; cv_rect(x + dx, y + k, 3, 2, mix16(R_BG, t->acc, 70)); cv_rect(x + 24 - dx, y + k, 3, 2, mix16(R_BG, t->acc, 70)); }
            if (n) { note_name(s, n); cv_text(x + (28 - text_w(&FONT_S, s)) / 2, y + 14, &FONT_S, s, mix16(R_BG, t->acc, 90)); }
            continue;
        }
        cv_rect(x, y, 28, 40, n ? t->acc : mix16(R_BG, t->acc, 56));
        if (METER_GRP[page_sig[seq_view]] >> (st % page_bar_steps(seq_view)) & 1u) cv_rect(x, y, 28, 3, n ? t->bg : t->acc);                 /* a stripe where a beat group starts */
        if (!n) { fmt_u(s, st + 1u); cv_text(x + (28 - text_w(&FONT_S, s)) / 2, y + 14, &FONT_S, s, mix16(R_BG, t->acc, 120)); }       /* an empty block: its number */
        if (n) { note_name(s, n); cv_text(x + (28 - text_w(&FONT_S, s)) / 2, y + 14, &FONT_S, s, KIND_CHIP[chan[c].kind] == CHIP_NES ? R_WHITE : t->bg); }
        if (st == ps) cv_rect(x, y + 36, 28, 4, R_WHITE);                        /* the playhead */
        if (st == ed_cur) { cv_rect(x - 1, y - 1, 30, 2, R_WHITE); cv_rect(x - 1, y + 39, 30, 2, R_WHITE); cv_rect(x - 1, y - 1, 2, 42, R_WHITE); cv_rect(x + 27, y - 1, 2, 42, R_WHITE); }
    }
    for (i = 0; i < SEQ_LEN; i++) {                                              /* long notes last, over the blocks that follow */
        int32_t x = 1 + (int32_t)(i % 8u) * 30, y = (int32_t)(i / 8u) * 44;
        const uint32_t st = st0 + i, n = st < plen ? STEP_R(seq_view, c, st)->note : 0u;
        if (n && STEP_R(seq_view, c, st)->gate > 6u) {                           /* a long note: a line along the bottom, out to where it ends */
            uint32_t e = st + (STEP_R(seq_view, c, st)->gate + 5u) / 6u - 1u;
            if (e > plen - 1u) e = plen - 1u;
            if (e > st0 + 15u) e = st0 + 15u;
            e -= st0;
            if (e / 8u == i / 8u) cv_rect(x, y + 37, 1 + (int32_t)(e % 8u) * 30 + 28 - x, 3, mix16(t->acc, R_WHITE, 110)); else cv_rect(x, y + 37, 239 - x, 3, mix16(t->acc, R_WHITE, 110));
        }
    }
    cv_blit(0, 48);
    if (!(les_on && !les_hide)) {                                                /* (a lesson's card covers the keyboard: painting it under the card made the card flash with every step) */
    cv_begin(240, 62, R_BG);                                                     /* the keyboard: F3 to G5, the cursor's note and the keys held lit */
    {
        int32_t wi = 0, k;
        for (k = 0; k < 27; k++) {                                               /* whites first */
            uint32_t pc = (5u + (uint32_t)k) % 12u;
            if (pc == 1u || pc == 3u || pc == 6u || pc == 8u || pc == 10u) continue;
            {
                uint32_t lit = fm1_in.notes >> k & 1u, mine = cn && cn == base + (uint32_t)k;
                cv_rect(wi * 15, 0, 14, 58, lit ? RGB(70, 150, 255) : mine ? t->acc : RGB(212, 212, 218));
                if (pc == 0u) cv_rect(wi * 15 + 5, 48, 4, 4, RGB(90, 90, 100));  /* a dot on each C */
            }
            wi++;
        }
        wi = 0;
        for (k = 0; k < 27; k++) {                                               /* then the blacks, over the gap after the white before */
            uint32_t pc = (5u + (uint32_t)k) % 12u;
            if (pc == 1u || pc == 3u || pc == 6u || pc == 8u || pc == 10u) {
                uint32_t lit = fm1_in.notes >> k & 1u, mine = cn && cn == base + (uint32_t)k;
                cv_rect(wi * 15 - 5, 0, 10, 36, lit ? RGB(40, 110, 200) : mine ? mix16(t->acc, R_BG, 90) : RGB(30, 30, 38));
            } else wi++;
        }
    }
    cv_blit(0, 148);
    }
    if (ov) mt_strip(148, t);
}

/* DRUMS: a drum channel's page as lanes, a row per drum and a block per step. The black keys pick the drum (the 11: BD SD LT MT HT RS CP CH OH CR RD),
 * the 16 white keys are the steps: the same hit again takes it away, ENV held makes it an accent (brighter, louder). It works while the page plays. */
static void drum_cols(uint32_t c, int32_t xa, int32_t w, int32_t line)     /* both bands over some columns; line = the playhead's x or -1 */
{
    static const char *const LN[11] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD"};
    uint32_t b, i, ln;
    for (b = 0; b < 2u; b++) {                                                  /* two bands: six lanes, then five */
        uint32_t first = b * 6u, n = b ? 5u : 6u;
        cv_begin((uint32_t)w, n * 15u, R_BG);
        rk_ox = xa;
        for (ln = first; ln < first + n; ln++) {
            int32_t y = (int32_t)(ln - first) * 15;
            uint32_t sel = ln == ed_lane;
            if (sel) rk_rect(0, y, 240, 15, R_SEL);
            rack_label(3, y + 2, LN[ln], sel ? DRUM_LC[ln] : mix16(R_BG, DRUM_LC[ln], 150));
            for (i = 0; i < SEQ_LEN; i++) {
                const uint32_t st = vwin * SEQ_LEN + i;
                uint32_t on = dstep_hits(STEP_R(seq_view, c, st)) >> ln & 1u, ac = dstep_acc(STEP_R(seq_view, c, st)) >> ln & 1u;
                if (st >= page_steps(seq_view)) { int32_t k; rk_rect(rack_x(i), y + 1, 10, 13, mix16(sel ? R_SEL : R_BG, DRUM_LC[ln], 20)); for (k = 0; k < 8; k += 2) { rk_rect(rack_x(i) + 1 + k, y + 3 + k, 2, 2, mix16(sel ? R_SEL : R_BG, DRUM_LC[ln], 70)); rk_rect(rack_x(i) + 7 - k, y + 3 + k, 2, 2, mix16(sel ? R_SEL : R_BG, DRUM_LC[ln], 70)); } continue; }
                rk_rect(rack_x(i), y + 1, 10, 13, on ? (ac ? mix16(DRUM_LC[ln], R_WHITE, 140) : DRUM_LC[ln]) : mix16(sel ? R_SEL : R_BG, DRUM_LC[ln], 56));
                if (on && ac) rk_rect(rack_x(i), y + 1, 10, 3, R_WHITE);                       /* an accent: a white cap */
            }
        }
        if (line >= 0) rk_rect(line, 0, 2, (int32_t)(n * 15u), R_WHITE);                        /* the playhead */
        cv_blit((uint32_t)xa, 48u + b * 90u);
    }
}
static void drum_update(uint32_t c, uint32_t force)       /* the whole page when something changed; only two slices while the playhead moves, as in the rack */
{
    static uint32_t sig[3];
    static int32_t line_d = -1;
    uint32_t cur[3], i, h = 0;
    const uint32_t ov = mt_t && fm1_ms - mt_t < MT_MS;
    int32_t line = rack_line();
    for (i = 0; i < SEQ_MAXLEN; i++) h = h * 31u + dstep_hits(STEP_R(seq_view, c, i)) + 7u * dstep_acc(STEP_R(seq_view, c, i)) + 3u;
    cur[0] = h; cur[1] = ed_lane | (seq_run | seq_hold << 1) << 8 | c << 16 | (uint32_t)seq_view << 24; cur[2] = page_steps(seq_view) | (uint32_t)page_sig[seq_view] << 8 | vwin << 12 | (ov ? 1u + (uint32_t)page_xb[seq_view] : 0u) << 16;
    if (force || memcmp(cur, sig, sizeof cur)) {
        memcpy(sig, cur, sizeof cur);
        if (force) { view_clear(R_BG); hdr_force = 1; }
        drum_cols(c, 0, 240, line);
        if (ov) mt_strip(150, &THEME[KIND_CHIP[chan[c].kind]]);
        line_d = line;
    } else if (line != line_d && !ov) {
        if (line_d >= 0) drum_cols(c, line_d, 2, -1);
        if (line >= 0) drum_cols(c, line, 2, line);
        line_d = line;
    }
}
