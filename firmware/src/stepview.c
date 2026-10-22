/* SPDX-License-Identifier: GPL-3.0-only */
/* STEP: one channel's page, a note at a time. SEQ in the rack or chip view opens it; SEQ or HOME closes it. 16 blocks with the note in each,
 * the keyboard below. A key sets the block and moves on; SELECT moves the cursor; EDIT clears the block; PLAY loops the page.
 * Included by chipbox.c after songview.c. */
static uint32_t ed_cur, ed_lane;                 /* the cursor's step; the drum picked on a drum channel */
static void ed_update(uint32_t c, uint32_t base, uint32_t force)
{
    static uint32_t sig[4];
    const theme_t *t = &THEME[KIND_CHIP[chan[c].kind]];
    uint32_t cur[4], i, h = 0, ps = seq_run && !seq_hold ? seq_step : 99u, cn;
    for (i = 0; i < SEQ_LEN; i++) h = h * 31u + pat[seq_view][c][i].note + 7u * pat[seq_view][c][i].gate;
    cur[0] = h; cur[1] = ed_cur | ps << 8 | c << 16 | (uint32_t)seq_view << 24; cur[2] = fm1_in.notes; cur[3] = base;
    if (!force && !memcmp(cur, sig, sizeof cur)) return;
    memcpy(sig, cur, sizeof cur);
    if (force) { lcd_fill(0, 0, 240, 240, R_BG); hdr_force = 1; }
    cn = pat[seq_view][c][ed_cur].note;
    cv_begin(240, 88, R_BG);                                                     /* the 16 blocks: two rows of eight */
    for (i = 0; i < SEQ_LEN; i++) {
        int32_t x = 1 + (int32_t)(i % 8u) * 30, y = (int32_t)(i / 8u) * 44;
        uint32_t n = pat[seq_view][c][i].note;
        char s[8];
        cv_rect(x, y, 28, 40, n ? t->acc : mix16(R_BG, t->acc, 56));
        if (!(i & 3u)) cv_rect(x, y, 28, 3, n ? t->bg : t->acc);                 /* a stripe on each beat */
        if (!n) { fmt_u(s, i + 1u); cv_text(x + (28 - text_w(&FONT_S, s)) / 2, y + 14, &FONT_S, s, mix16(R_BG, t->acc, 120)); }       /* an empty block: its number */
        if (n) { note_name(s, n); cv_text(x + (28 - text_w(&FONT_S, s)) / 2, y + 14, &FONT_S, s, KIND_CHIP[chan[c].kind] == CHIP_NES ? R_WHITE : t->bg); }
        if (i == ps) cv_rect(x, y + 36, 28, 4, R_WHITE);                         /* the playhead */
        if (i == ed_cur) { cv_rect(x - 1, y - 1, 30, 2, R_WHITE); cv_rect(x - 1, y + 39, 30, 2, R_WHITE); cv_rect(x - 1, y - 1, 2, 42, R_WHITE); cv_rect(x + 27, y - 1, 2, 42, R_WHITE); }
    }
    for (i = 0; i < SEQ_LEN; i++) {                                              /* long notes last, over the blocks that follow */
        int32_t x = 1 + (int32_t)(i % 8u) * 30, y = (int32_t)(i / 8u) * 44;
        uint32_t n = pat[seq_view][c][i].note;
        if (n && pat[seq_view][c][i].gate > 6u) {                                /* a long note: a line along the bottom, out to where it ends */
            uint32_t e = i + (pat[seq_view][c][i].gate + 5u) / 6u - 1u;
            if (e > 15u) e = 15u;
            if (e / 8u == i / 8u) cv_rect(x, y + 37, 1 + (int32_t)(e % 8u) * 30 + 28 - x, 3, mix16(t->acc, R_WHITE, 110)); else cv_rect(x, y + 37, 239 - x, 3, mix16(t->acc, R_WHITE, 110));
        }
    }
    cv_blit(0, 48);
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

/* DRUMS: a drum channel's page as lanes, a row per drum and a block per step. The black keys pick the drum (the 11: BD SD LT MT HT RS CP CH OH CR RD),
 * the 16 white keys are the steps: the same hit again takes it away, ENV held makes it an accent (brighter, louder). It works while the page plays. */
static void drum_update(uint32_t c, uint32_t force)
{
    static const char *const LN[11] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD"};
    static uint32_t sig[3];
    uint32_t cur[3], i, h = 0, b, ln, ps = seq_run && !seq_hold ? seq_step : 99u;
    for (i = 0; i < SEQ_LEN; i++) h = h * 31u + dstep_hits(&pat[seq_view][c][i]) + 7u * dstep_acc(&pat[seq_view][c][i]) + 3u;
    cur[0] = h; cur[1] = ed_lane | ps << 8 | c << 16 | (uint32_t)seq_view << 24; cur[2] = 0;
    if (!force && !memcmp(cur, sig, sizeof cur)) return;
    memcpy(sig, cur, sizeof cur);
    if (force) { lcd_fill(0, 0, 240, 240, R_BG); hdr_force = 1; }
    for (b = 0; b < 2u; b++) {                                                  /* two bands: six lanes, then five */
        uint32_t first = b * 6u, n = b ? 5u : 6u;
        cv_begin(240, n * 15u, R_BG);
        rk_ox = 0;
        for (ln = first; ln < first + n; ln++) {
            int32_t y = (int32_t)(ln - first) * 15;
            uint32_t sel = ln == ed_lane;
            if (sel) rk_rect(0, y, 240, 15, R_SEL);
            rack_label(3, y + 2, LN[ln], sel ? DRUM_LC[ln] : mix16(R_BG, DRUM_LC[ln], 150));
            for (i = 0; i < SEQ_LEN; i++) {
                uint32_t on = dstep_hits(&pat[seq_view][c][i]) >> ln & 1u, ac = dstep_acc(&pat[seq_view][c][i]) >> ln & 1u;
                rk_rect(rack_x(i), y + 1, 10, 13, on ? (ac ? mix16(DRUM_LC[ln], R_WHITE, 140) : DRUM_LC[ln]) : mix16(sel ? R_SEL : R_BG, DRUM_LC[ln], 56));
                if (on && ac) rk_rect(rack_x(i), y + 1, 10, 3, R_WHITE);                       /* an accent: a white cap */
                if (i == ps) rk_rect(rack_x(i) + 4, y, 2, 15, R_WHITE);                          /* the playhead */
            }
        }
        cv_blit(0, 48u + b * 90u);
    }
}
