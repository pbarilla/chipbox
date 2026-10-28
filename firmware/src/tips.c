/* SPDX-License-Identifier: GPL-3.0-only */
/* Tips: when nobody has touched the FM-1 for a while and nothing plays, a window shows the next tip from the list, a few seconds each. Any touch closes it.
 * Included by chipbox.c after manual.c (it uses the manual's drawing helpers and the QR codes). */
typedef struct { const char *text; int8_t qr; } tip_t;       /* qr: an index into QR_ROW, or -1 */
static const tip_t TIPS[] = {
    {"Press GLO at any time to open the manual.", -1},
    {"Press GLO and HOME together to open the system menu.", -1},
    {"Turn these tips off at any time in the system menu: press GLO and HOME together.", -1},
    {"Read the manual:", QR_GUIDE},
    {"While your song plays, press FX to punch in effects. Hold a white key for as long as you want one.", -1},
    {"Share your projects:", QR_PROJ},
    {"Report a bug here:", QR_GIT},
    {"Hold HOME to open the project browser.", -1},
    {"Press LFO to undo your last change.", -1},
    {"Hold SEL on a channel to copy it, with its notes or without.", -1},
    {"Press SEQ to enter notes one at a time.", -1},
    {"Press REC and play along while the page loops to record live.", -1},
    {"Turn KNOB 1 to change the tempo.", -1},
    {"Turn SELECT left to play backwards and right to play on. Leave it in the middle to pause.", -1},
    {"Press ARP in the rack to show note tails.", -1},
    {"Hold EDIT on a channel in the rack to remove it.", -1},
    {"Press OCT+ on the + row to add a channel. Turn SELECT to pick a console.", -1},
    {"Turn ALGORITHM in the rack to flip through your pages.", -1},
    {"Press HOME until you reach the song, then press OCT+ to add a new blank page.", -1},
    {"Tap PLAY to play on from this page. Hold PLAY to loop it.", -1},
    {"Turn the four knobs on the chip screen to shape your sound.", -1},
    {"Turn ALGORITHM on the chip screen to reach the envelope and arp knobs.", -1},
    {"Press SAVE to save your project. Stop editing for two seconds and it saves by itself.", -1},
    {"Play it online:", QR_PLAY},
    {"Hold ENV and press a white key in the drum lanes to add an accent.", -1},
    {"Press SEQ on a drum channel to place hits on the lanes.", -1},
    {"Hold EDIT in the song view to copy a page.", -1},
    {"Tap EDIT in the song view to open that page in the rack.", -1},
    {"Add up to 16 channels, from any mix of consoles.", -1},
    {"Press OCT- or OCT+ to move the keys down or up an octave.", -1},
    {"Hold SEL on a project in the browser to copy it into the next free slot.", -1},
    {"Open DEMO 2 in the project browser to hear every console in one song.", -1},
    {"Get updates here:", QR_SITE},
    {"Turn on Projects At Start in the system menu to pick a project each time you switch on.", -1},
    {"Press FX, then hold the TAPE pad and turn SELECT to slow your song down.", -1},
};
#define TIPN (sizeof TIPS / sizeof TIPS[0])
#define TIP_IDLE 300000u                         /* untouched this long */
#define TIP_EVERY 20000u                         /* then the next tip every so often, until a touch */

static uint32_t tip_n, tip_on, tip_dirty, tip_t0, tip_redo;       /* tip_redo: the next tip is another size, so the view under it is painted again */

static void tip_draw(void)
{
    const tip_t *t = &TIPS[(tip_n + TIPN - 1u) % TIPN];       /* tip_n is the one after */
    uint32_t b, r, q, qr = t->qr >= 0;
    int32_t y0, h, tx = qr ? 28 : 32, ty;
    man_dry = 1; ty = mWords(10, tx, t->text, 196); man_dry = 0;               /* the window is as tall as its text (a code adds its square) */
    h = (qr ? ty + 4 + (QR_N + 8) * 4 : ty + 4) + 29;
    y0 = (236 - h) / 2;
    win_shadow(12, y0, 216, h);
    for (b = 0; b * 80u < (uint32_t)h; b++) {
        win_band(b, 216, h);
        win_chrome(216, h, "Tip", M_ACC);
        win_foot(216, h, 0);
        mWords(10, tx, t->text, 196);
        if (qr) {
            mR(34, ty + 4, (QR_N + 8) * 4, (QR_N + 8) * 4, RGB(255, 255, 255));
            for (r = 0; r < QR_N; r++) for (q = 0; q < QR_N; q++) if (QR_ROW[t->qr][r] >> (QR_N - 1u - q) & 1u) mR(34 + 16 + (int32_t)q * 4, ty + 4 + 16 + (int32_t)r * 4, 4, 4, RGB(0, 0, 0));
        }
        cv_blit(12, (uint32_t)(y0 + (int32_t)b * 80));
    }
}
static void tip_open(void)
{
    tip_on = 1; tip_dirty = 1; tip_t0 = fm1_ms;
    tip_n = (tip_n + 1u) % TIPN;
}
