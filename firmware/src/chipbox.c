/* SPDX-License-Identifier: GPL-3.0-only */
/* Chipbox: sound chips for the FM-1: NES, Game Boy and Master System voices you add as channels, a rack of steps, projects in flash.
 *
 *   PRESETS          down the channels (in the rack the last row adds one: OCT+ on it)
 *   ALGORITHM        rack: the page (past the last makes a new one)     chip screen: the knobs' page (tone / envelope / arp)
 *   SELECT           rack: the cursor along the steps
 *   KNOBS 1..4       chip screen: the sound of the channel; rack: knob 1 is the tempo
 *   keys             play the selected channel (and, in the rack, write it on the step under the cursor; the same note again clears it)
 *   EDIT hold        rack: remove the channel (OCT+ yes, OCT- no)
 *   HOME             tap: the next view (rack, chip, song); hold: the projects
 *   PLAY             the rack and the chip: the page round and round; the song screen: the song, page after page
 *   PLAY / SAVE / GLO   play stop, save, help       OCT- OCT+   octave (on their LEDs)       MASTER   volume
 *   USB-MIDI         CC 102 = play (64 and up) or stop, CC 103 = tempo / 2, CC 104 = save, CC 105 = open a project (0..7), CC 106 / 107 = add a channel of kind d2 / remove the last;
 *                    MIDI channel n plays channel n; program change picks the channel; the device sends CC 100 = CPU %
 *                    (worst block since boot) and CC 101 = open project | 16 if unsaved | 32 if loaded from flash
 *
 * Integers only: no float, no 64-bit division. tests/host.c runs this file on a computer. */
#ifndef CHIPBOX_HOST
#define OTA_PROJ_IO
#endif
#include "platform.c"
#ifdef CHIPBOX_HOST
#define FM1_TICKS_PER_US 24u
static uint32_t fm1_ticks(void) { return 0; }
#endif
#include "chips.c"
static int add_aud_c = -1;                      /* the Add Channel audition's channel on loan (addmenu.c); -1 none */
#include "proj.c"
#include "projio.c"

/* ---- one look per chip */
typedef struct {
    const char *name, *ch[4];
    uint16_t bg, bar, fg, acc, dim, key, lit;
} theme_t;
static const theme_t THEME[CHIPS] = {
    {"NES", {"PU1", "PU2", "TRI"},
     RGB(58, 58, 57), RGB(44, 44, 43), RGB(201, 199, 195), RGB(154, 42, 38), RGB(111, 100, 89), RGB(201, 199, 195), RGB(154, 42, 38)},       /* the console's own colours: charcoal, warm grey, brown-grey, red */
    {"GAME BOY", {"PU1", "PU2", "WAV"},
     RGB(15, 56, 15), RGB(48, 98, 48), RGB(155, 188, 15), RGB(155, 188, 15), RGB(110, 150, 30), RGB(139, 172, 15), RGB(15, 56, 15)},
    {"SMS", {"TN1", "TN2", "TN3"},
     RGB(8, 16, 60), RGB(22, 40, 110), RGB(240, 240, 255), RGB(70, 170, 255), RGB(70, 90, 150), RGB(220, 226, 245), RGB(70, 170, 255)},
    {"PC ENGINE", {"WV1", "WV2", "WV3"},                                          /* the console's white and the orange of its logo, on a warm dark */
     RGB(34, 28, 30), RGB(22, 18, 20), RGB(234, 228, 216), RGB(242, 132, 34), RGB(120, 104, 94), RGB(234, 228, 216), RGB(242, 132, 34)},
    {"ATARI", {"TIA", "TIA", "TIA"},                                              /* the 2600's woodgrain and cream, sand for the notes */
     RGB(38, 26, 18), RGB(26, 17, 11), RGB(244, 226, 196), RGB(214, 158, 88), RGB(128, 96, 66), RGB(244, 226, 196), RGB(214, 158, 88)},
    {"KONAMI", {"PU1", "PU2", "SAW"},                                             /* the Famicom's deep red and gold, on a dark plum */
     RGB(34, 18, 34), RGB(22, 11, 24), RGB(240, 232, 214), RGB(240, 200, 40), RGB(130, 100, 120), RGB(240, 232, 214), RGB(240, 200, 40)},
    {"AY-3-8910", {"AY1", "AY2", "AY3"},                                                 /* a Spectrum: black, a teal accent, the rainbow in the rule */
     RGB(12, 12, 14), RGB(4, 4, 6), RGB(236, 236, 236), RGB(40, 210, 190), RGB(90, 100, 100), RGB(236, 236, 236), RGB(40, 210, 190)},
    {"MEGA DRIVE", {"FM1", "FM2", "FM3"},                                         /* black, Sega blue, a violet for the notes */
     RGB(10, 10, 16), RGB(3, 3, 8), RGB(240, 240, 246), RGB(150, 112, 250), RGB(80, 74, 120), RGB(240, 240, 246), RGB(150, 112, 250)},
    {"VIRTUAL BOY", {"VB1", "VB2", "VB3"},                                        /* red on black, the only colour its screens had */
     RGB(6, 0, 0), RGB(0, 0, 0), RGB(255, 70, 60), RGB(255, 36, 30), RGB(120, 24, 22), RGB(255, 70, 60), RGB(255, 36, 30)},
    {"ATARI 8-BIT", {"PK1", "PK2", "PK3"},                                        /* the 800's plum-black, pink for the notes */
     RGB(30, 14, 26), RGB(18, 8, 16), RGB(240, 226, 232), RGB(236, 88, 140), RGB(124, 76, 100), RGB(240, 226, 232), RGB(236, 88, 140)},
    {"PC SPEAKER", {"SP1", "SP2", "SP3"},                                         /* the BIOS: DOS blue, white text */
     RGB(0, 0, 112), RGB(0, 0, 70), RGB(228, 228, 232), RGB(255, 255, 255), RGB(86, 86, 170), RGB(228, 228, 232), RGB(255, 255, 255)},
    {"ADLIB", {"OP1", "OP2", "OP3"},                                              /* a sound card: a dark green board, gold contacts */
     RGB(6, 26, 22), RGB(3, 16, 14), RGB(230, 236, 224), RGB(236, 184, 60), RGB(70, 110, 90), RGB(230, 236, 224), RGB(236, 184, 60)},
    {"NAMCO WSG", {"WS1", "WS2", "WS3"},                                          /* the arcade: black, the maze's blue bars, Pac-Man yellow */
     RGB(4, 4, 18), RGB(8, 8, 56), RGB(236, 236, 255), RGB(255, 226, 0), RGB(60, 70, 160), RGB(236, 236, 255), RGB(255, 226, 0)},
    {"NAMCO 163", {"N1", "N2", "N3"},                                             /* a cartridge: slate with coral */
     RGB(22, 26, 40), RGB(12, 15, 26), RGB(236, 238, 246), RGB(255, 112, 96), RGB(96, 104, 134), RGB(236, 238, 246), RGB(255, 112, 96)},
    {"SUNSOFT 5B", {"5B1", "5B2", "5B3"},                                         /* a cartridge: night blue, a sun */
     RGB(10, 14, 34), RGB(5, 8, 22), RGB(244, 240, 226), RGB(255, 196, 36), RGB(100, 96, 80), RGB(244, 240, 226), RGB(255, 196, 36)},
    {"NES MMC5", {"MP1", "MP2", "MP3"},                                           /* a cartridge: slate green, mint */
     RGB(18, 30, 30), RGB(10, 18, 18), RGB(222, 236, 230), RGB(96, 214, 168), RGB(84, 118, 108), RGB(222, 236, 230), RGB(96, 214, 168)},
    {"FAMICOM DISK", {"FD1", "FD2", "FD3"},                                       /* the disk drive: deep blue, orange */
     RGB(12, 20, 46), RGB(6, 11, 30), RGB(236, 238, 246), RGB(255, 142, 40), RGB(80, 94, 140), RGB(236, 238, 246), RGB(255, 142, 40)},
    {"MSX SCC", {"SC1", "SC2", "SC3"},                                            /* a game cartridge: near black, lime */
     RGB(10, 14, 22), RGB(5, 8, 14), RGB(232, 240, 226), RGB(190, 232, 48), RGB(86, 106, 70), RGB(232, 240, 226), RGB(190, 232, 48)},
    {"MSX MUSIC", {"FM1", "FM2", "FM3"},                                          /* the FM cartridge: ocean blue, cyan */
     RGB(8, 26, 40), RGB(4, 15, 26), RGB(228, 242, 248), RGB(78, 218, 255), RGB(70, 104, 124), RGB(228, 242, 248), RGB(78, 218, 255)},
    {"KONAMI VRC7", {"VR1", "VR2", "VR3"},                                        /* a cartridge: warm black, vermilion */
     RGB(28, 22, 22), RGB(17, 13, 13), RGB(240, 230, 226), RGB(255, 98, 66), RGB(120, 92, 88), RGB(240, 230, 226), RGB(255, 98, 66)},
    {"YM2151", {"OM1", "OM2", "OM3"},                                             /* the arcade board: black violet, magenta */
     RGB(30, 8, 32), RGB(18, 4, 20), RGB(244, 226, 244), RGB(255, 70, 210), RGB(124, 70, 120), RGB(244, 226, 244), RGB(255, 70, 210)},
    {"OPL3", {"O31", "O32", "O33"},                                               /* a sound card: royal blue board, silver */
     RGB(10, 26, 64), RGB(5, 15, 42), RGB(236, 240, 250), RGB(210, 226, 255), RGB(78, 100, 150), RGB(236, 240, 250), RGB(210, 226, 255)},
    {"WONDERSWAN", {"SW1", "SW2", "SW3"},                                         /* a handheld: indigo, lavender */
     RGB(22, 18, 62), RGB(12, 9, 42), RGB(236, 234, 250), RGB(150, 160, 255), RGB(96, 92, 150), RGB(236, 234, 250), RGB(150, 160, 255)},
    {"SAM COUPE", {"SA1", "SA2", "SA3"},                                          /* a home computer: warm black, yellow */
     RGB(24, 18, 8), RGB(14, 10, 4), RGB(250, 244, 226), RGB(255, 226, 60), RGB(120, 106, 70), RGB(250, 244, 226), RGB(255, 226, 60)},
    {"VIC-20", {"VI1", "VI2", "VI3"},                                             /* the blue screen, a pale cyan */
     RGB(30, 44, 120), RGB(16, 26, 84), RGB(236, 244, 255), RGB(160, 230, 255), RGB(90, 108, 176), RGB(236, 244, 255), RGB(160, 230, 255)},
    {"PLUS/4", {"TD1", "TD2", "TD3"},                                             /* a grey case, a light green */
     RGB(36, 36, 42), RGB(22, 22, 27), RGB(238, 240, 238), RGB(140, 255, 120), RGB(110, 112, 118), RGB(238, 240, 238), RGB(140, 255, 120)},
    {"ATARI LYNX", {"LY1", "LY2", "LY3"},                                         /* a black handheld, orange */
     RGB(10, 14, 12), RGB(4, 7, 6), RGB(238, 240, 236), RGB(255, 150, 30), RGB(110, 90, 60), RGB(238, 240, 236), RGB(255, 150, 30)},
    {"PC-98", {"PC1", "PC2", "PC3"},                                              /* a beige Japanese PC, peach */
     RGB(34, 24, 28), RGB(21, 14, 17), RGB(246, 238, 228), RGB(255, 170, 120), RGB(128, 98, 96), RGB(246, 238, 228), RGB(255, 170, 120)},
    {"NEO GEO", {"NG1", "NG2", "NG3"},                                            /* black and crimson, the MVS cabinet */
     RGB(24, 8, 10), RGB(13, 3, 5), RGB(246, 232, 232), RGB(255, 40, 70), RGB(128, 60, 70), RGB(246, 232, 232), RGB(255, 40, 70)},
    {"NEO GEO POCKET", {"NP1", "NP2", "NP3"},                                     /* a handheld: deep violet, hot pink */
     RGB(22, 16, 34), RGB(12, 8, 22), RGB(240, 234, 248), RGB(255, 110, 170), RGB(110, 90, 130), RGB(240, 234, 248), RGB(255, 110, 170)},
    {"MSX-AUDIO", {"MA1", "MA2", "MA3"},                                          /* a cartridge: dark green, light green */
     RGB(8, 34, 30), RGB(4, 21, 18), RGB(234, 246, 238), RGB(120, 230, 120), RGB(72, 112, 98), RGB(234, 246, 238), RGB(120, 230, 120)},
    {"TX81Z", {"TX1", "TX2", "TX3"},                                              /* a rack synth: charcoal, an LCD cream */
     RGB(30, 26, 22), RGB(18, 15, 12), RGB(246, 240, 224), RGB(255, 235, 170), RGB(120, 108, 88), RGB(246, 240, 224), RGB(255, 235, 170)},
    {"SN76477", {"S71", "S72", "S73"},                                            /* a vector arcade: black, cyan */
     RGB(4, 8, 16), RGB(1, 3, 9), RGB(226, 248, 252), RGB(60, 255, 255), RGB(50, 100, 112), RGB(226, 248, 252), RGB(60, 255, 255)},
    {"NAMCO SYSTEM 1", {"C31", "C32", "C33"},                                     /* a red cabinet, a gold marquee */
     RGB(36, 10, 12), RGB(22, 5, 7), RGB(252, 240, 224), RGB(255, 186, 60), RGB(136, 80, 70), RGB(252, 240, 224), RGB(255, 186, 60)},
    {"KORG POLY-800", {"OR1", "OR2", "OR3"},                                      /* a synth: charcoal, raspberry */
     RGB(24, 18, 26), RGB(14, 10, 16), RGB(246, 234, 240), RGB(255, 60, 120), RGB(122, 80, 98), RGB(246, 234, 240), RGB(255, 60, 120)},
    {"ZX SPECTRUM", {"ZX1", "ZX2", "ZX3"},                                        /* black, a Spectrum red, the rainbow in the rule */
     RGB(8, 8, 10), RGB(2, 2, 4), RGB(240, 240, 240), RGB(236, 60, 56), RGB(100, 100, 106), RGB(240, 240, 240), RGB(236, 60, 56)},
};

static void text_on(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s, uint16_t c, uint16_t bg, int align)
{
    int32_t tw = text_w(f, s), tx = align == 1 ? ((int32_t)w - tw) / 2 : align == 2 ? (int32_t)w - tw : 0;
    cv_begin(w, f->h, bg);
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
}

/* ---- the sound: the chips, timed so the budget shows */
static volatile uint32_t cpu_max;               /* worst audio block so far, in ticks */
static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t t0 = fm1_ticks(), dt;
    chip_render(out, n);
    dt = (uint32_t)(fm1_ticks() - t0);
    if (dt > cpu_max) cpu_max = dt;
}
static int quiet(void)
{
    uint32_t c;
    for (c = 0; c < chan_n; c++)
        if (chip_level(c)) return 0;
    return 1;
}

/* ---- parameters under the knobs */
static uint32_t has_wave(uint32_t c) { return chan[c].kind == KD_GB_WAV || chan[c].kind == KD_PCE || chan[c].kind == KD_VB_WAV || chan[c].kind == KD_WSG || chan[c].kind == KD_N163 || chan[c].kind == KD_FDS || chan[c].kind == KD_SCC || chan[c].kind == KD_WS || chan[c].kind == KD_C30; }
static uint32_t has_fm(uint32_t c) { return chan[c].kind == KD_MD || chan[c].kind == KD_OPL || chan[c].kind == KD_OPM || chan[c].kind == KD_OPL3 || chan[c].kind == KD_PC98_FM || chan[c].kind == KD_NEO_FM || chan[c].kind == KD_OPL1 || chan[c].kind == KD_OPZ; }                              /* page 4 is its operators */
static uint32_t has_vol(uint32_t c) { return chan[c].kind != KD_NES_TRI && chan[c].kind != KD_SPK && chan[c].kind != KD_ZX; }                       /* the NES triangle has no level control */
static uint32_t has_duty(uint32_t c) { return chan[c].kind == KD_NES_PU || chan[c].kind == KD_GB_PU || chan[c].kind == KD_VRC6_PU || chan[c].kind == KD_MMC5 || chan[c].kind == KD_ZX; }
static uint32_t has_buzz(uint32_t c) { return chan[c].kind == KD_S5B || chan[c].kind == KD_FDS || chan[c].kind == KD_OPL3 || chan[c].kind == KD_OPZ || chan[c].kind == KD_SN77 || chan[c].kind == KD_MSM; }                        /* knob 4 on the tone page picks one of four (the 5B's buzz, the Disk System's modulator) */
static uint32_t pages(uint32_t c) { return IS_DRUM(chan[c].kind) ? 1u : has_wave(c) || has_fm(c) ? 4u : 3u; }                  /* pages of knobs */
static uint32_t has_k1(uint32_t c)                                                               /* knob 1 does something on the tone page */
{
    return has_wave(c) || has_duty(c) || has_fm(c) || chan[c].kind == KD_TIA || chan[c].kind == KD_AY || chan[c].kind == KD_POKEY || chan[c].kind == KD_S5B || IS_OPLL(chan[c].kind) || chan[c].kind == KD_SAA || chan[c].kind == KD_TED || chan[c].kind == KD_VIC || chan[c].kind == KD_LYNX || chan[c].kind == KD_PC98_SSG || chan[c].kind == KD_NEO_SSG || chan[c].kind == KD_SN77 || chan[c].kind == KD_MSM;
}
static uint32_t has_pan(uint32_t c) { uint32_t ch = KIND_CHIP[chan[c].kind]; return (ch == CHIP_GB || ch == CHIP_PCE || ch == CHIP_MD || ch == CHIP_VB || ch == CHIP_OPM || ch == CHIP_WS || ch == CHIP_SAA || ch == CHIP_LYNX || ch == CHIP_PC98 || ch == CHIP_NEO || ch == CHIP_NGP || ch == CHIP_C30) && chan[c].kind != KD_PC98_SSG && chan[c].kind != KD_NEO_SSG && !IS_DRUM(chan[c].kind); }
static int8_t clamp_i(int32_t v, int32_t lo, int32_t hi) { return (int8_t)(v < lo ? lo : v > hi ? hi : v); }
static void knob_set(uint32_t c, uint32_t page, uint32_t k, int32_t d)      /* knob k of a page of cards, on channel c */
{
    inst_t *in = &chan[c].in;
    if (page == 3u) {                                           /* the wave: shape, fold, crush, twist (the FM: ratio, index, feedback, decay) */
        uint8_t *p = k == 0u ? &in->wave : k == 1u ? &in->wf : k == 2u ? &in->wc : &in->wt;
        *p = (uint8_t)clamp_i((int32_t)*p + d, 0, 15);
    } else if (page == 1u) {                                    /* the envelope: attack, decay, sustain, release */
        uint8_t *p = k == 0u ? &in->atk : k == 1u ? &in->dec : k == 2u ? &in->sus : &in->rel;
        *p = (uint8_t)clamp_i((int32_t)*p + d, 0, 15);
    } else if (page == 2u) {                                    /* the arp: chord, speed, direction */
        if (k == 0u) in->arp = (uint8_t)clamp_i((int32_t)in->arp + d, 0, 8);
        else if (k == 1u) in->arp_rate = (uint8_t)clamp_i((int32_t)in->arp_rate + d, 0, 3);
        else if (k == 2u) in->arp_dir = (uint8_t)clamp_i((int32_t)in->arp_dir + d, 0, 2);
    } else if (k == 0u) {                                       /* the tone: wave or duty, volume, vibrato, sweep or pan */
        int32_t v;
        if (has_wave(c)) { v = ((int32_t)in->wave + 2) / 5 + (d < 0 ? -1 : 1); in->wave = (uint8_t)((v < 0 ? 0 : v > 3 ? 3 : v) * 5); }       /* pulse, saw, triangle, sine */
        else if (chan[c].kind == KD_TIA) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 0, 10);          /* the sound type */
        else if (chan[c].kind == KD_AY || chan[c].kind == KD_S5B || chan[c].kind == KD_SAA || chan[c].kind == KD_PC98_SSG || chan[c].kind == KD_NEO_SSG) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 0, 15);         /* the noise mix */
        else if (chan[c].kind == KD_TED || chan[c].kind == KD_VIC) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 0, 1);
        else if (chan[c].kind == KD_MSM) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 1, 15);          /* the footages, one bit each */
        else if (chan[c].kind == KD_SN77) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 0, 3);          /* the mixer */          /* tone or noise */
        else if (chan[c].kind == KD_LYNX) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 0, 7);         /* the sound */
        else if (chan[c].kind == KD_POKEY) in->wave = (uint8_t)clamp_i((int32_t)in->wave + d, 0, 7);        /* the distortion */
        else if (IS_OPLL(chan[c].kind)) opll_set(in, chan[c].kind, (uint32_t)clamp_i((int32_t)in->wave + d, 0, 14));       /* the instrument, with its envelope */
        else if (has_fm(c)) in->duty = (uint8_t)clamp_i((int32_t)in->duty + d, 0, chan[c].kind == KD_OPL1 ? 1 : chan[c].kind == KD_OPM || chan[c].kind == KD_PC98_FM || chan[c].kind == KD_NEO_FM || chan[c].kind == KD_OPZ || chan[c].kind == KD_MD ? 7 : 3);          /* the algorithm */
        else if (has_duty(c)) { v = (int32_t)in->duty + d; in->duty = (uint8_t)(v < 0 ? 0 : v > (chan[c].kind == KD_VRC6_PU || chan[c].kind == KD_ZX ? 7 : 3) ? (chan[c].kind == KD_VRC6_PU || chan[c].kind == KD_ZX ? 7 : 3) : v); }
    }
    else if (k == 1u) in->vol = (uint8_t)clamp_i((int32_t)in->vol + d, 0, 15);
    else if (k == 2u) in->vib_dep = (uint8_t)clamp_i((int32_t)in->vib_dep + d, 0, 15);
    else if (has_pan(c)) in->pan = (uint8_t)clamp_i((int32_t)in->pan + d, 0, 2);
    else if (has_buzz(c)) in->pan = (uint8_t)clamp_i((int32_t)in->pan + d, 0, chan[c].kind == KD_OPL3 || chan[c].kind == KD_OPZ ? 7 : 3);          /* the buzz, the modulator */
    else in->sweep = clamp_i((int32_t)in->sweep + d, -16, 16);
}

/* ---- the screen */
static const uint8_t AN_COUNT[CHIPS] = {3, 2, 1, 1, 1, 1, 1, 1, 1};
static void draw_header(const theme_t *t, uint32_t chip)
{
    (void)chip;
    lcd_fill(0, 0, 240, 240, t->bg);                /* (the top bar is header.c's) */
}
/* the waveform a channel plays, 2 periods in w x h */
enum { G_PULSE, G_TRI, G_SAW, G_SINE };
static void wave_glyph(uint32_t kind, uint32_t d8, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    static const int8_t SIN8[8] = {0, 71, 100, 71, 0, -71, -100, -71};
    int32_t p = w / 2, i, k, lo = y + h - 2, hi = lo - (h - 2), px = x, py = lo, nx, ny;
    for (k = 0; k < 2; k++) {
        int32_t x0 = x + k * p;
        if (kind == G_PULSE) {
            int32_t on = p * (int32_t)d8 / 8;
            cv_rect(x0, hi, 2, h, c); cv_rect(x0, hi, on, 2, c);
            cv_rect(x0 + on, hi, 2, h, c); cv_rect(x0 + on, lo, p - on, 2, c);
        } else {
            for (i = 0; i <= 8; i++) {
                nx = x0 + i * p / 8;
                ny = kind == G_TRI ? lo - (i <= 4 ? i : 8 - i) * (h - 2) / 4
                   : kind == G_SAW ? lo - (i == 8 ? 0 : i * (h - 2) / 8)
                   : y + h / 2 - SIN8[i & 7] * (h - 2) / 200;
                if (i) { cv_line(px, py, nx, ny, c); cv_line(px, py + 1, nx, ny + 1, c); }
                px = nx; py = ny;
            }
        }
    }
}
static void channel_glyph(uint32_t c, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t col)
{
    static const uint8_t D8[4] = {1, 2, 4, 6};
    const inst_t *in = &chan[c].in;
    if (has_wave(c)) {
        uint8_t wv[32];
        int32_t i;
        wave_make(in, wv);                                      /* the wave itself, as it is now */
        for (i = 0; i < 32; i++) cv_rect(x + i * w / 32, y + h - 2 - wv[i] * (h - 2) / 15, w / 32 + 1, 2, col);
    } else if (chan[c].kind == KD_NES_TRI) wave_glyph(G_TRI, 0, x, y, w, h, col);
    else if (chan[c].kind == KD_VRC6_SAW) wave_glyph(G_SAW, 0, x, y, w, h, col);
    else if (has_fm(c) || IS_OPLL(chan[c].kind)) wave_glyph(G_SINE, 0, x, y, w, h, col);
    else if (chan[c].kind == KD_SPK) wave_glyph(G_PULSE, 4, x, y, w, h, col);
    else if (chan[c].kind == KD_POKEY) {                        /* the distortion's bits, as they run */
        uint32_t w7 = in->wave > 7u ? 7u : in->wave;
        int32_t i, n = w7 >= 4u ? 24 : POKEY_LEN[w7] < 24u ? (int32_t)POKEY_LEN[w7] : 24;
        for (i = 0; i < 24; i++) {
            uint32_t j = (uint32_t)i % (uint32_t)n;
            cv_rect(x + i * w / 24, (w7 >= 4u ? (uint32_t)((i * 0x5B ^ i * i * 3 >> 1) >> 2 & 1) : POKEY_BITS[w7][j >> 3] >> (j & 7u) & 1u) ? y : y + h - 2, w / 24 + 1, 2, col);
        }
    }
    else if (chan[c].kind == KD_TIA) {                          /* the sound type's bits, as they run */
        uint32_t w9 = in->wave > 10u ? 10u : in->wave;
        int32_t i, n = TIA_LEN[w9] < 24u ? (int32_t)TIA_LEN[w9] : 24;
        for (i = 0; i < 24; i++) {
            uint32_t j = (uint32_t)i % (uint32_t)n;
            cv_rect(x + i * w / 24, TIA_BITS[w9][j >> 3] >> (j & 7u) & 1u ? y : y + h - 2, w / 24 + 1, 2, col);
        }
    }
    else wave_glyph(G_PULSE, has_duty(c) ? (chan[c].kind == KD_VRC6_PU ? (uint32_t)(in->duty & 7u) / 2u + 1u : chan[c].kind == KD_ZX ? D8[(in->duty >> 1) & 3u] : D8[in->duty & 3u]) : 4u, x, y, w, h, col);
}
#define KEY0 53u                                /* key 0 = F3 */
static const int8_t BLK_IDX[27] = {-1, 0, -1, 1, -1, 2, -1, -1, 3, -1, 4, -1, -1, 5, -1, 6, -1, 7, -1, -1, 8, -1, 9, -1, -1, 10, -1};    /* a black key's number, 0 to 10 */
static const uint8_t BLK_KEY[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};        /* the key of each black key */
static void note_name(char *s, uint32_t note)   /* "C#4"; blank for none (0 or 255) */
{
    static const char *const NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    s[0] = 0;
    if (note && note < 128u) {
        const char *p = NAME[note % 12u];
        uint32_t i = 0;
        while (*p) s[i++] = *p++;
        if (note >= 12u) s[i++] = (char)('0' + note / 12u - 1u);
        s[i] = 0;
    }
}
static void draw_note(const theme_t *t, uint32_t note)
{
    char s[8];
    note_name(s, note);
    text_on(160, 6, 72, &FONT_L, s, t->fg, t->bg, 2);
}
#include "rack.c"
#include "header.c"
#include "songview.c"
#include "stepview.c"
#include "fxview.c"
#include "flair.c"
#include "chipview.c"
#include "menu.c"
#include "addmenu.c"
#include "picker.c"

static int32_t enc_act(uint32_t e) { int32_t v = fm1_enc_take(e); if (v) act_ms = fm1_ms; return v; }       /* knobs count as being touched */
#define fm1_enc_take enc_act

#include "boot.c"
static void app_main(void)
{
    struct { int8_t oct; uint8_t an[3], proj, demo, unlocked; } set = {0, {AN_HW, AN_HW, 0}, 0, 0, 0};
    uint32_t prev = 0, k, t_change = 0, dirty = 0, note_drawn = ~0u, chip_drawn = ~0u;
    uint32_t last_note = 255u, view = 1, rack_force = 1, chip_page = 0, sel = 0;     /* view: 0 the chip, 1 the rack, 2 the song; sel: the channel (in the rack, one past the last is "add") */
    uint32_t home_was = 0, home_t0 = 0, home_used = 0, home_now, home_tap, edit_was = 0, edit_t0 = 0, edit_now, edit_used = 0, edit_rel, edit_clr = 255u;
    uint32_t btn_was = 0, crit_next = 0, crit_n = 0, sys_req = 0, play_t0 = 0, play_wait = 0, sel_was = 0, sel_t0 = 0, sel_used = 0, sel_now, dup_was = 0, dup_t0 = 0, dup_used = 0;
    int32_t ed_hold_k = -1;                             /* STEP: the key held now and where its note went: KNOB 2 sets that note's length */
    uint32_t ed_hold_step = 0;
    uint8_t held[27] = {0}, held_ch[27] = {0}, ch_drawn = 99;
    uint8_t rec_hold[27] = {0}, rec_pg[27] = {0}, rec_st[27] = {0}, rec_ch[27] = {0};     /* a recorded note still held: where it went, to set its length when the key comes up */
    uint32_t rec_t0[27] = {0};
    settings_load(&set, sizeof set);
    rack_tails = set.an[2] >> 7;                        /* set.an[2]: the boot screen in bits 0-6, note tails in bit 7 */
#if !defined(CHIPBOX_HOST) || defined(BOOT_DEMO)
    {                                                   /* the next power-on screen in the list; set.an[2] (unused) remembers the last */
        uint32_t last = (set.an[2] & 127u) < BOOT_N ? set.an[2] & 127u : 0u, n = (last + 1u) % BOOT_N;
#ifdef BOOT_DEMO
        n = BOOT_DEMO;
#endif
        boot_seed = fm1_ticks();
        set.an[2] = (uint8_t)(n | (set.an[2] & 128u));
        settings_save(&set, sizeof set);                /* now, while it is silent: a quick power cycle must not show the same one */
        boot_screen(n);
    }
#endif
    proj_demo_sync();                                   /* project 1 is the demo: always there, always as shipped */
    act_ms = fm1_ms;
    proj_cur = set.proj < PROJ_SLOTS ? set.proj : 0;                  /* the project you left open; Korobeiniki on a new device */
    proj_loaded = proj_load(proj_cur) == 0;
    if (!proj_loaded) { if (proj_cur == 0u) seq_tetris(); else if (proj_cur == 1u) seq_symphony(); else proj_blank(); }
    if (set.oct < -2 || set.oct > 2) set.oct = 0;
    for (k = 0; k < 2u; k++)
        if (set.an[k] >= AN_COUNT[k]) set.an[k] = AN_COUNT[k] > 1u ? 1u : 0u;
    set.an[CHIP_NES] = set.an[CHIP_GB] = AN_HW;     /* always the analog stage */
    for (k = 0; k < 2u; k++) analog_mode[k] = set.an[k];

    for (;;) {
        static uint32_t cc_ms;
        const theme_t *t;
        uint32_t keys, down, up, rel, b, lit = 0, base = (uint32_t)(KEY0 + 12 * set.oct), cpu, redraw = 0, in_menu = menu_on, nrows, ch_chip;
        uint8_t st, d1, d2;
        int32_t e;
        platform_poll();
#ifndef CHIPBOX_HOST
        if (px_open_req) {                              /* a project came in over USB: open it */
            uint32_t sl = px_open_req - 1u;
            px_open_req = 0;
            if (sl == proj_cur) proj_dirty = 0;         /* (the open one was just replaced: its unsaved copy is the old one) */
            proj_open(sl); set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; rack_force = 1; chip_drawn = ~0u; sel = 0; rack_inval = 1; view = 1;
            pick_on = 0; menu_on = 0;
        }
#endif
        if (!chan_n && !view) { view = 1; rack_force = 1; chip_drawn = ~0u; }       /* no channels: only the rack makes sense */
        if (rec_on && (view == 2u || view == 3u || !seq_run || sel >= chan_n || pick_on)) { rec_on = 0; duck_ch = 255; }     /* recording stops with the playing, the song screen, the + row */
        if (view == 3u) rack_cur = ed_cur;
        else if (!seq_run || seq_hold) rack_cur = seq_step;                  /* stopped or paused: the playhead is the cursor */
        nrows = chan_n + (view == 1u && chan_n < MAXCH ? 1u : 0u);
        if (sel >= nrows) sel = nrows ? nrows - 1u : 0u;
        if (sel < chan_n && chip_page >= pages(sel)) { chip_page = 0; redraw = 1; }
        ch_chip = sel < chan_n ? KIND_CHIP[chan[sel].kind] : CHIP_NES;
        t = &THEME[ch_chip];

        keys = fm1_in.notes;
        if (keys != prev || fm1_in.buttons != btn_was) act_ms = fm1_ms;
        btn_was = fm1_in.buttons;
        down = keys & ~prev;
        up = prev & ~keys;
        prev = keys;
        if (fx_layer && (menu_on || pick_on)) fx_off();
        if (fx_layer) {                                 /* the punch-in layer: the white keys are the pads, held = on; they play nothing else */
            uint32_t mask = 0, p, now;
            if (!menu_on && !pick_on) {
                uint32_t edge;
                for (p = 0; p < FXN; p++) if (keys >> FX_KEY[p] & 1u) mask |= FXB(p);
                edge = mask & ~fx_raw; fx_raw = mask;
                if (edge & FXB(FX_LATCH)) { fx_lat ^= 1u; if (!fx_lat) fx_tog = 0; }       /* LATCH: pads stay on when tapped, tap again to turn off */
                if (fx_lat) { fx_tog ^= edge & ~FXB(FX_LATCH); mask = fx_tog | FXB(FX_LATCH); }
                else mask &= ~FXB(FX_LATCH);
                now = mask & ~fx_held;
                for (p = 0; p < FXN; p++) if (now >> p & 1u) { if (p == FX_TAPE) anim_tape(); else anim_fx(p); }
                if (!mask && fx_held) { /* all let go: the animation finishes by itself */ }
                else if (mask && (fx_held & ~mask)) { for (p = 0; p < FXN; p++) if (mask >> p & 1u) { an_arg = p; break; } }
                fx_apply(mask);
            }
            down = up = 0;
        }
        for (k = 0; k < 27u; k++) {
            if ((down >> k & 1u) && menu_on && (menu_kind == MK_ADD || menu_kind == MK_ADD2)) { held[k] = (uint8_t)(base + k); held_ch[k] = 255u; add_key_on(held[k]); }       /* the Add Channel list: the keys play the console on the bar */
            else if (down >> k & 1u) {
                held[k] = (uint8_t)(base + k); held_ch[k] = (uint8_t)sel;
                if (sel < chan_n) {
                    uint32_t dr = IS_DRUM(chan[sel].kind), bk = BLK_IDX[k] < 0 ? 99u : (uint32_t)BLK_IDX[k];      /* a drum channel: the black keys are the kit (BD SD CP TM CH OH CY CB) */
                    if (!dr) chip_note(sel, held[k], 127);
                    else if (bk < 11u) { if (view == 3u) ed_lane = bk; chip_note(sel, DR_LANE_TYPE[bk], 127); }
                    last_note = dr ? 255u : held[k]; midi_send(0x90 | (sel & 15u), held[k], 100);
                    if (!menu_on && !pick_on && !rack_rm) {
                        if (dr) {                                                  /* the lanes: the white keys are the 16 steps; the same hit again takes it away */
                            uint32_t p;
                            if (view == 3u) for (p = 0; p < FXN; p++) if (FX_KEY[p] == k) {
                                step_t *x = &pat[seq_view][sel][p];
                                uint32_t h = dstep_hits(x), a = dstep_acc(x), bit = 1u << ed_lane, env = fm1_in.buttons >> BTN_ENV & 1u;
                                if (env) { if (h & bit && a & bit) a &= ~bit; else { h |= bit; a |= bit; } }          /* ENV held: an accented hit (on one already there: accent on, off) */
                                else if (h & bit) { h &= ~bit; a &= ~bit; }                                         /* the same hit again takes it away */
                                else h |= bit;
                                dstep_set(x, h, a); proj_edited();
                            }
                        }
                        else if (rec_on && seq_run && !seq_hold && view != 2u) {      /* recording: the note goes on the step nearest the playhead */
                            uint32_t a, sb, stp = 0, pg;
                            do { a = seq_step; sb = seq_sub; } while (a != seq_step);
                            stp = (a * SEQ_PPS + sb + SEQ_PPS / 2u) / SEQ_PPS % SEQ_LEN; pg = seq_pat;
                            {   step_t *x = &pat[pg][sel][stp];
                                x->note = held[k]; x->vel = 100; x->gate = 5; rec_hold[k] = 1; rec_t0[k] = seq_ticks; rec_pg[k] = (uint8_t)pg; rec_st[k] = (uint8_t)stp; rec_ch[k] = (uint8_t)sel;
                            }
                            rack_inval = 1; proj_edited();
                        } else if (view == 1u && (!seq_run || seq_hold)) { seq_toggle(sel, rack_cur, held[k]); rack_inval = 1; proj_edited(); }
                        else if (view == 3u) { step_t *x = &pat[seq_view][sel][ed_cur]; x->note = held[k]; x->vel = 100; x->gate = 5; ed_hold_k = (int32_t)k; ed_hold_step = ed_cur; ed_cur = (ed_cur + 1u) % SEQ_LEN; proj_edited(); }       /* STEP: the key sets the block and moves on */       /* stopped or paused: on the step the playhead is at */
                    }
                }
            }
            if ((up >> k & 1u) && held_ch[k] == 255u) { add_key_off(held[k]); held_ch[k] = 0; }
            else if (up >> k & 1u) {
                if ((int32_t)k == ed_hold_k) ed_hold_k = -1;
                if (held_ch[k] < chan_n && last_note == held[k]) chip_off(held_ch[k]);
                midi_send(0x80 | (held_ch[k] & 15u), held[k], 0);
                if (rec_hold[k]) {                      /* a recorded note: its length is how long the key was down */
                    step_t *x = &pat[rec_pg[k]][rec_ch[k]][rec_st[k]];
                    uint32_t g = seq_ticks - rec_t0[k];
                    if (x->note == held[k]) x->gate = (uint8_t)(g < 1u ? 1u : g > 96u ? 96u : g);
                    rec_hold[k] = 0; rack_inval = 1;
                }
            }
        }

        b = fm1_input_edges(&rel);
        home_now = fm1_in.buttons >> BTN_HOME & 1u;     /* HOME: a tap goes between the views, a hold opens the projects */
        if (home_now && !home_was) { home_t0 = fm1_ms; home_used = 0; }
        sys_req = 0;                                    /* GLO + HOME, either one first: the menu */
        if (!pick_on && home_now && (((b & (1u << BTN_GLO)) != 0u) || (!home_was && (fm1_in.buttons >> BTN_GLO & 1u)))) { home_used = 1; sys_req = 1; b &= ~(1u << BTN_GLO); }
        if (home_now && !home_used && !menu_on && !pick_on && fm1_ms - home_t0 >= 600u) { pick_open(); home_used = 1; }
        home_tap = !home_now && home_was && !home_used;
        home_was = home_now;
        edit_now = fm1_in.buttons >> BTN_EDIT & 1u;
        edit_rel = !edit_now && edit_was;
        if (edit_now && !edit_was) { edit_t0 = fm1_ms; edit_used = 0; }
        edit_was = edit_now;
        if (pick_on) {                                  /* the projects: SELECT moves, OCT+ yes, OCT- no, hold EDIT to clear one */
            e = fm1_enc_take(ENC_SELECT);
            if (!e) e = fm1_enc_take(ENC_PRESETS);
            if (e) { pick_sel = e < 0 ? (pick_sel ? pick_sel - 1u : 0u) : (pick_sel + 1u < PROJ_SLOTS ? pick_sel + 1u : PROJ_SLOTS - 1u); pick_del = 0; pick_dirty = 1; }
            if (edit_now && fm1_ms - edit_t0 >= 600u && !pick_del && pick_have[pick_sel] && pick_sel >= PROJ_DEMOS) { pick_del = 1; pick_dirty = 1; }
            sel_now = fm1_in.buttons >> BTN_SEL & 1u;                                   /* hold SEL on a project: a copy in the first free slot */
            if (sel_now && !sel_was) { sel_t0 = fm1_ms; sel_used = 0; }
            if (sel_now && !sel_used && !pick_del && pick_have[pick_sel] && fm1_ms - sel_t0 >= 600u) {
                uint32_t f;
                sel_used = 1;
                for (f = PROJ_DEMOS; f < PROJ_SLOTS && pick_have[f]; f++) {}
                if (f < PROJ_SLOTS) { seq_stop(); if (proj_copy(pick_sel, f) == 0) { pick_scan(); pick_sel = f; pick_dirty = 1; } }
            }
            sel_was = sel_now;
            if ((b & (1u << BTN_OCT_UP)) || (home_tap && !pick_del)) {       /* OCT+ (yes) or a HOME tap opens the slot under the frame */
                if (pick_del) {
                    proj_clear(pick_sel);
                    if (pick_sel == proj_cur) { proj_blank(); proj_dirty = 0; sel = 0; }
                    pick_del = 0; pick_scan(); pick_dirty = 1;
                } else {
                    if (pick_sel != proj_cur) {
                        proj_open(pick_sel); anim_start(AN_OPEN, pick_sel);
                        set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; last_note = 255u; sel = 0; rack_rm = 0;
                    }
                    pick_on = 0; rack_force = 1; chip_drawn = ~0u;
                }
            } else if (b & (1u << BTN_OCT_DOWN)) {
                if (pick_del) { pick_del = 0; pick_dirty = 1; } else { pick_on = 0; rack_force = 1; chip_drawn = ~0u; }
            } else if ((b & (1u << BTN_GLO)) || home_tap) { pick_del = 0; pick_on = 0; rack_force = 1; chip_drawn = ~0u; }
            fm1_enc_take(ENC_PRESETS); fm1_enc_take(ENC_ALGORITHM);
            for (k = 0; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else {
        if ((b & (1u << BTN_SEQ)) && !menu_on && !pick_on && !fx_layer) {      /* SEQ: step entry for the channel you are on, and back */
            if (view == 3u) { view = 1; anim_start(AN_VIEW, 1u); rack_force = 1; chip_drawn = ~0u; }
            else if (view != 2u && sel < chan_n) { chip_off(sel); view = 3; ed_cur = 0; anim_start(AN_VIEW, 4u); rack_force = 1; chip_drawn = ~0u; }
        }
        if (sys_req) {                                  /* the menu (it replaces a help list that is open) */
            menu_open_pick(SYSM, sizeof SYSM / sizeof SYSM[0]); menu_kind = MK_MENU;
        } else if (b & (1u << BTN_GLO)) {               /* help for the view you are on; GLO or HOME again goes back */
            if (menu_on) { menu_on = 0; rack_force = 1; chip_drawn = ~0u; }
            else if (fx_layer) { menu_open(HELP_FX, sizeof HELP_FX / sizeof HELP_FX[0]); menu_kind = MK_HELP; }
            else if (view == 2u) { menu_open(HELP_SONG, sizeof HELP_SONG / sizeof HELP_SONG[0]); menu_kind = MK_HELP; }
            else if (view == 3u && sel < chan_n && IS_DRUM(chan[sel].kind)) { menu_open(HELP_DRUM, sizeof HELP_DRUM / sizeof HELP_DRUM[0]); menu_kind = MK_HELP; }
            else if (view == 3u) { menu_open(HELP_STEP, sizeof HELP_STEP / sizeof HELP_STEP[0]); menu_kind = MK_HELP; }
            else if (view) { menu_open(HELP_RACK, sizeof HELP_RACK / sizeof HELP_RACK[0]); menu_kind = MK_HELP; }
            else { menu_open(HELP_CHIP, sizeof HELP_CHIP / sizeof HELP_CHIP[0]); menu_kind = MK_HELP; }
        } else if (home_tap) {
            if (menu_on) menu_on = 0;
            else if (fx_layer) { fx_layer = 0; fx_off(); anim_start(AN_VIEW, view); }       /* HOME leaves the punch-in layer first */
            else if (view == 3u) { view = 1; anim_start(AN_VIEW, 1u); }
            else { view = view == 1u ? (chan_n ? 0u : 2u) : view == 0u ? 2u : 1u; anim_start(AN_VIEW, view); }       /* HOME: the rack, the chip, the song, the rack ... */
            rack_force = 1; chip_drawn = ~0u;
        }
        if (b & (1u << BTN_PLAY)) {                     /* PLAY: in the song, the song from the cursor; anywhere else, the page on show round and round */
            if (seq_run && seq_hold) { seq_rate = 2; seq_hold = 0; anim_start(AN_PLAY, 0); }                   /* paused on the tape: go on */
            else if (seq_run) { seq_stop(); rec_on = 0; duck_ch = 255; anim_start(AN_PAUSE, 0); }
            else if (view == 1u) { play_wait = 1; play_t0 = fm1_ms; }                                          /* the rack: a tap plays on from this page, a hold loops it */
            else { if (view == 2u) seq_play(song_cur < song_len ? song_cur : 0u, 0); else seq_play(0, 1); anim_start(AN_PLAY, 0); }
        }
        if (play_wait) {
            if (fm1_ms - play_t0 >= 350u && (fm1_in.buttons >> BTN_PLAY & 1u)) { play_wait = 0; seq_play(0, 1); anim_start(AN_PLAY, 0); }
            else if (rel & (1u << BTN_PLAY)) {
                uint32_t s = 0;
                play_wait = 0;
                while (s < song_len && song[s] != seq_view) s++;
                if (s < song_len) seq_play(s, 0); else seq_play(0, 1);
                anim_start(AN_PLAY, 0);
            }
        }
        if ((b & (1u << BTN_FX)) && !menu_on) {          /* FX: the punch-in layer on and off */
            fx_layer ^= 1u;
            anim_start(AN_VIEW, fx_layer ? 3u : view);
            if (fx_layer) { for (k = 0; k < 27u; k++) if (prev >> k & 1u && held_ch[k] < chan_n) chip_off(held_ch[k]); }
            else fx_off();
            rack_force = 1; chip_drawn = ~0u;
        }
        if ((b & (1u << BTN_REC)) && !menu_on && view != 2u && view != 3u && sel < chan_n && !IS_DRUM(chan[sel].kind)) {      /* REC: arm (and start the page round and round); again: disarm */
            if (rec_on) { rec_on = 0; duck_ch = 255; seq_stop(); anim_start(AN_REC, 1); }                  /* REC again: stop recording and the playing */
            else { rec_on = 1; duck_ch = (uint8_t)sel; if (!seq_run) seq_play(0, 1); anim_start(AN_REC, 0); }
        }
        if (!edit_now) edit_clr = 255u;
        if (rec_on && edit_now && seq_run && sel < chan_n) {      /* recording, EDIT held: the note at the playhead goes, and each one the playhead reaches while it is held */
            uint32_t cs = seq_step;
            if (cs != edit_clr) {
                step_t *x = &pat[seq_pat][sel][cs];
                edit_clr = cs;
                if (x->note) { x->note = 0; rack_inval = 1; proj_edited(); }
            }
        }
        {                                               /* hold SEL on a channel in the rack: a copy of it (its patch and its steps) at the end */
            uint32_t sn = fm1_in.buttons >> BTN_SEL & 1u;
            if (sn && !dup_was) { dup_t0 = fm1_ms; dup_used = 0; }
            dup_was = sn;
            if (sn && !dup_used && view == 1u && sel < chan_n && !menu_on && !pick_on && !fx_layer && !rack_rm && fm1_ms - dup_t0 >= 600u) {
                dup_used = 1;
                if (chan_n < MAXCH) { menu_open_pick(COPY_MENU, sizeof COPY_MENU / sizeof COPY_MENU[0]); menu_kind = MK_COPY; }       /* a window: patch only, patch and notes, cancel */
            }
        }
        if ((b & (1u << BTN_ARP)) && !menu_on && !pick_on) {          /* ARP: note tails on or off */
            rack_tails ^= 1u; set.an[2] = (uint8_t)((set.an[2] & 127u) | rack_tails << 7); dirty = 1; t_change = fm1_ms; rack_inval = 1;
        }
        if ((b & (1u << BTN_LFO)) && !menu_on && !pick_on) {          /* LFO: undo */
            uint32_t vv = seq_view, sc = song_cur;
            if (undo_go() == 0) {
                if (vv < page_n) seq_view = (uint8_t)vv;
                if (sc <= song_len) song_cur = sc;
                rack_inval = 1; rack_rm = 0; chip_drawn = ~0u; redraw = 1; anim_start(AN_UNDO, 1);
            } else anim_start(AN_UNDO, 0);
        }
        undo_tick();
        if ((b & (1u << BTN_SAVE)) && proj_cur >= PROJ_DEMOS) { add_aud_stop(); if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; anim_start(AN_SAVE, 0); } }
        if (in_menu) {                                  /* in a menu only SELECT does anything (and, in a pick list, OCT+ yes, OCT- no) */
            if (b & ((1u << BTN_OCT_UP) | (1u << BTN_OCT_DOWN))) add_aud_stop();          /* the audition's channel goes before anything is added */
            if (menu_on && menu_kind == MK_ADD) {           /* the Add Channel list: KNOB1 groups, SEL opens the card, OCT- closes it first */
                e = fm1_enc_take(ENC_KNOB1);
                if (e) add_mode_step(e);
                if (b & (1u << BTN_SEL)) { add_card ^= 1u; add_touch(); menu_dirty = 1; }
                if (add_card && (b & (1u << BTN_OCT_DOWN))) { add_card = 0; menu_dirty = 1; b &= ~(1u << BTN_OCT_DOWN); }
            }
            e = fm1_enc_take(ENC_SELECT);
            if (e) { if (menu_kind == MK_ADD) add_scroll(e); else menu_scroll(e); }
            e = fm1_enc_take(ENC_PRESETS);
            if (e) { if (menu_kind == MK_ADD) add_scroll(e); else menu_scroll(e); }
            if (menu_on && (b & (1u << BTN_OCT_UP))) {
                if (menu_kind == MK_ADD) {                      /* a console: its channels */
                    add_console = add_chip(); add_card = 0;
                    menu_open_pick(ADD_SUB[add_console].l, ADD_SUB[add_console].n); menu_kind = MK_ADD2;
                } else if (menu_kind == MK_COPY) {
                    int c;
                    menu_on = 0; rack_force = 1; chip_drawn = ~0u;
                    if (menu_sel < 3u && sel < chan_n && chan_n < MAXCH && (c = chan_add(chan[sel].kind)) >= 0) {       /* 1 patch only, 2 patch and notes */
                        uint32_t p;
                        chan[c].in = chan[sel].in;
                        if (menu_sel == 2u) for (p = 0; p < page_n; p++) memcpy(pat[p][c], pat[p][sel], sizeof pat[p][c]);
                        sel = (uint32_t)c; proj_edited(); rack_inval = 1;
                        anim_start(AN_ADD, KIND_CHIP[chan[c].kind] | (uint32_t)c << 8);
                    }
                } else if (menu_kind == MK_ADD2) {
                    int c = chan_add(ADD_SUB[add_console].kind[menu_sel - 1u]);
                    if (c >= 0) { sel = (uint32_t)c; proj_edited(); anim_start(AN_ADD, KIND_CHIP[chan[c].kind] | (uint32_t)c << 8); }
                    menu_on = 0; rack_force = 1; chip_drawn = ~0u;
                } else if (menu_kind == MK_MENU) {
                    menu_open(CONF_FACTORY, sizeof CONF_FACTORY / sizeof CONF_FACTORY[0]); menu_kind = MK_FACTORY;
                } else if (menu_kind == MK_FACTORY) {
                    seq_stop(); rec_on = 0; duck_ch = 255;
                    for (k = PROJ_DEMOS; k < PROJ_SLOTS; k++) proj_clear(k);               /* every project goes, the demos stay */
                    set.oct = 0;
                    proj_cur = 0; proj_dirty = 0; proj_loaded = proj_load(0) == 0; undo_reset();
                    set.proj = 0; dirty = 1; t_change = fm1_ms;
                    sel = 0; rack_rm = 0; last_note = 255u; view = 1;
                    menu_on = 0; rack_force = 1; chip_drawn = ~0u; anim_start(AN_OPEN, 0);
                }
            } else if (menu_on && (b & (1u << BTN_OCT_DOWN)) && menu_kind == MK_ADD2) { add_open(add_console); }       /* back to the consoles */
            else if (menu_on && (b & (1u << BTN_OCT_DOWN)) && menu_kind != MK_HELP) { menu_on = 0; rack_force = 1; chip_drawn = ~0u; }
            fm1_enc_take(ENC_ALGORITHM);
            for (k = 0; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else if (view == 2u) {                        /* the song: SELECT moves (and plays the page once), PRESETS changes the slot's page, OCT+ a new blank page, OCT- removes the slot, EDIT opens the page in the rack */
            if (song_cur > song_len) song_cur = song_len;
            e = fm1_enc_take(ENC_SELECT);
            if (e) {
                song_cur = (song_cur + (e < 0 ? song_len : 1u)) % (song_len + 1u); rack_inval = 1;
                if (song_cur < song_len) { seq_view = song[song_cur]; if (!seq_run || seq_loop == 2u) { seq_stop(); seq_play(0, 2); } }
            }
            e = fm1_enc_take(ENC_PRESETS);
            if (e && song_cur < song_len) {
                song_cycle(song_cur, e); rack_inval = 1; proj_edited();
                if (!seq_run || seq_loop == 2u) { seq_stop(); seq_play(0, 2); }
            }
            if (b & (1u << BTN_OCT_UP)) {               /* a new slot with a new blank page */
                uint32_t pn = page_n;
                song_insert_blank(song_cur);
                if (page_n != pn) { song_cur = song_cur < song_len ? (song_cur + 1u < song_len ? song_cur + 1u : song_len - 1u) : song_len - 1u; rack_inval = 1; proj_edited(); anim_start(AN_COPY, (uint32_t)seq_view + 1u); }
            }
            if ((b & (1u << BTN_OCT_DOWN)) && song_len > 1u && song_cur < song_len) {
                uint32_t was = song[song_cur];
                song_remove(song_cur); rack_inval = 1; proj_edited();
                if (song_cur >= song_len) song_cur = song_len - 1u;
                anim_start(AN_PAGE, was + 1u);
            }
            if (edit_now && !edit_used && song_cur < song_len && fm1_ms - edit_t0 >= 600u) {      /* hold EDIT: a copy of this slot's page, in this slot */
                int np = seq_dup_page(song[song_cur]);
                if (np >= 0) { song[song_cur] = (uint8_t)np; seq_view = (uint8_t)np; rack_inval = 1; proj_edited(); anim_start(AN_COPY, (uint32_t)np + 1u); }
                edit_used = 1;
            }
            if (edit_rel && !edit_used && song_cur < song_len) {                                  /* a tap on EDIT: this page, in the rack */
                seq_view = song[song_cur]; view = 1; rack_force = 1; chip_drawn = ~0u;
            }
            e = fm1_enc_take(ENC_KNOB1);                /* knob 1: the tempo */
            if (e) { int32_t v = (int32_t)seq_bpm + e; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); proj_edited(); anim_tempo(); }
            fm1_enc_take(ENC_ALGORITHM);
            for (k = 1; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else if (view == 3u) {                        /* STEP: SELECT moves the cursor, PRESETS the channel, ALGORITHM the page, EDIT clears the block, the octave buttons as everywhere */
            uint32_t oct_was = (uint32_t)(set.oct + 2);
            e = fm1_enc_take(ENC_SELECT);
            if (e) ed_cur = (ed_cur + (e < 0 ? SEQ_LEN - 1u : 1u)) % SEQ_LEN;
            e = fm1_enc_take(ENC_PRESETS);
            if (e && chan_n) { chip_off(sel); sel = e < 0 ? (sel ? sel - 1u : 0u) : (sel + 1u < chan_n ? sel + 1u : chan_n - 1u); anim_start(AN_CHAN, sel | (uint32_t)KIND_CHIP[chan[sel].kind] << 8); }
            e = fm1_enc_take(ENC_ALGORITHM);
            if (e) seq_page(e);
            e = fm1_enc_take(ENC_KNOB1);
            if (e) { int32_t v = (int32_t)seq_bpm + e; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); proj_edited(); anim_tempo(); }
            e = fm1_enc_take(ENC_KNOB2);                /* KNOB 2: how long the note is, in steps: the one you are holding, or the one under the cursor */
            if (e && sel < chan_n) {
                uint32_t st = ed_hold_k >= 0 ? ed_hold_step : ed_cur;
                step_t *x = &pat[seq_view][sel][st];
                if (x->note) {
                    int32_t n = ((int32_t)x->gate + 1) / 6 + e, mx = (int32_t)(SEQ_LEN - st);
                    n = n < 1 ? 1 : n > mx ? mx : n;
                    x->gate = (uint8_t)(n * 6 - 1); proj_edited();
                }
            }
            for (k = 2; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
            if ((b & (1u << BTN_EDIT)) && sel < chan_n && IS_DRUM(chan[sel].kind)) {         /* drums: EDIT clears the picked drum on this page */
                uint32_t p, any = 0;
                for (p = 0; p < SEQ_LEN; p++) { step_t *x = &pat[seq_view][sel][p]; uint32_t h = dstep_hits(x); if (h >> ed_lane & 1u) { dstep_set(x, h & ~(1u << ed_lane), dstep_acc(x) & ~(1u << ed_lane)); any = 1; } }
                if (any) proj_edited();
            }
            else if ((b & (1u << BTN_EDIT)) && sel < chan_n) { step_t *x = &pat[seq_view][sel][ed_cur]; if (x->note) { x->note = 0; proj_edited(); } }
            if (b & (1u << BTN_OCT_DOWN) && set.oct > -2) { set.oct--; dirty = 1; t_change = fm1_ms; }
            if (b & (1u << BTN_OCT_UP) && set.oct < 2) { set.oct++; dirty = 1; t_change = fm1_ms; }
            if ((uint32_t)(set.oct + 2) != oct_was) anim_cont(AN_OCT, oct_was << 4 | (uint32_t)(set.oct + 2), 900u);
        } else {
        if (view == 1u && sel == chan_n && chan_n < MAXCH && (b & (1u << BTN_OCT_UP))) {                 /* OCT+ (yes) on the "+" row: add a channel */
            add_open(CHIPS); add_console = 0;
        } else if (rack_rm) {                           /* removing a channel: OCT+ yes, OCT- no */
            if (b & (1u << BTN_OCT_UP)) { uint32_t gone = KIND_CHIP[chan[sel].kind]; seq_stop(); chan_remove(sel); proj_edited(); rack_rm = 0; rack_inval = 1; anim_start(AN_REMOVE, gone | (uint32_t)sel << 8); }
            else if (b & (1u << BTN_OCT_DOWN)) { rack_rm = 0; rack_inval = 1; }
        } else {
            uint32_t oct_was = (uint32_t)(set.oct + 2);
            if (b & (1u << BTN_OCT_DOWN) && set.oct > -2) { set.oct--; dirty = 1; t_change = fm1_ms; }
            if (b & (1u << BTN_OCT_UP) && set.oct < 2) { set.oct++; dirty = 1; t_change = fm1_ms; }
            if ((uint32_t)(set.oct + 2) != oct_was) anim_cont(AN_OCT, oct_was << 4 | (uint32_t)(set.oct + 2), 900u);
        }
        if (view == 1u && sel < chan_n && edit_now && !edit_used && !rack_rm && !rec_on && fm1_ms - edit_t0 >= 600u) { rack_rm = 1; rack_inval = 1; edit_used = 1; anim_start(AN_DELETE, 0); }     /* hold EDIT on a channel: remove it? */
        e = fm1_enc_take(ENC_PRESETS);                  /* always: down the list of channels */
        if (e && nrows) {
            if (sel < chan_n) chip_off(sel);
            sel = e < 0 ? (sel ? sel - 1u : 0u) : (sel + 1u < nrows ? sel + 1u : nrows - 1u);       /* no wrap: the last row is a turn away */
            rack_rm = 0; rack_inval = 1; redraw = 1; last_note = 255u;
            if (sel < chan_n && !fx_layer) anim_start(AN_CHAN, sel | (uint32_t)KIND_CHIP[chan[sel].kind] << 8);
            if (rec_on) duck_ch = (uint8_t)(sel < chan_n ? sel : 255u);
        }
        e = fm1_enc_take(ENC_ALGORITHM);                /* the rack: the page; the chip screen: the knobs' page */
        if (e) {
            if (view == 1u) {
                uint32_t pn = page_n, was = seq_view;
                seq_page(e); rack_inval = 1;
                if (page_n != pn) proj_edited();
                if (seq_view != was) anim_start(AN_PAGE, (was + 1u) | (e < 0 ? 256u : 0u));
            }
            else if (sel < chan_n) { uint32_t np = pages(sel); chip_page = (chip_page + (e < 0 ? np - 1u : 1u)) % np; redraw = 1; }
        }
        e = fm1_enc_take(ENC_SELECT);                   /* the tape dial: middle pauses, right goes on, left goes back; in the punch-in layer only while TAPE is held */
        if (e && (!fx_layer || (fx_held & FXB(FX_TAPE)))) { seq_shuttle(e); anim_tape(); rack_inval = 1; }
        for (k = 0; k < 4u; k++) {
            e = fm1_enc_take(ENC_KNOB1 + k);
            if (!e) continue;
            if (view == 1u) {
                if (k == 0u) { int32_t v = (int32_t)seq_bpm + e; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); rack_inval = 1; proj_edited(); anim_tempo(); }     /* knob 1: the tempo */
            } else if (sel < chan_n) {
                knob_set(sel, chip_page, k, e); redraw = 1; proj_edited();
                if (chip_page == 3u && has_wave(sel)) anim_cont(AN_WAVE, (uint32_t)sel, 1200u);
            }
        }
        }
        }
        while (midi_get(&st, &d1, &d2)) {
            uint32_t mt = st & 0xFu;                         /* MIDI channel n plays channel n */
            if ((st & 0xF0u) == 0xC0u) {                    /* program change: pick the channel */
                if (chan_n) { if (sel < chan_n) chip_off(sel); sel = d1 % chan_n; last_note = 255u; rack_inval = 1; }
                continue;
            }
            if ((st & 0xF0u) == 0xB0u && d1 == 102u) { if (d2 >= 64u) seq_play(0, 0); else if (seq_run) seq_stop(); continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 103u) { seq_bpm = d2 * 2u; proj_edited(); continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 104u) { if (proj_cur >= PROJ_DEMOS && proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; } continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 108u) { fx_apply(d2 == 0u ? 0u : d2 <= FXN ? fx_held | FXB(d2 - 1u) : d2 <= 2u * FXN ? fx_held & ~FXB(d2 - 1u - FXN) : fx_held); if (d2 && d2 <= FXN) anim_fx(d2 - 1u); continue; }     /* test: pad d2-1 on; d2 17..32: pad off; 0: all off */
            if ((st & 0xF0u) == 0xB0u && d1 == 106u) { if (chan_add(d2) >= 0) proj_edited(); continue; }              /* test: add a channel of kind d2 */
            if ((st & 0xF0u) == 0xB0u && d1 == 107u) { if (chan_n) { seq_stop(); chan_remove(chan_n - 1u); proj_edited(); } continue; }     /* test: remove the last channel */
            if ((st & 0xF0u) == 0xB0u && d1 == 105u && d2 < PROJ_SLOTS) { proj_open(d2); set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; rack_force = 1; chip_drawn = ~0u; sel = 0; continue; }
            if ((st & 0xF0u) == 0x90u && d2) { if (mt < chan_n) chip_note(mt, d1, d2); }
            else if ((st & 0xF0u) == 0x80u || (st & 0xF0u) == 0x90u) { if (mt < chan_n) chip_off(mt); }
        }

        for (k = 0; k < 27u; k++)
            if (prev >> k & 1u && last_note == held[k]) lit |= 1u << k;
        if (seq_run && sel < chan_n && vc[sel].on) {            /* playing: the key of the note the playhead has reached lights */
            int32_t ki = (int32_t)vc[sel].note - (int32_t)base;
            if (ki >= 0 && ki < 27) lit |= 1u << ki;
        }
        if (fx_layer) {                                 /* the pads' keys glow, the held ones are bright */
            uint32_t p;
            for (k = 0; k < 27u; k++) { fm1_led_key(KEY_LED(k), 0); fm1_led_glow(KEY_LED(k), 0); }
            for (p = 0; p < FXN; p++) { fm1_led_glow(KEY_LED(FX_KEY[p]), 1); if (fx_mask >> p & 1u) fm1_led_key(KEY_LED(FX_KEY[p]), 1); }
        } else {
            uint32_t dr = sel < chan_n && IS_DRUM(chan[sel].kind);
            if (dr && view == 3u) {                                                  /* the lanes: the picked drum's black key, and a white key for each of its steps */
                uint32_t p;
                lit = 1u << BLK_KEY[ed_lane];
                for (p = 0; p < FXN; p++) if (dstep_hits(&pat[seq_view][sel][p]) >> ed_lane & 1u) lit |= 1u << FX_KEY[p];
            }
            for (k = 0; k < 27u; k++) { fm1_led_glow(KEY_LED(k), dr && BLK_IDX[k] >= 0); fm1_led_key(KEY_LED(k), lit >> k & 1u); }       /* a drum channel: the eight kit keys glow */
        }
        fx_solo_ch = (fx_held & FXB(FX_SOLO)) && sel < chan_n ? (uint8_t)sel : 255u;
        fm1_led_glow(BTN_LFO, undo_cnt > 0u);
        fm1_led_key(BTN_ARP, rack_tails);
        fm1_led_glow(BTN_ENV, view == 3u && sel < chan_n && IS_DRUM(chan[sel].kind));          /* drums: ENV is the accent */
        fm1_led_key(BTN_SEQ, view == 3u);
        fm1_led_key(BTN_FX, fx_layer);
        fm1_led_key(BTN_PLAY, seq_run);
        fm1_led_key(BTN_REC, rec_on);
        fm1_led_glow(BTN_SAVE, proj_dirty);             /* a dim light: edits not saved yet */
        fm1_led_key(BTN_SAVE, proj_t_saved && fm1_ms - proj_t_saved < 500u);
        fm1_led_key(BTN_OCT_DOWN, set.oct < -1);    /* octave: dim at +-1, bright at +-2 */
        fm1_led_glow(BTN_OCT_DOWN, set.oct == -1);
        fm1_led_key(BTN_OCT_UP, set.oct > 1);
        fm1_led_glow(BTN_OCT_UP, set.oct == 1);
        if ((pick_on && pick_del) || rack_rm) { fm1_led_key(BTN_OCT_DOWN, 1); fm1_led_key(BTN_OCT_UP, 1); }     /* yes or no */

        cpu = cpu_max / (58u * FM1_TICKS_PER_US);     /* percent of the 5.8 ms block */
        if (fm1_ms - cc_ms >= 500u) { cc_ms = fm1_ms; midi_send(0xB0, 100, cpu > 127u ? 127u : cpu); midi_send(0xB0, 101, proj_cur | (proj_dirty ? 0x10u : 0u) | (proj_loaded ? 0x20u : 0u)); }
        if (an_kind == AN_CRIT && act_ms >= an_t0) an_end = 0;           /* touched: the critter goes */
        if (!anim_on() && !rec_on && !fx_layer && !menu_on && !pick_on && fm1_ms - act_ms > CRIT_MS && fm1_ms >= crit_next) {
            anim_start(AN_CRIT, crit_n++ % 6u); crit_next = fm1_ms + 15000u;
        }
        add_aud_tick(); add_info_tick();
        if (pick_on) { if (pick_dirty) { pick_draw(); pick_dirty = 0; } }
        else if (menu_on) { if (menu_dirty) { menu_draw(t->acc); menu_dirty = 0; }      /* (the three bands cover the whole screen: no clear first, that was the flash) */ }
        else if (fx_layer) {
            if (rack_force) { fx_view_force = 1; hdr_force = 1; rack_force = 0; }
            fx_view_update(); hdr_update(view, sel, t, ch_chip, last_note);
        }
        else if (view == 3u && sel < chan_n) { if (IS_DRUM(chan[sel].kind)) drum_update(sel, rack_force); else ed_update(sel, base, rack_force); rack_force = 0; hdr_update(3, sel, t, ch_chip, last_note); }
        else if (view == 2u) { sg_update(rack_force); rack_force = 0; hdr_update(2, sel, t, ch_chip, last_note); }
        else if (view == 1u) { rack_update(sel, rack_force); rack_force = 0; hdr_update(1, sel, t, ch_chip, last_note); }
        else if (sel < chan_n) {
            if (chip_drawn != ch_chip) {
                chip_drawn = ch_chip; draw_header(t, ch_chip); hdr_force = 1;
                note_drawn = ~0u; ch_drawn = 99;
            }
            if (redraw || ch_drawn != sel) { chip_panels(t, sel, chip_page); ch_drawn = (uint8_t)sel; }
            hdr_update(0, sel, t, ch_chip, last_note);
        }

        if (proj_dirty && proj_cur >= PROJ_DEMOS && !seq_run && !pick_on && add_aud_c < 0 && quiet() && fm1_ms - proj_t_edit > 2000u && (!proj_t_fail || fm1_ms - proj_t_fail > 10000u)) {   /* stopped and silent: save the project */
            if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; } else proj_t_fail = fm1_ms;
        }
        if (dirty && quiet() && fm1_ms - t_change > 2000u) {
            settings_save(&set, sizeof set);
            dirty = 0;
        }
    }
}
