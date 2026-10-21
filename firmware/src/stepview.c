/* SPDX-License-Identifier: GPL-3.0-only */
/* STEP: one channel's page, a note at a time. SEQ in the rack or chip view opens it; SEQ or HOME closes it. 16 blocks with the note in each,
 * the keyboard below. A key sets the block and moves on; SELECT moves the cursor; EDIT clears the block; PLAY loops the page.
 * Included by chipbox.c after songview.c. */
static uint32_t ed_cur;
static void ed_update(uint32_t c, uint32_t base, uint32_t force)
{
    static uint32_t sig[4];
    const theme_t *t = &THEME[KIND_CHIP[chan[c].kind]];
    uint32_t cur[4], i, h = 0, ps = seq_run && !seq_hold ? seq_step : 99u, cn;
    for (i = 0; i < SEQ_LEN; i++) h = h * 31u + pat[seq_view][c][i].note;
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
        cv_rect(x, y, 28, 40, n ? t->acc : mix16(R_BG, t->acc, 40));
        if (!(i & 3u)) cv_rect(x, y, 28, 3, n ? t->bg : t->acc);                 /* a stripe on each beat */
        if (n) { note_name(s, n); cv_text(x + (28 - text_w(&FONT_S, s)) / 2, y + 14, &FONT_S, s, KIND_CHIP[chan[c].kind] == CHIP_NES ? R_WHITE : t->bg); }
        if (i == ps) cv_rect(x, y + 36, 28, 4, R_WHITE);                         /* the playhead */
        if (i == ed_cur) { cv_rect(x - 1, y - 1, 30, 2, R_WHITE); cv_rect(x - 1, y + 39, 30, 2, R_WHITE); cv_rect(x - 1, y - 1, 2, 42, R_WHITE); cv_rect(x + 27, y - 1, 2, 42, R_WHITE); }
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
