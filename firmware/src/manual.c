/* SPDX-License-Identifier: GPL-3.0-only */
/* The manual: GLO opens the chapter for the view you are on. A page is the FM-1 with the controls to use ringed, a topic and a few lines.
 * SELECT turns pages, PRESETS jumps chapters, GLO again is the contents, HOME leaves. Included by chipbox.c. */
enum {
    M_MASTER = 1u << 0, M_SELECT = 1u << 1, M_PRESETS = 1u << 2, M_ALGO = 1u << 3, M_K1 = 1u << 4, M_K2 = 1u << 5, M_K3 = 1u << 6, M_K4 = 1u << 7,
    M_OCTD = 1u << 8, M_OCTU = 1u << 9, M_FX = 1u << 10, M_SEL = 1u << 11, M_ENV = 1u << 12, M_LFO = 1u << 13, M_EDIT = 1u << 14, M_GLO = 1u << 15,
    M_HOME = 1u << 16, M_SAVE = 1u << 17, M_ARP = 1u << 18, M_SEQ = 1u << 19, M_PLAY = 1u << 20, M_REC = 1u << 21, M_KEYS = 1u << 22,
    M_KNOBS = M_K1 | M_K2 | M_K3 | M_K4,
    M_SMALL = 1u << 24, M_NOPANEL = 1u << 25, M_QR = 1u << 26                   /* the picture small (long text), no picture, the QR code */
};
#include "qr.h"
typedef struct { const char *topic; uint32_t ring; const char *text; } mpage_t;
typedef struct { const char *name; uint16_t col; const mpage_t *page; uint8_t n; } mchap_t;

static const mpage_t MP_QUICK[] = {
    {"", M_GLO | M_SELECT | M_HOME | M_SMALL, "Press GLO at any time to view this manual. Flip through the pages by using the SELECT knob. When you're finished, hit HOME to close."},
    {"", M_HOME | M_SELECT | M_SMALL, "Hold HOME to open the project browser. Use the SELECT knob to pick an existing project, or start a new one. When you're ready, hit HOME to open it!"},
    {"", M_QR, "Find the full manual at"},
    {"", M_GLO | M_HOME | M_SMALL, "To see this quick start every time, or to adjust other settings, press GLO and HOME."},
    {"", M_NOPANEL, "I hope you make some really cool stuff! Have fun!"},
};
static const mpage_t MP_RACK[] = {
    {"Pick a channel", M_PRESETS, "Turn PRESETS to pick a channel. Press a key to write a note on the playhead's step."},
    {"Add a channel", M_PRESETS | M_OCTU, "Turn PRESETS down to the + row. Press OCT+ to add a channel, then turn SELECT to pick a console."},
    {"Remove a channel", M_EDIT | M_OCTU | M_OCTD, "Hold EDIT on a channel to remove it. Press OCT+ for yes or OCT- for no."},
    {"Pages", M_ALGO, "Turn ALGORITHM to flip through your pages. Add new pages in the song."},
    {"Play", M_PLAY | M_K1, "Tap PLAY to play on from this page. Hold PLAY to loop it. Turn KNOB 1 to change the tempo."},
    {"Tape", M_SELECT, "Turn SELECT right to play on and left to play backwards. Leave it in the middle to pause."},
    {"Record", M_REC | M_KEYS, "Press REC, then play the keys as the page loops. Hold EDIT to delete notes as they pass."},
    {"Copy and undo", M_SEL | M_LFO, "Hold SEL on a channel to copy it. Press LFO to undo your last change."},
};
static const mpage_t MP_CHIP[] = {
    {"Knobs", M_KNOBS, "Turn KNOB 1 to 4 to change the sound. Watch the screen to see what each one does."},
    {"More knobs", M_ALGO, "Turn ALGORITHM to reach the next page of knobs: tone, then envelope, then arp."},
    {"Channel", M_PRESETS, "Turn PRESETS to hear another channel's sound. Press HOME to go on to the song."},
    {"Keys", M_KEYS | M_OCTD | M_OCTU, "Press the keys to try the sound. Press OCT- or OCT+ to move up or down an octave."},
};
static const mpage_t MP_SONG[] = {
    {"Slots", M_SELECT, "Turn SELECT to move along the slots. Listen to each page play once as you go."},
    {"Change a page", M_PRESETS, "Turn PRESETS to swap the page in a slot. Listen to it play once."},
    {"Add a slot", M_OCTU | M_OCTD, "Press OCT+ to add a slot with a new blank page. Press OCT- to remove the slot."},
    {"Open a page", M_EDIT, "Tap EDIT to open a page in the rack. Hold EDIT to copy the page."},
    {"Play", M_PLAY | M_K1, "Press PLAY to play the song in order. Turn KNOB 1 to change the tempo."},
};
static const mpage_t MP_STEP[] = {
    {"Step entry", M_SEQ | M_HOME, "Press SEQ to enter notes one at a time. Press HOME to go back to the rack when you are done."},
    {"Notes", M_KEYS | M_SELECT, "Press a key to write a note and move on. Turn SELECT to skip a step and leave a rest."},
    {"Length", M_K2 | M_EDIT, "Turn KNOB 2 to make a note longer. Press EDIT to clear the note under the cursor."},
};
static const mpage_t MP_DRUM[] = {
    {"Pick a drum", M_KEYS, "Press a black key to pick a drum and hear it. Turn PRESETS to pick another channel."},
    {"Place hits", M_KEYS, "Press a white key to place the drum on a step. Press it again to take it away."},
    {"Accents", M_ENV | M_EDIT, "Hold ENV and press a white key to add an accent. Press EDIT to clear this drum."},
};
static const mpage_t MP_FX[] = {
    {"Punch in", M_FX, "Press FX to turn the punch-in layer on. Press FX or HOME to leave it."},
    {"Pads", M_KEYS, "Press a white key to punch in an effect. Hold the key for as long as you want it."},
    {"Tape", M_SELECT, "Hold the TAPE pad and turn SELECT to speed the song up or slow it down."},
};
static const mpage_t MP_PROJ[] = {
    {"Open", M_HOME | M_SELECT, "Hold HOME to open the projects. Turn SELECT to pick one, then press HOME to open it."},
    {"Copy and clear", M_SEL | M_EDIT, "Hold SEL on a project to copy it. Hold EDIT on a project to clear it."},
    {"Save", M_SAVE, "Press SAVE to save your project. Stop editing for two seconds and it saves by itself."},
};
static const mpage_t MP_MENU[] = {
    {"Menu", M_GLO | M_HOME, "Press GLO and HOME together to open the menu. Set what happens at start-up, or Factory Reset."},
    {"Note tails", M_ARP, "Press ARP to turn the note tails in the rack on or off."},
    {"Moving around", M_SELECT | M_PRESETS, "Turn SELECT to turn the pages. Turn PRESETS to jump to another chapter."},
    {"Contents", M_GLO | M_HOME, "Press GLO to see the contents. Press HOME to leave the manual."},
};
#define MC(n, c, p) {n, c, p, sizeof p / sizeof p[0]}
static const mchap_t MCH[] = {
    MC("Quick Start", RGB(120, 210, 120), MP_QUICK),
    MC("Rack", RGB(240, 150, 40), MP_RACK), MC("Chip", RGB(60, 190, 170), MP_CHIP), MC("Song", RGB(150, 110, 235), MP_SONG), MC("Step Entry", RGB(240, 150, 40), MP_STEP),
    MC("Drums", RGB(235, 100, 70), MP_DRUM), MC("FX Punch In", RGB(230, 90, 140), MP_FX), MC("Projects", RGB(120, 170, 230), MP_PROJ), MC("Menu", RGB(160, 160, 175), MP_MENU),
};
#define MCHN (sizeof MCH / sizeof MCH[0])
enum { MC_QUICK, MC_RACK, MC_CHIP, MC_SONG, MC_STEP, MC_DRUM, MC_FX, MC_PROJ, MC_MENU };

/* the FM-1 as the Play online page draws it (1040 x 664, cut off under the buttons): knob and button centres */
static const int16_t MKN[8][2] = {{139, 187}, {253, 185}, {139, 295}, {253, 295}, {665, 185}, {772, 185}, {880, 185}, {988, 185}};
static const int16_t MBT[14][3] = {{147, 380, 84}, {249, 380, 84}, {663, 287, 58}, {729, 287, 58}, {794, 287, 58}, {860, 287, 58}, {925, 287, 58}, {990, 287, 58},
                                   {663, 352, 58}, {729, 352, 58}, {794, 352, 58}, {860, 352, 58}, {925, 352, 58}, {990, 352, 58}};
static int32_t man_k = 13, man_x0 = 16, man_y0 = 34;      /* the picture's scale in 64ths of a unit (13 = 0.2, 7 = small) and where it sits */
#define MPX(v) (man_x0 + (((v) - 30) * man_k) / 64)
#define MPY(v) (man_y0 + (((v) - 82) * man_k) / 64)
#define MZ(n) (((n) * man_k + 6) / 13)
#define M_ACC RGB(240, 170, 50)

static uint32_t man_ch, man_pg, man_toc, man_lock;        /* man_lock: the quick start at power-on: its pages only */
static int32_t man_off;                                 /* the band being drawn */

static void mR(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x, y - man_off, w, h, c); }
static void mT(int32_t x, int32_t y, const char *s, uint16_t c) { cv_text(x, y - man_off, &FONT_S, s, c); }
static void mRing(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    mR(x1, y1, x2 - x1, 2, M_ACC); mR(x1, y2 - 2, x2 - x1, 2, M_ACC); mR(x1, y1, 2, y2 - y1, M_ACC); mR(x2 - 2, y1, 2, y2 - y1, M_ACC);
}
static int32_t mLines(int32_t x, int32_t y, const char *s, int32_t maxw)       /* wrapped on spaces, 17 px a line; returns the next y */
{
    const char *ls = s, *le = s, *p = s;
    char t[64];
    for (;;) {
        const char *we;
        uint32_t n;
        while (*p == ' ') p++;
        we = p;
        while (*we && *we != ' ') we++;
        if (we == p) break;
        n = (uint32_t)(we - ls);
        if (n > 63u) n = 63u;
        memcpy(t, ls, n); t[n] = 0;
        if (le != ls && text_w(&FONT_S, t) > maxw) {
            n = (uint32_t)(le - ls); memcpy(t, ls, n); t[n] = 0;
            mT(x, y, t, RGB(225, 225, 232)); y += 17;
            ls = p;
        }
        le = we; p = we;
    }
    if (le != ls) { uint32_t n = (uint32_t)(le - ls); memcpy(t, ls, n); t[n] = 0; mT(x, y, t, RGB(225, 225, 232)); y += 17; }
    return y;
}
static void mWords(int32_t x, int32_t y, const char *s, int32_t maxw)       /* "\n" starts a new line */
{
    char seg[192];
    for (;;) {
        uint32_t n = 0;
        while (s[n] && s[n] != '\n' && n < 191u) n++;
        memcpy(seg, s, n); seg[n] = 0;
        y = mLines(x, y, seg, maxw);
        if (s[n] != '\n') break;
        s += n + 1u;
    }
}
static void man_panel(uint32_t ring)
{
    uint32_t i;
    mR(MPX(32), MPY(84), MZ(207), MZ(68 + 14), RGB(24, 24, 32));
    mR(MPX(342), MPY(127), MZ(49), MZ(49), RGB(0, 0, 0));
    for (i = 0; i < 8u; i++) { mR(MPX(MKN[i][0]) - MZ(5), MPY(MKN[i][1]) - MZ(5), MZ(11), MZ(11), RGB(14, 14, 18)); mR(MPX(MKN[i][0]) - 1, MPY(MKN[i][1]) - MZ(5), 2, MZ(5), RGB(232, 232, 238)); }
    for (i = 0; i < 14u; i++) mR(MPX(MBT[i][0]) - MZ(MBT[i][2] / 10), MPY(MBT[i][1]) - MZ(4), MZ(MBT[i][2] / 5), MZ(8), RGB(60, 60, 78));
    for (i = 0; i < 16u; i++) mR(MPX(80 + (int32_t)i * 587 / 10), MPY(424) + MZ(4), MZ(9), MZ(7), RGB(74, 74, 90));
    for (i = 0; i < 8u; i++)
        if (ring >> i & 1u) mRing(MPX(MKN[i][0]) - MZ(9), MPY(MKN[i][1]) - MZ(9), MPX(MKN[i][0]) + MZ(10), MPY(MKN[i][1]) + MZ(10));
    for (i = 0; i < 14u; i++)
        if (ring >> (i + 8u) & 1u) mRing(MPX(MBT[i][0]) - MZ(MBT[i][2] / 10 + 4), MPY(MBT[i][1]) - MZ(8), MPX(MBT[i][0]) + MZ(MBT[i][2] / 10 + 4), MPY(MBT[i][1]) + MZ(9));
    if (ring & M_KEYS) mRing(MPX(60), MPY(424) + MZ(1), MPX(1040), MPY(424) + MZ(14));
}
static void man_num(char *d, uint32_t a, uint32_t b)         /* "a/b", both under 100 */
{
    uint32_t n = 0;
    if (a >= 10u) d[n++] = (char)('0' + a / 10u);
    d[n++] = (char)('0' + a % 10u); d[n++] = '/';
    if (b >= 10u) d[n++] = (char)('0' + b / 10u);
    d[n++] = (char)('0' + b % 10u); d[n] = 0;
}
static void man_draw(void)
{
    uint32_t b, i;
    char nb[8];
    const mchap_t *c = &MCH[man_ch];
    const mpage_t *p = &c->page[man_pg];
    if (man_toc) man_num(nb, man_ch + 1u, (uint32_t)MCHN); else man_num(nb, man_pg + 1u, c->n);
    for (b = 0; b < 3u; b++) {
        man_off = (int32_t)b * 80;
        cv_begin(240, 80, R_BG);
        if (man_toc) {
            mR(0, 0, 240, 26, RGB(60, 60, 78));
            mT(8, 4, "Manual", RGB(255, 255, 255));
            for (i = 0; i < MCHN; i++) {
                int32_t y = 30 + (int32_t)i * 22;
                char cn[4];
                if (i == man_ch) mR(0, y, 240, 21, R_SEL);
                mR(8, y + 3, 4, 15, MCH[i].col);
                mT(20, y + 2, MCH[i].name, i == man_ch ? RGB(255, 255, 255) : RGB(180, 180, 195));
                cn[0] = (char)('0' + MCH[i].n / 10u); cn[1] = (char)('0' + MCH[i].n % 10u); cn[2] = 0;
                mT(214, y + 2, MCH[i].n >= 10u ? cn : cn + 1, RGB(110, 110, 125));
            }
        } else {
            mR(0, 0, 240, 26, c->col);
            mT(8, 4, c->name, R_BG);
            mT(232 - text_w(&FONT_S, nb), 4, nb, R_BG);
            if (p->ring & M_QR) {                                           /* the code: dark on light, with its quiet zone */
                uint32_t r, q;
                mWords(8, 32, p->text, 224);
                mR(46, 54, (QR_N + 8) * 4, (QR_N + 8) * 4, RGB(255, 255, 255));
                for (r = 0; r < QR_N; r++) for (q = 0; q < QR_N; q++) if (QR_ROW[r] >> (QR_N - 1u - q) & 1u) mR(46 + 16 + (int32_t)q * 4, 54 + 16 + (int32_t)r * 4, 4, 4, RGB(0, 0, 0));
            } else if (p->ring & M_NOPANEL) mWords(8, 70, p->text, 224);
            else {
                int32_t ty = 142;
                man_k = p->ring & M_SMALL ? 7 : 13; man_x0 = p->ring & M_SMALL ? 64 : 16; man_y0 = p->ring & M_SMALL ? 34 : 34;
                man_panel(p->ring);
                if (p->ring & M_SMALL) ty = 88;
                if (p->topic[0]) { mT(8, 120, p->topic, M_ACC); }
                else if (!(p->ring & M_SMALL)) ty = 120;
                mWords(8, ty, p->text, 224);
                man_k = 13; man_x0 = 16;
            }
            {   int32_t d0 = man_lock ? 120 - (int32_t)c->n * 5 : 224 - (int32_t)c->n * 10;          /* the dots: centred, or at the right with GLO Menu at the left */
                for (i = 0; i < c->n; i++) mR(d0 + (int32_t)i * 10, 226, 6, 4, i <= man_pg ? c->col : RGB(58, 58, 74));
                if (!man_lock) mT(8, 218, "GLO Menu", RGB(138, 138, 154));
                if (man_pg > 0u || (!man_lock && man_ch > 0u)) mT(man_lock ? 4 : d0 - 14, 216, "<", RGB(138, 138, 154));
                if (man_pg + 1u < c->n || (!man_lock && man_ch + 1u < MCHN)) mT(228, 216, ">", RGB(138, 138, 154));
            }
        }
        cv_blit(0, b * 80u);
    }
}
static void man_open(uint32_t ch, uint32_t pg) { man_ch = ch; man_pg = pg; man_toc = 0; man_lock = 0; menu_kind = MK_MANUAL; menu_on = 1; menu_dirty = 1; menu_pick = 0; }
static void man_glo(void) { if (!man_lock) { man_toc ^= 1u; menu_dirty = 1; } }
static void man_input(uint32_t btn)                      /* SELECT turns pages (on into the next chapter), PRESETS jumps chapters, OCT+ opens a chapter from the contents */
{
    int32_t e = fm1_enc_take(ENC_SELECT), f = fm1_enc_take(ENC_PRESETS);
    if (man_lock) {                                         /* the quick start alone: SELECT turns its pages, nothing else */
        if (e > 0 && man_pg + 1u < MCH[man_ch].n) { man_pg++; menu_dirty = 1; }
        else if (e < 0 && man_pg > 0u) { man_pg--; menu_dirty = 1; }
        return;
    }
    if (e && !man_toc) {
        if (e > 0) { if (man_pg + 1u < MCH[man_ch].n) man_pg++; else if (man_ch + 1u < MCHN) { man_ch++; man_pg = 0; } }
        else { if (man_pg > 0u) man_pg--; else if (man_ch > 0u) { man_ch--; man_pg = MCH[man_ch].n - 1u; } }
        menu_dirty = 1;
    } else if (e) f = e;
    if (f) {
        if (f > 0 && man_ch + 1u < MCHN) man_ch++;
        else if (f < 0 && man_ch > 0u) man_ch--;
        man_pg = 0; menu_dirty = 1;
    }
    if (man_toc && (btn & (1u << BTN_OCT_UP))) { man_toc = 0; man_pg = 0; menu_dirty = 1; }
}
