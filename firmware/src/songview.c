/* SPDX-License-Identifier: GPL-3.0-only */
/* The song (SEQ): a grid of slots, each one a page number in the page's colour (1 1 1 1 2 3 1 ...), the frame the cursor, green the slot
 * that plays. SELECT moves, ALGORITHM changes a slot's page, OCT+ adds a slot (a copy), OCT- removes it, hold EDIT copies the page.
 * Included by chipbox.c after header.c. */
#define SG_COLS 8u
#define SG_ROWS 4u
#define SG_Y 48

static void sg_frame(int32_t x, int32_t y, int32_t w, int32_t h, int32_t n, uint16_t c)
{
    cv_rect(x, y, w, n, c); cv_rect(x, y + h - n, w, n, c); cv_rect(x, y + n, n, h - 2 * n, c); cv_rect(x + w - n, y + n, n, h - 2 * n, c);
}
static void sg_band(uint32_t b)                  /* two rows of cells */
{
    uint32_t rr, col, rows = 2u;
    cv_begin(240, rows * 32u, R_BG);
    for (rr = 0; rr < rows; rr++)
        for (col = 0; col < SG_COLS; col++) {
            uint32_t i = (song_top + b * 2u + rr) * SG_COLS + col;
            int32_t x = 4 + (int32_t)col * 29, y = (int32_t)rr * 32 + 2;
            uint32_t cur = i == song_cur, play = seq_run && !seq_loop && i == seq_slot;
            if (i < song_len) {
                uint16_t c = sg_color(song[i]);
                char n[8];
                fmt_u(n, (uint32_t)song[i] + 1u);
                cv_rect(x, y, 27, 28, play ? c : mix16(R_BG, c, 90));
                cv_text(x + (27 - text_w(&FONT_S, n)) / 2, y + 6, &FONT_S, n, play ? R_BG : R_WHITE);
                if (play) sg_frame(x - 2, y - 2, 31, 32, 2, RGB(80, 220, 120));
            } else if (i == song_len && song_len < SONG_MAX) {            /* the next slot: a plus */
                uint16_t c = cur ? R_WHITE : RGB(90, 90, 105);
                cv_rect(x + 7, y + 13, 13, 3, c); cv_rect(x + 12, y + 8, 3, 13, c);
            }
            if (cur) sg_frame(x - 3, y - 3, 33, 34, 2, R_WHITE);
        }
    cv_blit(0, SG_Y + b * 64u);
}
static void sg_update(uint32_t force)
{
    static uint32_t cur_d, len_d, slot_d, run_d, top_d, loop_d;
    uint32_t row = song_cur / SG_COLS;
    if (row < song_top) song_top = row;
    if (row >= song_top + SG_ROWS) song_top = row - SG_ROWS + 1u;
    if (force || rack_inval || song_cur != cur_d || song_len != len_d || seq_slot != slot_d || seq_run != run_d || song_top != top_d || seq_loop != loop_d) {
        uint32_t b;
        if (force) { lcd_fill(0, 0, 240, 240, R_BG); hdr_force = 1; }
        rack_inval = 0;
        for (b = 0; b < 2u; b++) sg_band(b);
        cur_d = song_cur; len_d = song_len; slot_d = seq_slot; run_d = seq_run; top_d = song_top; loop_d = seq_loop;
    }
}
