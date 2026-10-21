/* SPDX-License-Identifier: GPL-3.0-only */
/* Menus: a list of lines, the SELECT knob scrolls, GLO or HOME goes back. The help list for the view you are on is the first. Included by chipbox.c. */
typedef struct { const char *text; uint8_t head; } mline_t;      /* head: a heading, in the chip's colour */
#define MH(s) {s, 1}
#define ML(s) {s, 0}
#define MENU_ROWS 12                            /* 20 px a line */
static const mline_t HELP_CHIP[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Select - Tape Shuttle"), ML("Algorithm - Flip Page"), ML("Knobs 1-4 - Adjust Controls"),
    MH("Buttons"), ML("Keys - Play Notes"), ML("HOME - Next View"), ML("REC - Record"), ML("FX - Punch In"), ML("SEQ - Step Entry"), ML("ARP - Live Arp"), ML("HOME Hold - Projects"), ML("PLAY - Loop Page"), ML("SAVE - Save"), ML("OCT - Octave"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_RACK[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Select - Tape Shuttle"), ML("Algorithm - Flip Page"), ML("Knob 1 - Tempo"),
    MH("Buttons"), ML("Keys - Set Note"), ML("REC EDIT - Delete Notes"), ML("OCT+ on + - Add Channel"), ML("EDIT Hold - Remove Channel"), ML("HOME - Next View"), ML("REC - Record"), ML("FX - Punch In"), ML("SEQ - Step Entry"), ML("ARP - Live Arp"), ML("HOME Hold - Projects"), ML("PLAY - Loop Page"), ML("SAVE - Save"), ML("OCT - Octave"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_STEP[] = {
    MH("Knobs"), ML("Select - Move"), ML("Presets - Change Channel"), ML("Algorithm - Flip Page"),
    MH("Buttons"), ML("Keys - Set Note"), ML("EDIT - Clear Note"), ML("PLAY - Loop"), ML("SEQ - Done"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_FX[] = {
    MH("Knobs"), ML("Select - Tape (Hold TAPE)"),
    MH("Buttons"), ML("Keys - Hold For Effect"), ML("LATCH - Keep Effects On"), ML("FX - Back"), ML("HOME - Back"), ML("PLAY - Play Stop"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_SONG[] = {
    MH("Knobs"), ML("Select - Move Cursor"), ML("Presets - Change Page"), ML("Knob 1 - Tempo"),
    MH("Buttons"), ML("OCT+ - New Page"), ML("OCT- - Remove Page"), ML("EDIT - Open Page"), ML("EDIT Hold - Copy Page"), ML("HOME - Next View"), ML("FX - Punch In"), ML("PLAY - Play Song"), ML("SAVE - Save"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t ADD_MENU[] = {                      /* in the order of the kinds in chips.c */
    MH("Add Channel"), ML("NES Pulse"), ML("NES Triangle"), ML("GB Pulse"), ML("GB Wave"), ML("SMS Tone"),
};
static mline_t SYSM[4] = {MH("Menu"), ML("Restore Demos"), ML("Unlock Demos"), ML("Factory Reset")};       /* GLO + HOME */
static const mline_t CONF_DEMO[] = {MH("Restore Demos?"), ML("OCT+ Yes"), ML("OCT- No")};
static const mline_t CONF_FACTORY[] = {MH("Factory Reset?"), ML("OCT+ Yes"), ML("OCT- No")};
enum { MK_HELP, MK_ADD, MK_MENU, MK_DEMO, MK_FACTORY };
static uint32_t menu_kind;                       /* what the open list is: help, the add-channel list, the menu, a confirm */
static const mline_t *menu_lines;
static uint32_t menu_n, menu_top, menu_on, menu_dirty, menu_pick, menu_sel;     /* menu_pick: a list you choose from (SELECT moves the bar, OCT+ yes) */

static void menu_open(const mline_t *l, uint32_t n) { menu_lines = l; menu_n = n; menu_top = 0; menu_on = 1; menu_dirty = 1; menu_pick = 0; }
static void menu_open_pick(const mline_t *l, uint32_t n) { menu_open(l, n); menu_pick = 1; menu_sel = 1; }
static void menu_scroll(int32_t d)
{
    if (menu_pick) {                            /* the bar moves over the lines that are not headings */
        uint32_t v = menu_sel;
        do { v = (v + (d < 0 ? menu_n - 1u : 1u)) % menu_n; } while (menu_lines[v].head);
        menu_sel = v; menu_dirty = 1;
        return;
    }
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
            if (menu_pick && i == menu_sel) cv_rect(0, (int32_t)r * 20, 240, 20, R_SEL);
            cv_text(8, (int32_t)r * 20 + 2, &FONT_S, menu_lines[i].text, menu_lines[i].head ? accent : RGB(230, 230, 235));
        }
        if (menu_n > MENU_ROWS)                 /* a thin bar at the right edge: where you are in the list */
            cv_rect(237, (int32_t)(240u * menu_top / menu_n) - (int32_t)b * 80, 3, (int32_t)(240u * MENU_ROWS / menu_n), RGB(90, 90, 100));
        cv_blit(0, b * 80u);
    }
}
