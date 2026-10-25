/* SPDX-License-Identifier: GPL-3.0-only */
/* Menus: a list of lines, the SELECT knob scrolls, GLO or HOME goes back. The help list for the view you are on is the first. Included by chipbox.c. */
typedef struct { const char *text; uint8_t head; } mline_t;      /* head: a heading, in the chip's colour */
#define MH(s) {s, 1}
#define ML(s) {s, 0}
#define MENU_ROWS 12                            /* 20 px a line */
static const mline_t HELP_CHIP[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Select - Tape Shuttle"), ML("Algorithm - Flip Page"), ML("Knobs 1-4 - Adjust Controls"),
    MH("Buttons"), ML("Keys - Play Notes"), ML("HOME - Next View"), ML("REC - Record"), ML("FX - Punch In"), ML("SEQ - Step Entry"), ML("HOME Hold - Projects"), ML("PLAY - Loop Page"), ML("SAVE - Save"), ML("OCT - Octave"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_RACK[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Select - Tape Shuttle"), ML("Algorithm - Flip Page"), ML("Knob 1 - Tempo"),
    MH("Buttons"), ML("Keys - Set Note"), ML("REC EDIT - Delete Notes"), ML("ARP - Note Tails"), ML("SEL Hold - Copy Channel"), ML("OCT+ on + - Add Channel"), ML("EDIT Hold - Remove Channel"), ML("HOME - Next View"), ML("REC - Record"), ML("FX - Punch In"), ML("SEQ - Step Entry"), ML("HOME Hold - Projects"), ML("PLAY - Play From Page"), ML("PLAY Hold - Loop Page"), ML("SAVE - Save"), ML("OCT - Octave"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_STEP[] = {
    MH("Knobs"), ML("Select - Move"), ML("Presets - Change Channel"), ML("Algorithm - Flip Page"),
    MH("Buttons"), ML("Keys - Set Note"), ML("Knob 2 - Note Length"), ML("EDIT - Clear Note"), ML("PLAY - Loop"), ML("HOME - Done"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_DRUM[] = {
    MH("Knobs"), ML("Presets - Change Channel"), ML("Algorithm - Flip Page"), ML("Knob 1 - Tempo"),
    MH("Buttons"), ML("Black Keys - Pick Drum"), ML("White Keys - Place Hits"), ML("ENV + White Key - Accent"), ML("EDIT - Clear Drum"), ML("PLAY - Loop"), ML("HOME - Done"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_FX[] = {
    MH("Knobs"), ML("Select - Tape (Hold TAPE)"),
    MH("Buttons"), ML("Keys - Hold For Effect"), ML("LATCH - Keep Effects On"), ML("FX - Back"), ML("HOME - Back"), ML("PLAY - Play Stop"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t HELP_SONG[] = {
    MH("Knobs"), ML("Select - Move Cursor"), ML("Presets - Change Page"), ML("Knob 1 - Tempo"),
    MH("Buttons"), ML("OCT+ - New Page"), ML("OCT- - Remove Page"), ML("EDIT - Open Page"), ML("EDIT Hold - Copy Page"), ML("HOME - Next View"), ML("FX - Punch In"), ML("PLAY - Play Song"), ML("SAVE - Save"), ML("LFO - Undo"), ML("GLO - Help"), ML("GLO + HOME - Menu"),
};
static const mline_t ADD_MENU[] = {                      /* the consoles, in the order of the chips in chips.c; a second list holds each one's channels */
    MH("Add Channel"), ML("NES"), ML("Game Boy"), ML("Master System"), ML("PC Engine"), ML("Atari 2600"), ML("Konami VRC6"), ML("AY-3-8910"), ML("Mega Drive"), ML("Virtual Boy"), ML("Atari 8-bit"), ML("PC Speaker"), ML("AdLib"), ML("Namco WSG"), ML("Namco 163"), ML("Sunsoft 5B"), ML("NES MMC5"), ML("Famicom Disk"), ML("Konami SCC"), ML("MSX Music"), ML("Konami VRC7"), ML("Arcade FM"), ML("Sound Blaster"), ML("WonderSwan"), ML("Sam Coupe"), ML("VIC-20"), ML("Plus/4"), ML("Atari Lynx"), ML("PC-98"), ML("Neo Geo"), ML("Neo Geo Pocket"), ML("MSX-Audio"), ML("Yamaha TX81Z"), ML("SN76477"), ML("Namco System 1"), ML("Korg Poly-800"), ML("ZX Spectrum"),
};
static const mline_t ADD_NES[] = {MH("NES"), ML("Pulse"), ML("Triangle"), ML("Drums")};
static const mline_t ADD_GB[] = {MH("Game Boy"), ML("Pulse"), ML("Wave"), ML("Drums")};
static const mline_t ADD_SMS[] = {MH("Master System"), ML("Tone"), ML("Drums")};
static const mline_t ADD_PCE[] = {MH("PC Engine"), ML("Wave"), ML("Drums")};
static const mline_t ADD_TIA[] = {MH("Atari 2600"), ML("Sound"), ML("Drums")};
static const mline_t ADD_VRC6[] = {MH("Konami VRC6"), ML("Pulse"), ML("Saw"), ML("Drums")};
static const mline_t ADD_AY[] = {MH("AY-3-8910"), ML("Tone"), ML("Drums")};
static const mline_t ADD_MD[] = {MH("Mega Drive"), ML("FM"), ML("Drums")};
static const mline_t ADD_VB[] = {MH("Virtual Boy"), ML("Wave"), ML("Drums")};
static const mline_t ADD_POKEY[] = {MH("Atari 8-bit"), ML("Sound"), ML("Drums")};
static const mline_t ADD_SPK[] = {MH("PC Speaker"), ML("Beeper"), ML("Drums")};
static const mline_t ADD_OPL[] = {MH("AdLib"), ML("FM"), ML("Drums")};
static const mline_t ADD_WSG[] = {MH("Namco WSG"), ML("Wave"), ML("Drums")};
static const mline_t ADD_N163[] = {MH("Namco 163"), ML("Wave"), ML("Drums")};
static const mline_t ADD_S5B[] = {MH("Sunsoft 5B"), ML("Tone"), ML("Drums")};
static const mline_t ADD_MMC5[] = {MH("NES MMC5"), ML("Pulse"), ML("Drums")};
static const mline_t ADD_FDS[] = {MH("Famicom Disk"), ML("Wave"), ML("Drums")};
static const mline_t ADD_SCC[] = {MH("Konami SCC"), ML("Wave"), ML("Drums")};
static const mline_t ADD_OPLL[] = {MH("MSX Music"), ML("FM"), ML("Drums")};
static const mline_t ADD_VRC7[] = {MH("Konami VRC7"), ML("FM"), ML("Drums")};
static const mline_t ADD_OPM[] = {MH("Arcade FM"), ML("FM"), ML("Drums")};
static const mline_t ADD_OPL3[] = {MH("Sound Blaster"), ML("FM"), ML("Drums")};
static const mline_t ADD_WS[] = {MH("WonderSwan"), ML("Wave"), ML("Drums")};
static const mline_t ADD_SAA[] = {MH("Sam Coupe"), ML("Tone"), ML("Drums")};
static const mline_t ADD_VIC[] = {MH("VIC-20"), ML("Tone"), ML("Drums")};
static const mline_t ADD_TED[] = {MH("Plus/4"), ML("Tone"), ML("Drums")};
static const mline_t ADD_LYNX[] = {MH("Atari Lynx"), ML("Sound"), ML("Drums")};
static const mline_t ADD_PC98[] = {MH("PC-98"), ML("FM"), ML("SSG"), ML("Drums")};
static const mline_t ADD_NEO[] = {MH("Neo Geo"), ML("FM"), ML("SSG"), ML("Drums")};
static const mline_t ADD_NGP[] = {MH("Neo Geo Pocket"), ML("Tone"), ML("Drums")};
static const mline_t ADD_OPL1[] = {MH("MSX-Audio"), ML("FM"), ML("Drums")};
static const mline_t ADD_OPZ[] = {MH("Yamaha TX81Z"), ML("FM"), ML("Drums")};
static const mline_t ADD_SN77[] = {MH("SN76477"), ML("Sound"), ML("Drums")};
static const mline_t ADD_C30[] = {MH("Namco System 1"), ML("Wave"), ML("Drums")};
static const mline_t ADD_MSM[] = {MH("Korg Poly-800"), ML("Organ"), ML("Drums")};
static const mline_t ADD_ZX[] = {MH("ZX Spectrum"), ML("Beeper"), ML("Drums")};
static const struct { const mline_t *l; uint8_t n, kind[3]; } ADD_SUB[CHIPS] = {
    {ADD_NES, 4, {KD_NES_PU, KD_NES_TRI, KD_DRUM0}}, {ADD_GB, 4, {KD_GB_PU, KD_GB_WAV, KD_DRUM0 + 1}}, {ADD_SMS, 3, {KD_SMS, KD_DRUM0 + 2, 0}},
    {ADD_PCE, 3, {KD_PCE, KD_DRUM0 + 3, 0}}, {ADD_TIA, 3, {KD_TIA, KD_DRUM0 + 4, 0}}, {ADD_VRC6, 4, {KD_VRC6_PU, KD_VRC6_SAW, KD_DRUM0 + 5}},
    {ADD_AY, 3, {KD_AY, KD_DRUM0 + 6, 0}}, {ADD_MD, 3, {KD_MD, KD_DRUM0 + 7, 0}}, {ADD_VB, 3, {KD_VB_WAV, KD_DRUM0 + 8, 0}},
    {ADD_POKEY, 3, {KD_POKEY, KD_DRUMB, 0}}, {ADD_SPK, 3, {KD_SPK, KD_DRUMB + 1, 0}}, {ADD_OPL, 3, {KD_OPL, KD_DRUMB + 2, 0}},
    {ADD_WSG, 3, {KD_WSG, KD_DRUMB + 3, 0}}, {ADD_N163, 3, {KD_N163, KD_DRUMB + 4, 0}}, {ADD_S5B, 3, {KD_S5B, KD_DRUMB + 5, 0}},
    {ADD_MMC5, 3, {KD_MMC5, KD_DRUMC, 0}}, {ADD_FDS, 3, {KD_FDS, KD_DRUMC + 1, 0}}, {ADD_SCC, 3, {KD_SCC, KD_DRUMC + 2, 0}}, {ADD_OPLL, 3, {KD_OPLL, KD_DRUMC + 3, 0}},
    {ADD_VRC7, 3, {KD_VRC7, KD_DRUMD, 0}}, {ADD_OPM, 3, {KD_OPM, KD_DRUMD + 1, 0}}, {ADD_OPL3, 3, {KD_OPL3, KD_DRUMD + 2, 0}}, {ADD_WS, 3, {KD_WS, KD_DRUMD + 3, 0}},
    {ADD_SAA, 3, {KD_SAA, KD_DRUME, 0}}, {ADD_VIC, 3, {KD_VIC, KD_DRUME + 1, 0}}, {ADD_TED, 3, {KD_TED, KD_DRUME + 2, 0}}, {ADD_LYNX, 3, {KD_LYNX, KD_DRUME + 3, 0}},
    {ADD_PC98, 4, {KD_PC98_FM, KD_PC98_SSG, KD_DRUMF}}, {ADD_NEO, 4, {KD_NEO_FM, KD_NEO_SSG, KD_DRUMF + 1}}, {ADD_NGP, 3, {KD_NGP, KD_DRUMF + 2, 0}}, {ADD_OPL1, 3, {KD_OPL1, KD_DRUMF + 3, 0}},
    {ADD_OPZ, 3, {KD_OPZ, KD_DRUMF + 4, 0}}, {ADD_SN77, 3, {KD_SN77, KD_DRUMF + 5, 0}}, {ADD_C30, 3, {KD_C30, KD_DRUMF + 6, 0}},
    {ADD_MSM, 3, {KD_MSM, KD_DRUMG, 0}}, {ADD_ZX, 3, {KD_ZX, KD_DRUMG + 1, 0}},
};
static uint32_t add_console;
static const mline_t SYSM[] = {MH("Menu"), ML("Factory Reset")};       /* GLO + HOME */
static const mline_t CONF_FACTORY[] = {MH("Factory Reset?"), ML("OCT+ Yes"), ML("OCT- No")};
static const mline_t COPY_MENU[] = {MH("Copy Channel"), ML("Patch Only"), ML("Patch And Notes"), ML("Cancel")};
enum { MK_HELP, MK_ADD, MK_ADD2, MK_MENU, MK_FACTORY, MK_COPY };
static uint32_t menu_kind;                       /* what the open list is: help, the add-channel list, the menu, a confirm */
static const mline_t *menu_lines;
static uint32_t menu_n, menu_top, menu_on, menu_dirty, menu_pick, menu_sel;     /* menu_pick: a list you choose from (SELECT moves the bar, OCT+ yes) */

static void menu_open(const mline_t *l, uint32_t n) { menu_lines = l; menu_n = n; menu_top = 0; menu_on = 1; menu_dirty = 1; menu_pick = 0; }
static void menu_open_pick(const mline_t *l, uint32_t n) { menu_open(l, n); menu_pick = 1; menu_sel = 1; }
static void menu_scroll(int32_t d)
{
    if (menu_pick) {                            /* the bar moves over the lines that are not headings */
        uint32_t v = menu_sel;
        while (d < 0 ? v > 0u : v + 1u < menu_n) { v = d < 0 ? v - 1u : v + 1u; if (!menu_lines[v].head) break; }          /* no wrap */
        if (menu_lines[v].head) v = menu_sel;
        menu_sel = v; menu_dirty = 1;
        if (v < menu_top + 1u) menu_top = v > 1u ? v : 0u;                                  /* the list follows the bar (the heading shows at the top) */
        else if (v >= menu_top + MENU_ROWS) menu_top = v + 1u - MENU_ROWS;
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
            if (menu_kind == MK_ADD && i >= 1u && i <= CHIPS) {      /* the consoles: each chip's logo at the right */
                static const uint8_t LN[CHIPS] = {4, 3, 7, 7, 3, 4, 6, 6, 3, 5, 4, 6, 3, 4, 2, 4, 6, 3, 6, 4, 6, 6, 5, 7, 4, 4, 5, 6, 6, 5, 5, 6, 7, 3, 7, 3};
                uint32_t ch = i - 1u;
                ico_logo(ch, 240 - 14 - ((int32_t)LN[ch] * 8 + 8), (int32_t)r * 20 + 3);
            }
        }
        if (menu_n > MENU_ROWS)                 /* a thin bar at the right edge: where you are in the list */
            cv_rect(237, (int32_t)(240u * menu_top / menu_n) - (int32_t)b * 80, 3, (int32_t)(240u * MENU_ROWS / menu_n), RGB(90, 90, 100));
        cv_blit(0, b * 80u);
    }
}
