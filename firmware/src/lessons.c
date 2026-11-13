/* SPDX-License-Identifier: GPL-3.0-only */
/* LESSONS: the menu's first row starts one. A lesson works on a scratch project and puts yours back exactly when it ends: nothing it does is saved. A card over
 * the bottom of the screen gives one instruction at a time, with the control to use in a pill; a step finishes by itself when what it asks for has happened,
 * the card goes green for a moment, and the next step shows. GLO hides the card and brings it back. It never starts by itself.
 * A step is a value to watch (V_*), a test (equal, at least, at most, changed, up, down) and a number. "Changed / up / down" compare with the value when
 * the step began. To add a lesson: an entry in LESSON[] (the steps), a setup in les_setup() if it needs a project, a tutorial page and a line in the guide.
 * Included by chipbox.c before menu.c. */
enum { V_VIEW, V_SEL, V_CHIPPG, V_LASTPG, V_NOTES, V_HITS, V_ACC, V_CUR, V_RUN, V_PAUSED, V_RATE, V_REC, V_SLOTS, V_PAGES, V_PAGE, V_SLOTPAGE, V_TEMPO,
       V_SIG, V_BARS, V_PTMP, V_GL, V_CHAN_N, V_INHASH, V_ENV, V_ARP, V_ARPRATE, V_VIB, V_VOL, V_DUCK, V_LEN, V_ONNOTE, V_MENU, V_FX, V_FXHELD, V_DRUMCH,
       V_ISDRUM, V_LANE, V_WCUS, V_DRCUR, V_FMFOC, V_FMHASH, V_TRKHASH, V_PAGEHASH, V_ONPLUS, V_KIND, V_MENUSEL, V_TAILS, V_KEYS, V_BRUSH, V_SLOTCUR, V_INALL, V_FXLAT, V_FXTOG,
       V_READ, V_FMCH, V_FMOP, V_SID, V_INF, V_DHIT, V_DLANE };      /* V_READ: a page to read (OCT+ goes on); the last ones read a field named by the step's `a` */
enum { O_EQ, O_GE, O_LE, O_CHG, O_UP, O_DOWN };
enum { S_NONE, S_EMPTY, S_MELODY, S_TWOPAGES, S_BAND, S_DRUMS, S_GBWAVE, S_GLIDE, S_MELODY8, S_CHIP, S_MC };
typedef struct { const char *text, *pill; uint8_t v, op; uint16_t thr; uint8_t a; } lstep_t;      /* a read page: text is the page, pill its title, thr the patch to set up when it shows */
typedef struct { const char *name; uint8_t setup, n; const lstep_t *s; uint8_t kd, mc; } lesson_t;      /* kd: the channel kind of a chip lesson; mc: a masterclass (1 FM, 2 SID, 3 waves, 4 drums) */
#define ST(t, p, v, o, n) {t, p, v, o, n, 0}
#define STA(t, p, v, o, n, a) {t, p, v, o, n, a}
#define RD(title, text, patch) {text, title, V_READ, O_EQ, patch, 0}
static const lesson_t LESSON[] = {
    {"Get Around", S_BAND, 5, (const lstep_t[]){ST("Press HOME to open CHIP", "HOME", V_VIEW, O_EQ, 0), ST("Press HOME to open SONG", "HOME", V_VIEW, O_EQ, 2), ST("Press HOME to open RACK", "HOME", V_VIEW, O_EQ, 1),
        ST("Turn PRESETS: next channel", "PRESETS", V_SEL, O_CHG, 0), ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1)}},
    {"First Notes", S_EMPTY, 6, (const lstep_t[]){ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Press a key to add a note", "KEYS", V_NOTES, O_GE, 1), ST("Press 3 more keys", "KEYS", V_NOTES, O_GE, 4),
        ST("Turn SELECT to move cursor", "SELECT", V_CUR, O_CHG, 0), ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1), ST("Press PLAY to stop", "PLAY", V_RUN, O_EQ, 0)}},
    {"Rests", S_EMPTY, 6, (const lstep_t[]){ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Press a key to add a note", "KEYS", V_NOTES, O_GE, 1), ST("Turn SELECT to skip a step", "SELECT", V_CUR, O_UP, 0),
        ST("Press a key after the rest", "KEYS", V_NOTES, O_GE, 2), ST("Press PLAY to hear the rest", "PLAY", V_RUN, O_EQ, 1), ST("Press PLAY to stop", "PLAY", V_RUN, O_EQ, 0)}},
    {"Length and Erase", S_MELODY8, 5, (const lstep_t[]){ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Turn SELECT to move cursor", "SELECT", V_CUR, O_CHG, 0), ST("Turn KNOB 2: longer note", "KNOB 2", V_LEN, O_UP, 0),
        ST("Press EDIT to clear note", "EDIT", V_NOTES, O_DOWN, 0), ST("Press a key to add it back", "KEYS", V_NOTES, O_UP, 0)}},
    {"Play and Tape", S_MELODY, 6, (const lstep_t[]){ST("Hold PLAY to loop the page", "PLAY", V_RUN, O_EQ, 1), ST("Turn SELECT right: faster", "SELECT", V_RATE, O_GE, 11), ST("Turn SELECT left: slower", "SELECT", V_RATE, O_LE, 9),
        ST("Turn SELECT left to pause", "SELECT", V_PAUSED, O_EQ, 1), ST("Turn SELECT right: play on", "SELECT", V_PAUSED, O_EQ, 0), ST("Press PLAY to stop", "PLAY", V_RUN, O_EQ, 0)}},
    {"Record Live", S_EMPTY, 4, (const lstep_t[]){ST("Press REC to record", "REC", V_REC, O_EQ, 1), ST("Press keys to record notes", "KEYS", V_NOTES, O_GE, 2), ST("Press a few more keys", "KEYS", V_NOTES, O_GE, 4), ST("Press REC to stop", "REC", V_REC, O_EQ, 0)}},
    {"Pages", S_TWOPAGES, 5, (const lstep_t[]){ST("Turn ALGORITHM: next page", "ALGORITHM", V_PAGE, O_CHG, 0), ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Press a key to add a note", "KEYS", V_NOTES, O_UP, 0),
        ST("Turn ALGORITHM back a page", "ALGORITHM", V_PAGE, O_CHG, 0), ST("Hold PLAY to loop a page", "PLAY", V_RUN, O_EQ, 1)}},
    {"Make a Song", S_TWOPAGES, 6, (const lstep_t[]){ST("Press HOME to open SONG", "HOME", V_VIEW, O_EQ, 2), ST("Press OCT+ to add a slot", "OCT+", V_SLOTS, O_UP, 0), ST("Turn PRESETS to pick a page", "PRESETS", V_SLOTPAGE, O_CHG, 0),
        ST("Turn SELECT to pick slot 1", "SELECT", V_SLOTCUR, O_CHG, 0), ST("Press PLAY to hear the song", "PLAY", V_RUN, O_EQ, 1), ST("Press PLAY to stop", "PLAY", V_RUN, O_EQ, 0)}},
    {"Copy a Page", S_TWOPAGES, 5, (const lstep_t[]){ST("Press HOME to open SONG", "HOME", V_VIEW, O_EQ, 2), ST("Hold EDIT to copy the page", "EDIT", V_PAGES, O_UP, 0), ST("Tap EDIT to open the copy", "EDIT", V_VIEW, O_EQ, 1),
        ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Press a key to edit a note", "KEYS", V_PAGEHASH, O_CHG, 0)}},
    {"Tempo", S_MELODY, 4, (const lstep_t[]){ST("Turn KNOB 1 for the tempo", "KNOB 1", V_TEMPO, O_CHG, 0), ST("Press PLAY to start", "PLAY", V_RUN, O_EQ, 1), ST("Turn KNOB 1 while it plays", "KNOB 1", V_TEMPO, O_CHG, 0),
        ST("Press PLAY to stop", "PLAY", V_RUN, O_EQ, 0)}},
    {"Change the Sound", S_MELODY, 5, (const lstep_t[]){ST("Press HOME to open CHIP", "HOME", V_VIEW, O_EQ, 0), ST("Turn KNOB 1 to shape sound", "KNOB 1", V_INHASH, O_CHG, 0), ST("Turn KNOB 2 for the volume", "KNOB 2", V_VOL, O_CHG, 0),
        ST("Turn KNOB 3 or 4 as well", "KNOB 3", V_INHASH, O_CHG, 0), ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1)}},
    {"Envelope", S_MELODY, 5, (const lstep_t[]){ST("Press HOME to open CHIP", "HOME", V_VIEW, O_EQ, 0), ST("Turn ALGORITHM: next page", "ALGORITHM", V_CHIPPG, O_EQ, 1), ST("Turn KNOB 1 for the attack", "KNOB 1", V_ENV, O_CHG, 0),
        ST("Turn KNOB 3 for the sustain", "KNOB 3", V_ENV, O_CHG, 0), ST("Turn KNOB 4 for the release", "KNOB 4", V_ENV, O_CHG, 0)}},
    {"Arpeggio", S_MELODY, 5, (const lstep_t[]){ST("Press HOME to open CHIP", "HOME", V_VIEW, O_EQ, 0), ST("Turn ALGORITHM two pages on", "ALGORITHM", V_CHIPPG, O_EQ, 2), ST("Turn KNOB 1 to pick a chord", "KNOB 1", V_ARP, O_GE, 1),
        ST("Turn KNOB 2 for the speed", "KNOB 2", V_ARPRATE, O_CHG, 0), ST("Press a key to hear it", "KEYS", V_KEYS, O_EQ, 1)}},
    {"Vibrato and Sweep", S_MELODY, 4, (const lstep_t[]){ST("Press HOME to open CHIP", "HOME", V_VIEW, O_EQ, 0), ST("Turn KNOB 3 for vibrato", "KNOB 3", V_VIB, O_CHG, 0), ST("Turn KNOB 4 for sweep", "KNOB 4", V_VIB, O_CHG, 0),
        ST("Press a key to hear it", "KEYS", V_KEYS, O_EQ, 1)}},
    {"Add a Channel", S_MELODY, 5, (const lstep_t[]){ST("Turn PRESETS to the + row", "PRESETS", V_ONPLUS, O_EQ, 1), ST("Press OCT+ for the consoles", "OCT+", V_MENU, O_EQ, 2), ST("Turn SELECT to pick console", "SELECT", V_MENUSEL, O_CHG, 0),
        ST("Press OCT+ for its channels", "OCT+", V_MENU, O_EQ, 3), ST("Press OCT+ to add it", "OCT+", V_CHAN_N, O_UP, 0)}},
    {"Add Drums", S_MELODY, 8, (const lstep_t[]){ST("Turn PRESETS to the + row", "PRESETS", V_ONPLUS, O_EQ, 1), ST("Press OCT+ for the consoles", "OCT+", V_MENU, O_EQ, 2), ST("Press OCT+ for its channels", "OCT+", V_MENU, O_EQ, 3),
        ST("Pick Drums, then press OCT+", "OCT+", V_DRUMCH, O_GE, 1), ST("Press SEQ to open the lanes", "SEQ", V_ISDRUM, O_EQ, 1), ST("Press black key: pick lane", "KEYS", V_LANE, O_CHG, 0),
        ST("Press white key: add a hit", "KEYS", V_HITS, O_UP, 0), ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1)}},
    {"Drum Accents", S_DRUMS, 4, (const lstep_t[]){ST("Turn PRESETS to the drums", "PRESETS", V_SEL, O_EQ, 1), ST("Press SEQ to open the lanes", "SEQ", V_VIEW, O_EQ, 3), ST("Hold ENV, press a white key", "ENV", V_ACC, O_UP, 0),
        ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1)}},
#include "lessons_mc_drums.h"
    {"Mixing", S_BAND, 5, (const lstep_t[]){ST("Turn KNOB 2 for the level", "KNOB 2", V_VOL, O_CHG, 0), ST("Turn KNOB 4 for sidechain", "KNOB 4", V_DUCK, O_CHG, 0), ST("Turn KNOB 3 to slide steps", "KNOB 3", V_TRKHASH, O_CHG, 0),
        ST("Turn PRESETS: next channel", "PRESETS", V_SEL, O_CHG, 0), ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1)}},
    {"Punch-in FX", S_BAND, 4, (const lstep_t[]){ST("Press PLAY to start", "PLAY", V_RUN, O_EQ, 1), ST("Press FX to show the pads", "FX", V_FX, O_EQ, 1), ST("Hold a white key for an FX", "KEYS", V_FXHELD, O_GE, 1),
        ST("Press FX to leave", "FX", V_FX, O_EQ, 0)}},
    {"Undo", S_EMPTY, 4, (const lstep_t[]){ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Press a key to add a note", "KEYS", V_NOTES, O_UP, 0), ST("Press another key", "KEYS", V_NOTES, O_UP, 0), ST("Press LFO to undo", "LFO", V_NOTES, O_DOWN, 0)}},
    {"Copy a Channel", S_MELODY, 3, (const lstep_t[]){ST("Hold SEL on the channel", "SEL", V_MENU, O_EQ, 6), ST("Turn SELECT, then OCT+", "OCT+", V_CHAN_N, O_UP, 0), ST("Turn PRESETS to the copy", "PRESETS", V_SEL, O_CHG, 0)}},
    {"New Console", S_MELODY, 4, (const lstep_t[]){ST("Tap EDIT on the channel", "EDIT", V_MENU, O_EQ, 2), ST("Turn SELECT to pick console", "SELECT", V_MENUSEL, O_CHG, 0), ST("Press OCT+ for its channels", "OCT+", V_MENU, O_EQ, 3),
        ST("Press OCT+ to swap the chip", "OCT+", V_KIND, O_CHG, 0)}},
    {"Draw a Wave", S_GBWAVE, 6, (const lstep_t[]){ST("Press HOME to open CHIP", "HOME", V_VIEW, O_EQ, 0), ST("Turn ALGORITHM until BRUSH", "ALGORITHM", V_LASTPG, O_EQ, 1), ST("Turn KNOB 1 to draw a point", "KNOB 1", V_WCUS, O_EQ, 1),
        ST("Turn SELECT to next point", "SELECT", V_DRCUR, O_CHG, 0), ST("Turn KNOB 2 for the brush", "KNOB 2", V_BRUSH, O_CHG, 0), ST("Press a key to hear it", "KEYS", V_KEYS, O_EQ, 1)}},
#include "lessons_mc_waves.h"
    {"Odd Meters", S_EMPTY, 5, (const lstep_t[]){ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Press a key to add a note", "KEYS", V_NOTES, O_GE, 1), ST("Turn KNOB 3 down to 3/4", "KNOB 3", V_SIG, O_EQ, 2),
        ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1), ST("Turn KNOB 3 to 6/8 or 7/8", "KNOB 3", V_SIG, O_GE, 4)}},
    {"Long Pages", S_MELODY, 4, (const lstep_t[]){ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Turn KNOB 4 for more bars", "KNOB 4", V_BARS, O_GE, 2), ST("Turn SELECT past step 16", "SELECT", V_CUR, O_GE, 16),
        ST("Press HOME, then hold PLAY", "HOME", V_RUN, O_EQ, 1)}},
    {"Tempo Changes", S_MELODY, 6, (const lstep_t[]){ST("Press HOME to open SONG", "HOME", V_VIEW, O_EQ, 2), ST("Press OCT+ to add a slot", "OCT+", V_SLOTS, O_GE, 2), ST("Tap EDIT to open that page", "EDIT", V_VIEW, O_EQ, 1),
        ST("Press SEQ to enter notes", "SEQ", V_VIEW, O_EQ, 3), ST("Turn KNOB 1 for page tempo", "KNOB 1", V_PTMP, O_GE, 40), ST("Go to SONG, press PLAY", "PLAY", V_RUN, O_EQ, 1)}},
    {"Glide", S_GLIDE, 4, (const lstep_t[]){ST("Press SEQ on page 1", "SEQ", V_VIEW, O_EQ, 3), ST("Tap SEL for a tempo glide", "SEL", V_GL, O_GE, 1), ST("Tap SEL for another shape", "SEL", V_GL, O_GE, 2),
        ST("Go to SONG, press PLAY", "PLAY", V_RUN, O_EQ, 1)}},
    {"Slide Steps", S_MELODY, 4, (const lstep_t[]){ST("Turn KNOB 3 to slide steps", "KNOB 3", V_TRKHASH, O_CHG, 0), ST("Turn KNOB 3 the other way", "KNOB 3", V_TRKHASH, O_CHG, 0), ST("Press PLAY to hear it", "PLAY", V_RUN, O_EQ, 1),
        ST("Press PLAY to stop", "PLAY", V_RUN, O_EQ, 0)}},
    {"Note Tails", S_BAND, 3, (const lstep_t[]){ST("Press ARP to show tails", "ARP", V_TAILS, O_EQ, 1), ST("Press PLAY to see them", "PLAY", V_RUN, O_EQ, 1), ST("Press ARP to hide tails", "ARP", V_TAILS, O_EQ, 0)}},
    {"FX Latch", S_BAND, 5, (const lstep_t[]){ST("Press PLAY to start", "PLAY", V_RUN, O_EQ, 1), ST("Press FX to show the pads", "FX", V_FX, O_EQ, 1), ST("Tap pad 16 to turn on latch", "KEYS", V_FXLAT, O_EQ, 1),
        ST("Tap a pad to keep it on", "KEYS", V_FXTOG, O_EQ, 1), ST("Tap pad 16 to let go", "KEYS", V_FXLAT, O_EQ, 0)}},
#include "lessons_chips_a.h"
#include "lessons_mc_fm.h"
#include "lessons_chips_b.h"
#include "lessons_mc_sid.h"
    {"Your First Song", S_NONE, 9, (const lstep_t[]){ST("Press OCT+ to add a channel", "OCT+", V_CHAN_N, O_GE, 1), ST("Press SEQ, then add 4 notes", "SEQ", V_NOTES, O_GE, 4),
        ST("Press HOME to open RACK", "HOME", V_VIEW, O_EQ, 1), ST("Add a drums channel", "OCT+", V_DRUMCH, O_GE, 1), ST("Press SEQ, then add 4 hits", "SEQ", V_HITS, O_GE, 4),
        ST("Press HOME to open SONG", "HOME", V_VIEW, O_EQ, 2), ST("Press OCT+ to add a slot", "OCT+", V_SLOTS, O_GE, 2), ST("Hold EDIT to copy the page", "EDIT", V_PAGES, O_GE, 3), ST("Press PLAY to hear it all", "PLAY", V_RUN, O_EQ, 1)}},
};
#define LES_N ((uint32_t)(sizeof LESSON / sizeof LESSON[0]))
static uint8_t les_buf[PROJ_BANK] POOL;         /* the user's project, packed, while a lesson runs */
static uint32_t les_drawn = ~0u;                /* the state the card on the screen shows */
static uint32_t les_len, les_i, les_step, les_ok, les_ok_t, les_fin, les_pick, les_sig = ~0u, les_snap, les_snap_step = ~0u, les_kbase;
static char les_txt[40];
static uint8_t les_done[12];                    /* a bit a lesson: finished (kept in the settings) */
static uint32_t les_rdirty, les_req_page = ~0u, les_req_view = ~0u;                 /* a read page needs painting; a patch wants the CHIP screen on this page (~0: none) */
static uint32_t les_saved, les_top, les_arm = 1;            /* les_saved: a lesson was just finished, the settings are to be saved */
static uint32_t les_ndone(void) { uint32_t i, n = 0; for (i = 0; i < LES_N; i++) n += les_done[i >> 3] >> (i & 7u) & 1u; return n; }
static const char *const LES_PRAISE[9] = {"Nice!", "Well done!", "Way to go!", "Good!", "Got it!", "Yes!", "Sweet!", "Nailed it!", "Easy!"};
#define LES_AMB RGB(255, 190, 60)
#define LES_GRN RGB(80, 220, 110)

static const char *les_row(void)                /* the menu's first row */
{
    uint32_t i = 0;
    const char *p = les_on ? "End Lesson: " : "Lessons: ", *n = LESSON[les_i].name;
    while (*p) les_txt[i++] = *p++;
    if (les_on) { while (*n && i < 38u) les_txt[i++] = *n++; }
    else { i += fmt_u(les_txt + i, les_ndone()); les_txt[i++] = '/'; i += fmt_u(les_txt + i, LES_N); for (n = " done"; *n; n++) les_txt[i++] = *n; }
    les_txt[i] = 0;
    return les_txt;
}
static void les_list_open(void)                 /* the list opens on the first lesson you have not done */
{
    uint32_t i = 0;
    while (i < LES_N && (les_done[i >> 3] >> (i & 7u) & 1u)) i++;
    les_pick = i < LES_N ? i : 0u;
    menu_open(SYSM, 1); menu_kind = MK_LESSONS; menu_pick = 0;
}
static void les_list_move(int32_t d)
{
    int32_t v = (int32_t)les_pick + (d < -8 ? -8 : d > 8 ? 8 : d);                    /* a quick turn goes further: the list is long */
    les_pick = (uint32_t)(v < 0 ? 0 : v >= (int32_t)LES_N ? (int32_t)LES_N - 1 : v);
    menu_dirty = 1;
}
static void les_tick_mark(int32_t x, int32_t y, uint32_t done, uint16_t col)      /* a tick for a lesson you finished, an empty box for one you have not */
{
    if (done) { mR(x, y + 5, 2, 2, col); mR(x + 2, y + 7, 2, 2, col); mR(x + 4, y + 5, 2, 2, col); mR(x + 6, y + 3, 2, 2, col); mR(x + 8, y + 1, 2, 2, col); }
    else { mR(x, y, 11, 1, RGB(90, 90, 110)); mR(x, y + 10, 11, 1, RGB(90, 90, 110)); mR(x, y, 1, 11, RGB(90, 90, 110)); mR(x + 10, y, 1, 11, RGB(90, 90, 110)); }
}
static void les_list_draw(void)
{
    const int32_t w = 232, h = 200, x = 4, y0 = 34, pitch = 21, rows = 7;
    const uint16_t col = M_ACC, grn = LES_GRN;
    char t[24];
    uint32_t b, i, k, top;
    top = les_top;
    if (les_pick < top) top = les_pick; else if (les_pick >= top + (uint32_t)rows) top = les_pick + 1u - (uint32_t)rows;
    les_top = top;
    k = 0; for (const char *p = "Lessons  "; *p; p++) t[k++] = *p;
    k += fmt_u(t + k, les_ndone()); t[k++] = '/'; k += fmt_u(t + k, LES_N); t[k] = 0;
    win_shadow(x, y0, w, h);
    for (b = 0; b * 80u < (uint32_t)h; b++) {
        win_band(b, w, h);
        win_chrome(w, h, t, col);
        for (i = 0; i < (uint32_t)rows && top + i < LES_N; i++) {
            const uint32_t n = top + i, dn = les_done[n >> 3] >> (n & 7u) & 1u;
            const int32_t y = 29 + (int32_t)i * pitch;
            if (n == les_pick) { mR(4, y - 3, w - 8, pitch - 2, R_SEL); mR(4, y - 3, 3, pitch - 2, col); }
            les_tick_mark(14, y + 1, dn, grn);
            mT(32, y, LESSON[n].name, n == les_pick ? R_WHITE : dn ? grn : LESSON[n].mc ? LES_AMB : RGB(200, 200, 210));       /* (a masterclass in gold) */
        }
        {   const int32_t th = rows * pitch - 4, len = th * rows / (int32_t)LES_N, at = (th - len) * (int32_t)top / (int32_t)(LES_N - (uint32_t)rows);      /* a thin scroll bar at the right */
            mR(w - 8, 27, 2, th, RGB(48, 48, 66)); mR(w - 8, 27 + at, 2, len, RGB(150, 150, 175));
        }
        win_foot(w, h, "OCT+  Start");
        cv_blit(x, y0 + b * 80u);
    }
}
/* ---- masterclasses: the sound a chapter starts from. A read page's `thr` names it; it is set when the page shows. */
#define FMO(o, f) (chan[0].fm.op[o][f])
static void fm_op(uint32_t o, uint32_t tl, uint32_t mul, uint32_t ar, uint32_t d1r, uint32_t d1l, uint32_t d2r, uint32_t rr)      /* one operator: level, multiple, the envelope */
{
    FMO(o, OF_TL) = (uint8_t)tl; FMO(o, OF_MUL) = (uint8_t)mul; FMO(o, OF_AR) = (uint8_t)ar; FMO(o, OF_D1R) = (uint8_t)d1r; FMO(o, OF_D1L) = (uint8_t)d1l; FMO(o, OF_D2R) = (uint8_t)d2r; FMO(o, OF_RR) = (uint8_t)rr;
    FMO(o, OF_DT1) = 3; FMO(o, OF_DT2) = 0; FMO(o, OF_KS) = 0; FMO(o, OF_AM) = 0;
}
static void les_patch(uint32_t mc, uint32_t id)
{
    if (chan_n == 0u) return;
    fm_foc = 0; les_req_page = 0;                                                /* the CHIP screen shows the channel's first page */
    if (mc != 4u) les_req_view = 0;                                              /* (the drums work in step entry, from the rack) */
    if (mc == 1u) {                                                              /* FM */
        uint32_t o;
        chan[0].fm.ch[CF_FB] = 0; chan[0].fm.ch[CF_PAN] = 1; chan[0].fm.ch[CF_PMS] = 0; chan[0].fm.ch[CF_AMS] = 0; chan[0].fm.ch[CF_PMD] = 0; chan[0].fm.ch[CF_AMD] = 0;
        for (o = 0; o < 4u; o++) fm_op(o, 127, 1, 31, 0, 0, 0, 6);                // all four silent, held
        switch (id) {
        case 1: chan[0].fm.ch[CF_ALG] = 7; fm_op(0, 0, 1, 31, 0, 0, 0, 6); break;                                         // one operator alone: a sine
        case 2: chan[0].fm.ch[CF_ALG] = 4; fm_op(0, 60, 1, 31, 0, 0, 0, 6); fm_op(1, 0, 1, 31, 0, 0, 0, 6); break;         // a weak modulator on a carrier
        case 3: chan[0].fm.ch[CF_ALG] = 4; fm_op(0, 36, 1, 31, 0, 0, 0, 6); fm_op(1, 0, 1, 31, 0, 0, 0, 6); break;         // a firm one: the ratio is the lesson
        case 4: chan[0].fm.ch[CF_ALG] = 4; fm_op(0, 18, 1, 31, 18, 10, 6, 6); fm_op(1, 0, 1, 31, 0, 0, 0, 8); break;       // the modulator fades: a pluck
        case 5: chan[0].fm.ch[CF_ALG] = 7; fm_op(0, 0, 1, 31, 0, 0, 0, 6); break;                                         // feedback on a lone operator
        case 6: chan[0].fm.ch[CF_ALG] = 0; fm_op(0, 40, 1, 31, 0, 0, 0, 6); fm_op(1, 40, 1, 31, 0, 0, 0, 6); fm_op(2, 40, 2, 31, 0, 0, 0, 6); fm_op(3, 0, 1, 31, 0, 0, 0, 6); break;      // all four at work: the algorithm is the lesson
        case 7: chan[0].fm.ch[CF_ALG] = 7; fm_op(0, 16, 1, 31, 0, 0, 0, 6); fm_op(1, 16, 1, 31, 0, 0, 0, 6); break;       // two carriers: detune
        case 8: chan[0].fm.ch[CF_ALG] = 4; fm_op(0, 28, 1, 31, 0, 0, 0, 6); fm_op(1, 0, 1, 31, 0, 0, 0, 6); break;         // a sustained pair: the LFO
        default: break;                                                          // 9: what you made stays
        }
    } else if (mc == 2u) {                                                       /* SID */
        inst_t *in = &chan[0].in;
        sid_put(in, SP_WAVE, 1); sid_put(in, SP_MODEL, 0); sid_put(in, SP_PW, 8); sid_put(in, SP_PWM, 0); sid_put(in, SP_SYNC, 0); sid_put(in, SP_RING, 0); sid_put(in, SP_CUT, 15); sid_put(in, SP_RES, 0); sid_put(in, SP_MODE, 0); sid_put(in, SP_FENV, 0);
        in->atk = 0; in->dec = 8; in->sus = 12; in->rel = 6;
        switch (id) {
        case 1: sid_put(in, SP_WAVE, 0); break;                                  // triangle, no filter
        case 2: sid_put(in, SP_WAVE, 3); break;                                  // pulse: the width
        case 3: sid_put(in, SP_WAVE, 1); sid_put(in, SP_CUT, 4); sid_put(in, SP_MODE, 1); break;       // saw into a low pass
        case 4: sid_put(in, SP_WAVE, 1); sid_put(in, SP_CUT, 3); sid_put(in, SP_MODE, 1); sid_put(in, SP_RES, 6); in->dec = 6; in->sus = 4; break;      // the filter envelope
        case 5: sid_put(in, SP_WAVE, 1); break;                                  // hard sync
        case 6: sid_put(in, SP_WAVE, 0); break;                                  // ring modulation
        case 7: sid_put(in, SP_WAVE, 1); sid_put(in, SP_CUT, 5); sid_put(in, SP_MODE, 1); sid_put(in, SP_RES, 10); break;      // two chips, one patch
        default: break;
        }
    } else if (mc == 3u) {                                                       /* a wave table */
        inst_t *in = &chan[0].in;
        in->wave = 15; in->wf = 0; in->wc = 0; in->wt = 0; in->atk = 0; in->dec = 8; in->sus = 15; in->rel = 6; in->vol = 15;
        switch (id) {
        case 1: in->wave = 15; break;                                            // a sine
        case 2: in->wave = 15; break;                                            // fold
        case 3: in->wave = 10; break;                                            // a triangle: the steps
        case 4: in->wave = 5; break;                                             // a saw: the twist
        default: break;
        }
    }
}
static void les_enter_step(void)                /* a step became the current one: a read page sets its sound up */
{
    const lstep_t *s = &LESSON[les_i].s[les_step];
    if (s->v == V_READ) { if (s->thr) les_patch(LESSON[les_i].mc, s->thr); les_rdirty = 1; }
}
static void les_note(uint32_t p, uint32_t c, uint32_t s, uint32_t n, uint32_t g)
{
    step_t *x = STEP_W(p, c, s);
    x->note = (uint8_t)n; x->vel = 100; x->gate = (uint8_t)g;
}
static void les_setup(uint32_t i)               /* the scratch project of a lesson */
{
    static const uint8_t MEL[8] = {60, 62, 64, 67, 69, 67, 64, 62};
    const uint32_t kind = LESSON[i].setup;
    uint32_t k, p;
    seq_clear();
    seq_bpm = 120; seq_bpm_play = 120; seq_view = 0;
    if (kind == S_NONE) return;
    if (kind == S_GBWAVE) chan_add(KD_GB_WAV);
    else if (kind == S_CHIP || kind == S_MC) chan_add(LESSON[i].kd);
#ifdef LABEL_TRACE
    else if (getenv("LES_KIND")) chan_add((uint32_t)atoi(getenv("LES_KIND")));
#endif
    else {                                                                       /* the lead: a different chip for each lesson, the NES red is hard to read */
        static const uint8_t LEAD[5] = {KD_SCC, KD_SMS, KD_WSG, KD_VRC6_PU, KD_AY};
        chan_add(LEAD[i % 5u]);
        if (has_duty(0)) chan[0].in.duty = 2;
    }
    if (kind == S_EMPTY || kind == S_MC) return;
    for (p = 0; p < (kind == S_GLIDE ? 2u : 1u); p++)
        for (k = 0; k < 8u; k++) les_note(p, 0, k * (kind == S_MELODY8 ? 1u : 2u), MEL[k] + (p ? 5u : 0u), 11);
    if (kind == S_TWOPAGES || kind == S_GLIDE) { page_n = 2; song_len = 2; song[0] = 0; song[1] = 1; }
    if (kind == S_GLIDE) page_tmp[1] = 84;                                       /* the glide goes down into a slower page */
    if (kind == S_BAND || kind == S_DRUMS) {
        if (kind == S_BAND) { int b = chan_add(KD_PCE); if (b >= 0) for (k = 0; k < 4u; k++) les_note(0, (uint32_t)b, k * 4u, k & 1u ? 43 : 36, 17); }
        {   int d = chan_add(KD_DRUM0);
            if (d >= 0) for (k = 0; k < 16u; k++) dstep_set(STEP_W(0, (uint32_t)d, k), (k % 8u == 0u ? 1u : 0u) | (k % 8u == 4u ? 2u : 0u) | (k % 2u == 0u ? 128u : 0u), 0);
        }
    }
}
static uint32_t les_count(uint32_t what, uint32_t selc)    /* 0 notes on the page on show, 1 drum hits, 2 accents, 3 a hash of one channel's steps, 4 a hash of the page */
{
    uint32_t t, s, n = 0, len = page_steps(seq_view);
    for (t = 0; t < chan_n; t++) {
        const uint32_t dr = IS_DRUM(chan[t].kind) != 0u;
        for (s = 0; s < len; s++) {
            const step_t *x = STEP_R(seq_view, t, s);
            uint32_t h, b;
            if (what == 0u) n += !dr && x->note != 0u;
            else if (what == 1u || what == 2u) { if (dr) { h = what == 1u ? dstep_hits(x) : dstep_acc(x); for (b = 0; b < 11u; b++) n += h >> b & 1u; } }
            else if (what == 4u || (what == 3u && t == selc)) n = n * 31u + (uint32_t)x->note * (s + 3u) + (dr ? dstep_hits(x) * 7u : 0u);
        }
    }
    return n;
}
static uint32_t les_val(uint32_t v, uint32_t a, uint32_t view, uint32_t sel, uint32_t cpg)
{
    const uint32_t ok = sel < chan_n;
    const inst_t *in = &chan[ok ? sel : 0u].in;
    switch (v) {
    case V_VIEW: return view;
    case V_SEL: return sel;
    case V_CHIPPG: return cpg;
    case V_LASTPG: return ok && cpg + 1u == pages(sel);
    case V_NOTES: return les_count(0, sel);
    case V_HITS: return les_count(1, sel);
    case V_ACC: return les_count(2, sel);
    case V_CUR: return ed_cur;
    case V_RUN: return seq_run && !seq_hold;
    case V_PAUSED: return seq_run && seq_hold;
    case V_RATE: return (uint32_t)((int32_t)seq_rate + 8);        /* (shifted to stay positive: 8 paused, 10 normal, 11 faster, 9 half speed) */
    case V_REC: return rec_on;
    case V_SLOTS: return song_len;
    case V_PAGES: return page_n;
    case V_PAGE: return seq_view;
    case V_SLOTPAGE: return song_cur < song_len ? song[song_cur] : 0u;
    case V_TEMPO: return page_bpm(seq_view);
    case V_SIG: return page_sig[seq_view];
    case V_BARS: return page_bars(seq_view);
    case V_PTMP: return page_tmp[seq_view];
    case V_GL: return page_gl[seq_view];
    case V_CHAN_N: return chan_n;
    case V_INHASH: return ok ? (uint32_t)in->wave * 3u + in->duty * 5u + in->pan * 7u + in->wf * 11u + in->wc * 13u + in->wt * 17u + (uint32_t)(in->sweep + 64) * 19u + in->vib_dep * 23u + in->vib_spd * 29u + in->vol * 31u : 0u;
    case V_ENV: return ok ? in->atk * 3u + in->dec * 5u + in->sus * 7u + in->rel * 11u : 0u;
    case V_ARP: return ok ? in->arp : 0u;
    case V_ARPRATE: return ok ? in->arp_rate * 4u + in->arp_dir : 0u;
    case V_VIB: return ok ? in->vib_dep + (uint32_t)(in->sweep + 64) * 17u : 0u;
    case V_VOL: return ok ? in->vol : 0u;
    case V_DUCK: return ok ? in->duck : 0u;
    case V_LEN: return ok ? STEP_R(seq_view, sel, ed_cur)->gate : 0u;
    case V_ONNOTE: return ok && STEP_R(seq_view, sel, ed_cur)->note != 0u;
    case V_MENU: return menu_on ? 1u + menu_kind : 0u;
    case V_MENUSEL: return menu_on ? (menu_kind == MK_ADD ? add_cur : menu_sel) : 99u;       /* (the console list keeps its bar in add_cur) */
    case V_FX: return fx_layer;
    case V_FXHELD: return fx_layer && fx_raw != 0u;
    case V_DRUMCH: { uint32_t t, n = 0; for (t = 0; t < chan_n; t++) n += IS_DRUM(chan[t].kind) != 0u; return n; }
    case V_ISDRUM: return view == 3u && ok && IS_DRUM(chan[sel].kind) != 0u;
    case V_LANE: return ed_lane;
    case V_WCUS: return ok && chan[sel].wcus;
    case V_DRCUR: return dr_cur;
    case V_FMFOC: return fm_foc;
    case V_FMHASH: { uint32_t h = 7u, i; if (ok) for (i = 0; i < sizeof chan[0].fm; i++) h = h * 31u + ((const uint8_t *)&chan[sel].fm)[i]; return h; }
    case V_TRKHASH: return les_count(3, sel);
    case V_PAGEHASH: return les_count(4, sel);
    case V_ONPLUS: return sel >= chan_n;
    case V_KIND: return ok ? chan[sel].kind : 0u;
    case V_TAILS: return rack_tails;
    case V_KEYS: return fm1_in.notes != 0u;
    case V_BRUSH: return dr_brush;
    case V_SLOTCUR: return song_cur;
    case V_INALL: { uint32_t h = 5u, i; if (ok) for (i = 0; i < sizeof chan[0].in; i++) h = h * 31u + ((const uint8_t *)&chan[sel].in)[i]; return h; }
    case V_FXLAT: return fx_lat;
    case V_FXTOG: return fx_tog != 0u;
    case V_FMCH: return ok && IS_FMK(chan[sel].kind) ? chan[sel].fm.ch[a % NCHF] : 0u;                          /* a: the channel field (CF_*) */
    case V_FMOP: return ok && IS_FMK(chan[sel].kind) ? chan[sel].fm.op[(a >> 5) & 3u][(a & 31u) % NOPF] : 0u;   /* a: operator * 32 + the field (OF_*) */
    case V_SID: return ok && chan[sel].kind == KD_SID ? sid_get(&chan[sel].in, a) : 0u;                         /* a: SP_* */
    case V_INF: {                                                                                               /* a: IF_*, an instrument byte */
        const inst_t *in = &chan[sel].in;
        if (!ok) return 0u;
        switch (a) { case 0: return in->wave; case 1: return in->duty; case 2: return in->wf; case 3: return in->wc; case 4: return in->wt; case 5: return in->vol; case 6: return in->pan;
                     case 7: return in->atk; case 8: return in->dec; case 9: return in->sus; case 10: return in->rel; case 11: return in->vib_dep; case 12: return in->vib_spd; default: return 0u; }
    }
    case V_DHIT: { const uint32_t c = ok && IS_DRUM(chan[sel].kind) ? sel : 0u; return IS_DRUM(chan[c].kind) ? dstep_hits(STEP_R(seq_view, c, a >> 4)) >> (a & 15u) & 1u : 0u; }      /* a: step * 16 + lane */
    case V_DLANE: { uint32_t c = ok && IS_DRUM(chan[sel].kind) ? sel : 0u, st, n = 0; if (IS_DRUM(chan[c].kind)) for (st = 0; st < page_steps(seq_view); st++) n += dstep_hits(STEP_R(seq_view, c, st)) >> a & 1u; return n; }       /* hits in lane a */
    }
    return 0;
}
static int les_enter(uint32_t i)                /* 1: the lesson runs on a scratch project (the user's is kept) */
{
    if (!les_on) { les_len = proj_pack(les_buf); if (!les_len) return 0; }
    seq_stop();
    les_setup(i);
    undo_reset();
    les_on = 1; les_i = i; les_step = 0; les_ok = 0; les_fin = 0; les_hide = 0; les_sig = ~0u; les_drawn = ~0u; les_snap_step = ~0u;
    les_enter_step();
    return 1;
}
static void les_exit(void)                      /* the user's project comes back */
{
    seq_stop();
    proj_unpack(les_buf, les_len);
    undo_reset();
    les_on = 0; lcd_wy0 = lcd_wy1 = 0;
}
static uint32_t les_true(const lstep_t *s, uint32_t v) { return (s->op == O_EQ && v == s->thr) || (s->op == O_GE && v >= s->thr) || (s->op == O_LE && v <= s->thr); }
static void les_tick(uint32_t view, uint32_t sel, uint32_t cpg)      /* every loop: has the step been done; a finished one goes green for a moment, then the next */
{
    const lesson_t *l = &LESSON[les_i];
    const lstep_t *s;
    uint32_t v, done = 0;
    if (!les_on || les_fin) return;
    if (les_ok) {
        if (fm1_ms - les_ok_t > 900u) { les_ok = 0; if (++les_step >= l->n) { les_fin = 1; les_done[les_i >> 3] |= (uint8_t)(1u << (les_i & 7u)); les_saved = 1; } else les_enter_step(); }
        return;
    }
    s = &l->s[les_step];
    if (s->v == V_READ) return;                                                     /* a page to read: OCT+ goes on (les_read_next) */
    v = les_val(s->v, s->a, view, sel, cpg);
    if (les_snap_step != les_step) { les_snap_step = les_step; les_snap = v; les_kbase = les_kclk; les_arm = !les_true(s, v);
#ifdef CHIPBOX_HOST
        if (!les_arm) printf("LESSON %u STEP %u INSTANT\n", (unsigned)les_i, (unsigned)les_step);
#endif
    }
    if (!les_arm && !les_true(s, v)) les_arm = 1;                                   /* a step that was already true when it showed waits until it has been undone */
    switch (s->op) {
    case O_EQ: case O_GE: case O_LE: done = les_arm && les_true(s, v); break;
    case O_CHG: done = v != les_snap; break;
    case O_UP: done = v > les_snap; break;
    case O_DOWN: done = v < les_snap; break;
    }
    if (done && s->op >= O_CHG && s->pill[0] == 'K' && s->pill[1] == 'N' && les_kclk - les_kbase < 4u) done = 0;       /* a knob step wants a few clicks of a knob, not one */
    if (done) {
        les_ok = 1; les_ok_t = fm1_ms;
        if (les_step + 1u < l->n) { les_snap = les_val(l->s[les_step + 1u].v, l->s[les_step + 1u].a, view, sel, cpg); les_snap_step = les_step + 1u; les_kbase = les_kclk; les_arm = !les_true(&l->s[les_step + 1u], les_snap);
#ifdef CHIPBOX_HOST
            if (!les_arm && l->s[les_step + 1u].v != V_READ) printf("LESSON %u STEP %u INSTANT\n", (unsigned)les_i, (unsigned)les_step + 1u);
#endif
        }       /* (what you do while the card is green counts for the next step) */
#ifdef CHIPBOX_HOST
        printf("LESSON %u STEP %u OK\n", (unsigned)les_i, (unsigned)les_step);
#endif
    }
}
static uint32_t les_wants_top(uint32_t view)     /* the card sits at the top on the CHIP screen (its knobs are at the bottom) and in the drum lanes when the step is about a low lane (the lanes run down to where the card is) */
{
    const lstep_t *s;
    uint32_t lane;
    if (!les_on) return 0u;
    if (view == 0u) return 1u;
    if (view != 3u || les_fin || LESSON[les_i].mc != 4u) return 0u;
    s = &LESSON[les_i].s[les_step];
    lane = s->v == V_LANE ? s->thr : s->v == V_DHIT ? (uint32_t)(s->a & 15u) : s->v == V_DLANE ? s->a : 0u;
    return lane >= 6u;
}
static uint32_t les_reading(void)               /* a page to read is up (GLO or HOME puts it away: les_hide) */
{
    return les_on && !les_hide && !les_fin && !les_ok && LESSON[les_i].s[les_step].v == V_READ;
}
static void les_read_next(uint32_t view, uint32_t sel, uint32_t cpg)      /* OCT+ on a read page */
{
    const lesson_t *l = &LESSON[les_i];
    les_drawn = ~0u; les_rdirty = 1;
#ifdef CHIPBOX_HOST
    printf("LESSON %u STEP %u OK\n", (unsigned)les_i, (unsigned)les_step);
#endif
    if (++les_step >= l->n) { les_fin = 1; les_done[les_i >> 3] |= (uint8_t)(1u << (les_i & 7u)); les_saved = 1; return; }
    les_enter_step();
    les_snap = les_val(l->s[les_step].v, l->s[les_step].a, view, sel, cpg); les_snap_step = les_step; les_kbase = les_kclk; les_arm = !les_true(&l->s[les_step], les_snap);
}
static void les_read_draw(void)                 /* the page: a window with its title, the text, OCT+ Next */
{
    const lesson_t *l = &LESSON[les_i];
    const lstep_t *s = &l->s[les_step];
    const int32_t w = 232, h = 200, x = 4, y0 = 34;
    char ct[16], ln[34];
    uint32_t b, k = 0;
    k += fmt_u(ct + k, les_step + 1u); ct[k++] = '/'; k += fmt_u(ct + k, l->n); ct[k] = 0;
    win_shadow(x, y0, w, h);
    for (b = 0; b * 80u < (uint32_t)h; b++) {
        const char *p = s->text;
        int32_t y = 28;
        win_band(b, w, h);
        win_chrome(w, h, s->pill, LES_AMB);
        mT(w - 14 - text_w(&FONT_S, ct), 5, ct, R_BG);
        while (*p && y < h - 34) {
            uint32_t n = 0;
            while (*p && *p != '\n' && n < 33u) ln[n++] = *p++;
            ln[n] = 0;
            if (*p == '\n') p++;
            mT(14, y, ln, RGB(226, 226, 236));
            y += 13;
        }
        win_foot(w, h, "OCT+  Next");
        cv_blit(x, y0 + b * 80u);
    }
}
static uint32_t les_top, les_anim_t, les_moving;                                     /* 1: the card sits at the top (the CHIP screen has its knobs at the bottom) */
static uint32_t les_sigcur(void) { return les_i | les_step << 8 | les_ok << 16 | les_fin << 17 | les_hide << 18 | les_top << 19; }
static void les_clip(uint32_t covered, uint32_t strip_on)    /* before the screen paints: while the card shows what it should, the rectangle it keeps is never painted over (that was the flash) */
{
    lcd_wy0 = lcd_wy1 = 0;
    if (!les_on || covered || (strip_on && !les_hide) || les_moving || les_sigcur() != les_drawn) return;
    lcd_wx0 = les_hide ? 126 : 0; lcd_wx1 = les_hide ? 238 : 240;
    lcd_wy0 = les_hide ? (les_top ? 0 : 196) : les_top ? 0 : 148; lcd_wy1 = les_hide ? (les_top ? 18 : 214) : les_top ? 64 : 212;
}
static void les_card(uint32_t covered, uint32_t strip_on)    /* the card: painted when its state changes (the rectangle is off while it paints) */
{
    const lesson_t *l = &LESSON[les_i];
    const lstep_t *s;
    uint32_t sig;
    uint16_t edge;
    lcd_wy0 = lcd_wy1 = 0;
    if (!les_on || covered) { les_drawn = ~0u; return; }
    if (strip_on && !les_hide) { les_drawn = ~0u; return; }                        /* (the meter strip has the place for a moment) */
    if (les_moving) {                                                            /* the card changed places: it waits for the view's title animation to finish (and the screen to settle), then appears whole */
        if (an_kind == AN_VIEW && fm1_ms < an_end) les_anim_t = fm1_ms;
        if (fm1_ms - les_anim_t < 250u) { les_drawn = ~0u; return; }
        les_moving = 0;
    }
    sig = les_sigcur();
    if (sig == les_drawn) return;
    les_drawn = sig;
    s = &l->s[les_step < l->n ? les_step : l->n - 1u];
    if (les_hide) {                                                              /* a small tab: GLO brings the card back */
        char t[24];
        uint32_t k = 0;
        const char *w = "LESSON ";
        while (*w) t[k++] = *w++;
        k += fmt_u(t + k, les_step + (les_fin ? 0u : 1u)); t[k++] = '/'; k += fmt_u(t + k, l->n); t[k] = 0;
        cv_begin(112, 18, R_BG);
        cv_rect(0, 0, 112, 18, LES_AMB); cv_rect(1, 1, 110, 16, RGB(24, 24, 34));
        cv_text(8, 1, &FONT_S, t, LES_AMB);
        cv_blit(126, les_top ? 0 : 196);
        return;
    }
    edge = les_ok || les_fin ? LES_GRN : LES_AMB;
    cv_begin(240, 64, R_BG);
    cv_rect(6, 0, 228, 64, edge); cv_rect(8, 2, 224, 60, RGB(24, 24, 34));
    cv_text(14, 6, &FONT_S, l->name, LES_AMB);
    { char t[16]; uint32_t k = fmt_u(t, les_fin ? l->n : les_step + 1u); t[k++] = ' '; t[k++] = '/'; t[k++] = ' '; k += fmt_u(t + k, l->n); t[k] = 0; cv_text(226 - text_w(&FONT_S, t), 6, &FONT_S, t, RGB(120, 120, 140)); }
    if (les_fin) cv_text(14, 24, &FONT_S, "That's it!", LES_GRN);
    else cv_text(14, 24, &FONT_S, les_ok ? LES_PRAISE[(les_i * 5u + les_step * 3u) % 9u] : s->text, les_ok ? LES_GRN : R_WHITE);
    {   const char *pill = les_fin ? "OCT+" : s->pill;
        const int32_t pw = text_w(&FONT_S, pill) + 14;
        cv_rect(14, 40, pw, 19, LES_AMB); cv_rect(15, 41, pw - 2, 17, RGB(60, 40, 14));
        cv_text(21, 43, &FONT_S, pill, LES_AMB);
    }
    if (les_fin) { cv_text(66, 43, &FONT_S, "Next", LES_GRN); cv_text(226 - text_w(&FONT_S, "OCT-  Done"), 43, &FONT_S, "OCT-  Done", RGB(150, 150, 170)); }
    else cv_text(226 - text_w(&FONT_S, "GLO  hide"), 43, &FONT_S, "GLO  hide", RGB(120, 120, 140));
    cv_blit(0, les_top ? 0 : 148);
}
