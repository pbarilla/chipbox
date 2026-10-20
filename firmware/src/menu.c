/* SPDX-License-Identifier: GPL-3.0-only */
/* Menus: a list of lines, the SELECT knob scrolls, GLO or HOME goes back. The help list for the view you are on is the first. Included by chipbox.c. */
typedef struct { const char *text; uint8_t head; } mline_t;      /* head: a heading, in the chip's colour */
#define MH(s) {s, 1}
#define ML(s) {s, 0}
#define MENU_ROWS 12                            /* 20 px a line */
static const mline_t HELP_CHIP[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Algorithm - Controls Page"), ML("Knobs 1-4 - Adjust Controls"),
    MH("Buttons"), ML("Keys - Play Notes"), ML("HOME - Rack"), ML("HOME Hold - Projects"), ML("PLAY - Play Stop"), ML("SAVE - Save"), ML("OCT - Octave"), ML("GLO - Help"),
};
static const mline_t HELP_RACK[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Select - Move Cursor"), ML("Algorithm - Flip Page"),
    MH("Buttons"), ML("Keys - Set Note"), ML("HOME - Back"), ML("HOME Hold - Projects"), ML("PLAY - Play Stop"), ML("SAVE - Save"), ML("OCT - Octave"), ML("GLO - Help"),
};
static const mline_t *menu_lines;
static uint32_t menu_n, menu_top, menu_on, menu_dirty;

static void menu_open(const mline_t *l, uint32_t n) { menu_lines = l; menu_n = n; menu_top = 0; menu_on = 1; menu_dirty = 1; }
static void menu_scroll(int32_t d)
{
    uint32_t max = menu_n > MENU_ROWS ? menu_n - MENU_ROWS : 0u, top = menu_top;
    if (d > 0 && top < max) top++;
    if (d < 0 && top > 0u) top--;
    if (top != menu_top) { menu_top = top; menu_dirty = 1; }
}
static void menu_draw(uint16_t accent)
{
    uint32_t b, r;
    for (b = 0; b < 3u; b++) {
        cv_begin(240, 80, R_BG);
        for (r = 0; r < 4u; r++) {
            uint32_t i = menu_top + b * 4u + r;
            if (i >= menu_n) break;
            cv_text(8, (int32_t)r * 20 + 2, &FONT_S, menu_lines[i].text, menu_lines[i].head ? accent : RGB(230, 230, 235));
        }
        if (menu_n > MENU_ROWS)                 /* a thin bar at the right edge: where you are in the list */
            cv_rect(237, (int32_t)(240u * menu_top / menu_n) - (int32_t)b * 80, 3, (int32_t)(240u * MENU_ROWS / menu_n), RGB(90, 90, 100));
        cv_blit(0, b * 80u);
    }
}
