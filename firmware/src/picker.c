/* SPDX-License-Identifier: GPL-3.0-only */
/* The project picker (hold HOME): 12 slots, 3 across and 4 down, each a number and a little picture of its first page. The frame is the cursor,
 * the green dot the open project; an empty slot is a dim number. SELECT moves, OCT+ opens (YES), OCT- / GLO / HOME go back,
 * hold EDIT to clear a slot (the frame turns red, OCT+ confirms). Included by chipbox.c after rack.c. */
static uint32_t pick_on, pick_sel, pick_del, pick_dirty, pick_fresh;
static uint16_t pick_rows[PROJ_SLOTS][MAXCH];
static uint8_t pick_chips[PROJ_SLOTS][MAXCH], pick_have[PROJ_SLOTS];
static uint32_t pick_nch[PROJ_SLOTS];

static void pick_scan(void)
{
    uint32_t s;
    for (s = 0; s < PROJ_SLOTS; s++) {
        pick_have[s] = s == proj_cur || proj_exists(s);
        proj_preview(s, pick_rows[s], pick_chips[s], &pick_nch[s]);
    }
}
static void pick_open(void) { pick_scan(); pick_sel = proj_cur; pick_del = 0; pick_on = 1; pick_dirty = 1; pick_fresh = 1; }
static void pick_draw(void)
{
    uint32_t row, col, r, c;
    if (pick_fresh) { lcd_fill(0, 0, 240, 240, R_BG); pick_fresh = 0; }       /* (only when it opens: a clear on every move flashes) */
    for (row = 0; row < 4u; row++) {
        cv_begin(240, 58, R_BG);
        rk_ox = 0;
        for (col = 0; col < 3u; col++) {
            uint32_t s = row * 3u + col;
            int32_t x = 7 + (int32_t)col * 79;
            char num[4];
            fmt_u(num, s);                                      /* 1 to 11; the first tile is the demo */
            rk_rect(x, 3, 70, 52, pick_have[s] ? RGB(30, 30, 40) : RGB(22, 22, 28));
            if (s) cv_text(x + 5, 5, &FONT_S, num, pick_have[s] ? RGB(235, 235, 240) : RGB(90, 90, 100));
            else cv_text(x + 5, 5, &FONT_S, "DEMO", RGB(235, 235, 240));
            if (pick_have[s] && pick_nch[s]) {                          /* a row a channel, a block a step: the rows share the height, 16 channels or 2 */
                int32_t rh = 27 / (int32_t)pick_nch[s], y0;
                if (rh > 6) rh = 6;
                y0 = 25 + (27 - rh * (int32_t)pick_nch[s]) / 2;
                for (r = 0; r < pick_nch[s]; r++) {
                    uint16_t on = THEME[pick_chips[s][r]].acc;
                    for (c = 0; c < 16u; c++)
                        rk_rect(x + 3 + (int32_t)c * 4, y0 + (int32_t)r * rh, 3, rh > 2 ? rh - 1 : 1, pick_rows[s][r] >> c & 1u ? on : mix16(R_BG, on, 46));
                }
            }
            if (s == proj_cur) rk_rect(x + 60, 9, 6, 6, RGB(80, 220, 120));
            if (s == 0u) {                                              /* the demo: a padlock */
                uint16_t lk = RGB(140, 140, 155);
                rk_rect(x + 56, 15, 8, 6, lk); rk_rect(x + 57, 11, 1, 4, lk); rk_rect(x + 62, 11, 1, 4, lk); rk_rect(x + 57, 10, 6, 1, lk);
            }
            if (s == pick_sel) {
                uint16_t f = pick_del ? RGB(235, 64, 52) : R_WHITE;
                rk_rect(x - 3, 0, 76, 3, f); rk_rect(x - 3, 55, 76, 3, f);
                rk_rect(x - 3, 3, 3, 52, f); rk_rect(x + 70, 3, 3, 52, f);
            }
        }
        cv_blit(0, row * 58u + 4u);
    }
    if (pick_del) win_ask("Delete project?");
}
