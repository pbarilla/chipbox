/* SPDX-License-Identifier: GPL-3.0-only */
/* The sound chips: NES 2A03 (pulse, triangle), Game Boy (pulse, wave), Master System PSG (tone), PC Engine HuC6280 (wave), Atari TIA, Konami VRC6 (pulse, saw), AY-3-8910, Mega Drive FM. Included by chipbox.c after platform.c.
 *
 * A channel is one voice of one chip, added by the user: up to MAXCH, any mix (fifteen Game Boy pulses are fine).
 * Layers, top down:
 *   chip_note / chip_off      the app (main loop) -> a small event queue
 *   tick()                    60 Hz, in the audio ISR: envelope, vibrato, sweep, arp -> the voice's oscillator
 *   r_nes / r_gb / r_sms      the chips' mixers, 4x oversampled and averaged down; only voices that sound are visited
 * A chip with nothing playing is not rendered. Tables come from tools/gen_chip_tables.py. Integers only. */
#include "chip_tables.h"

#define OS 4u                                   /* oscillator steps per output frame */
#define MAXCH 16u
enum { CHIP_NES, CHIP_GB, CHIP_SMS, CHIP_PCE, CHIP_TIA, CHIP_VRC6, CHIP_AY, CHIP_MD, CHIP_VB, CHIP_POKEY, CHIP_SPK, CHIP_OPL, CHIP_WSG, CHIP_N163, CHIP_S5B, CHIP_MMC5, CHIP_FDS, CHIP_SCC, CHIP_OPLL, CHIP_VRC7, CHIP_OPM, CHIP_OPL3, CHIP_WS, CHIP_SAA, CHIP_VIC, CHIP_TED, CHIP_LYNX, CHIP_PC98, CHIP_NEO, CHIP_NGP, CHIP_OPL1, CHIP_OPZ, CHIP_SN77, CHIP_C30, CHIP_MSM, CHIP_ZX, CHIP_OPN, CHIP_SID, CHIPS };
enum { KD_NES_PU, KD_NES_TRI, KD_GB_PU, KD_GB_WAV, KD_SMS, KD_PCE, KD_TIA, KD_VRC6_PU, KD_VRC6_SAW, KD_AY, KD_MD, KD_VB_WAV, KD_DRUM0, KD_POKEY = KD_DRUM0 + 9, KD_SPK, KD_OPL, KD_WSG, KD_N163, KD_S5B, KD_DRUMB, KD_MMC5 = KD_DRUMB + 6, KD_FDS, KD_SCC, KD_OPLL, KD_DRUMC, KD_VRC7 = KD_DRUMC + 4, KD_OPM, KD_OPL3, KD_WS, KD_DRUMD, KD_SAA = KD_DRUMD + 4, KD_VIC, KD_TED, KD_LYNX, KD_DRUME, KD_PC98_FM = KD_DRUME + 4, KD_PC98_SSG, KD_NEO_FM, KD_NEO_SSG, KD_NGP, KD_OPL1, KD_OPZ, KD_SN77, KD_C30, KD_DRUMF, KD_MSM = KD_DRUMF + 7, KD_ZX, KD_DRUMG, KD_OPN_FM = KD_DRUMG + 2, KD_OPN_SSG, KD_DRUMH, KD_SID, KD_DRUMI, KINDS = KD_DRUMI + 1 };       /* the drum kits, one a console, in the order of the chips */
#define IS_DRUM(k) (((k) >= KD_DRUM0 && (k) < KD_POKEY) || ((k) >= KD_DRUMB && (k) < KD_MMC5) || ((k) >= KD_DRUMC && (k) < KD_VRC7) || ((k) >= KD_DRUMD && (k) < KD_SAA) || ((k) >= KD_DRUME && (k) < KD_PC98_FM) || ((k) >= KD_DRUMF && (k) < KD_MSM) || ((k) >= KD_DRUMG && (k) < KD_OPN_FM) || ((k) >= KD_DRUMH && (k) < KD_SID) || (k) >= KD_DRUMI)      /* the kits of the first nine consoles come first, the later ones after the later chips */
#define DRUM_KIT(k) ((k) < KD_POKEY ? (k) - KD_DRUM0 : (k) < KD_MMC5 ? 9u + (k) - KD_DRUMB : (k) < KD_VRC7 ? 15u + (k) - KD_DRUMC : (k) < KD_SAA ? 19u + (k) - KD_DRUMD : (k) < KD_PC98_FM ? 23u + (k) - KD_DRUME : (k) < KD_MSM ? 27u + (k) - KD_DRUMF : (k) < KD_OPN_FM ? 34u + (k) - KD_DRUMG : (k) < KD_SID ? 36u + (k) - KD_DRUMH : 37u + (k) - KD_DRUMI)
#define IS_OPLL(k) ((k) == KD_OPLL || (k) == KD_VRC7)                        /* the YM2413 and its Konami cousin: 15 ROM instruments */
static const uint8_t KIND_CHIP[KINDS] = {CHIP_NES, CHIP_NES, CHIP_GB, CHIP_GB, CHIP_SMS, CHIP_PCE, CHIP_TIA, CHIP_VRC6, CHIP_VRC6, CHIP_AY, CHIP_MD, CHIP_VB,
                                         CHIP_NES, CHIP_GB, CHIP_SMS, CHIP_PCE, CHIP_TIA, CHIP_VRC6, CHIP_AY, CHIP_MD, CHIP_VB,
                                         CHIP_POKEY, CHIP_SPK, CHIP_OPL, CHIP_WSG, CHIP_N163, CHIP_S5B,
                                         CHIP_POKEY, CHIP_SPK, CHIP_OPL, CHIP_WSG, CHIP_N163, CHIP_S5B,
                                         CHIP_MMC5, CHIP_FDS, CHIP_SCC, CHIP_OPLL,
                                         CHIP_MMC5, CHIP_FDS, CHIP_SCC, CHIP_OPLL,
                                         CHIP_VRC7, CHIP_OPM, CHIP_OPL3, CHIP_WS,
                                         CHIP_VRC7, CHIP_OPM, CHIP_OPL3, CHIP_WS,
                                         CHIP_SAA, CHIP_VIC, CHIP_TED, CHIP_LYNX,
                                         CHIP_SAA, CHIP_VIC, CHIP_TED, CHIP_LYNX,
                                         CHIP_PC98, CHIP_PC98, CHIP_NEO, CHIP_NEO, CHIP_NGP, CHIP_OPL1, CHIP_OPZ, CHIP_SN77, CHIP_C30,
                                         CHIP_PC98, CHIP_NEO, CHIP_NGP, CHIP_OPL1, CHIP_OPZ, CHIP_SN77, CHIP_C30,
                                         CHIP_MSM, CHIP_ZX,
                                         CHIP_MSM, CHIP_ZX,
                                         CHIP_OPN, CHIP_OPN, CHIP_OPN,
                                         CHIP_SID, CHIP_SID};

/* ---- instruments: what a note sounds like, per channel */
typedef struct {
    uint8_t wave, duty;                         /* wave: the Game Boy wave's shape 0..15 (pulse, saw, triangle, sine at 0, 5, 10, 15); duty: pulse duty */
    uint8_t wf, wc, wt;                         /* the wave's fold, crush and twist 0..15 */
    uint8_t atk, dec, sus, rel;                 /* 0..15: ticks to rise, ticks to fall (x2), level, ticks to fade (x2) */
    uint8_t vib_spd, vib_dep;                   /* vibrato: speed, depth in 1/16 semitone */
    int8_t sweep;                               /* pitch change per tick, 1/64 semitone */
    uint8_t pan;                                /* 0 left, 1 centre, 2 right (Game Boy) */
    uint8_t vol;                                /* 0..15, the channel's level */
    uint8_t arp, arp_rate, arp_dir;             /* arp: 0 off, 1..8 a chord; rate 0..3 (slow to fast); direction 0 up, 1 down, 2 up and down */
} inst_t;
#define I(w, d, a, de, s, r, vs, vd, sw, p) {w, d, 0, 0, 0, a, de, s, r, vs, vd, sw, p, 15, 0, 1, 0}
#define IV(w, d, a, de, s, r, vs, vd, sw, p, v) {w, d, 0, 0, 0, a, de, s, r, vs, vd, sw, p, v, 0, 1, 0}
/* default levels: vol sets a fresh channel about as loud as any other chip's (A-weighted, tests/level_check.c; melodic and kits about 63 dB) */
static const inst_t INST_DEF[KINDS] = {
    I(0, 2, 0, 5, 9, 4, 5, 0, 0, 1),            /* NES pulse */
    I(0, 0, 0, 0, 15, 2, 5, 0, 0, 1),           /* NES triangle */
    IV(0, 2, 0, 5, 9, 4, 5, 0, 0, 1, 14),            /* Game Boy pulse */
    I(15, 0, 0, 0, 15, 2, 5, 0, 0, 1),          /* Game Boy wave */
    IV(0, 0, 0, 6, 14, 4, 5, 4, 0, 1, 14),           /* Master System tone */
    I(5, 0, 0, 0, 12, 2, 5, 0, 0, 1),           /* PC Engine wave: a saw to start with */
    IV(5, 0, 0, 4, 12, 3, 5, 0, 0, 1, 7),           /* Atari TIA: wave = the sound type 0..9 */
    IV(0, 3, 0, 5, 10, 4, 5, 0, 0, 1, 14),           /* VRC6 pulse: duty 0..7 */
    IV(0, 0, 0, 0, 15, 2, 5, 0, 0, 1, 6),           /* VRC6 saw */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 1),           /* AY: wave = the noise mix 0..15 */
    {1, 4, 9, 0, 4, 0, 6, 12, 3, 5, 0, 0, 1, 13, 0, 1, 0},     /* Mega Drive FM: duty = algorithm 0..7, wave = ratio, wf = index, wc = feedback, wt = index decay */
    IV(10, 0, 0, 0, 15, 2, 5, 0, 0, 1, 14),          /* Virtual Boy: a wave, a triangle to start with */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),     /* drums: a kit per console */
    IV(0, 0, 0, 4, 15, 4, 5, 0, 0, 1, 14), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    IV(1, 0, 0, 4, 12, 3, 5, 0, 0, 1, 11),           /* POKEY: wave = the distortion 0..7 */
    I(0, 0, 0, 0, 15, 1, 5, 0, 0, 1),           /* PC speaker: one square, no volume */
    {1, 0, 10, 0, 3, 0, 6, 12, 3, 5, 0, 0, 1, 14, 0, 1, 0},     /* AdLib OPL2: duty = the pair 0..3, wave = ratio, wf = index, wc = feedback, wt = index decay */
    IV(5, 0, 0, 0, 12, 2, 5, 0, 0, 1, 12),           /* Namco WSG: a saw to start with */
    IV(10, 0, 0, 0, 15, 2, 5, 0, 0, 1, 4),          /* Namco 163: a triangle */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 0),           /* Sunsoft 5B: wave = the noise mix, pan = the buzz (0 off) */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), IV(0, 0, 0, 4, 15, 4, 5, 0, 0, 1, 14), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), IV(0, 0, 0, 4, 15, 4, 5, 0, 0, 1, 12), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    I(0, 2, 0, 5, 9, 4, 5, 0, 0, 1),            /* NES MMC5 pulse */
    IV(10, 0, 0, 0, 15, 2, 5, 0, 0, 0, 7),          /* Famicom Disk wave: a triangle, pan = the modulator's depth (0 off) */
    IV(5, 0, 0, 0, 12, 2, 5, 0, 0, 1, 11),           /* Konami SCC: a saw to start with, an 8 bit wave */
    I(0, 0, 0, 0, 0, 0, 0, 0, 0, 1),            /* MSX Music: wave = the instrument 0..14 (the envelope is set by chan_add) */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    IV(0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 10),            /* Konami VRC7: wave = the instrument 0..14 (the envelope is set by chan_add) */
    {1, 4, 9, 0, 4, 0, 6, 12, 3, 5, 0, 0, 1, 14, 0, 1, 0},     /* YM2151: duty = algorithm 0..7, wave = ratio, wf = index, wc = feedback, wt = index decay, pan = left, centre, right */
    {1, 1, 8, 0, 4, 0, 6, 12, 3, 5, 0, 0, 0, 13, 0, 1, 0},     /* OPL3: duty = connection 0..3, wave = ratio, wf = index, wc = feedback, wt = index decay, pan = the wave 0..7 */
    IV(10, 0, 0, 0, 15, 2, 5, 0, 0, 1, 12),          /* WonderSwan: a 4 bit wave, a triangle to start with; pan = left, centre, right */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    IV(0, 0, 0, 5, 12, 3, 5, 0, 0, 1, 10),           /* SAA1099: wave = the noise mix 0..15, pan = left, centre, right */
    IV(0, 0, 0, 5, 12, 3, 5, 0, 0, 1, 10),           /* VIC-20: one square, the pitch a 7 bit divide */
    IV(0, 0, 0, 5, 12, 3, 5, 0, 0, 1, 9),           /* Plus/4 TED: wave = 0 tone, 1 noise */
    IV(1, 0, 0, 4, 12, 3, 5, 0, 0, 1, 10),           /* Atari Lynx: wave = the sound 0..7 (a square, then shift register taps), pan = left, centre, right */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    {1, 4, 9, 0, 4, 0, 6, 12, 3, 5, 0, 0, 1, 14, 0, 1, 0},     /* PC-98 FM (YM2608): like the YM2151 */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 1),           /* PC-98 SSG: the AY's tone and noise */
    {1, 4, 9, 0, 4, 0, 6, 12, 3, 5, 0, 0, 1, 14, 0, 1, 0},     /* Neo Geo FM (YM2610) */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 1),           /* Neo Geo SSG */
    IV(0, 0, 0, 6, 14, 4, 5, 4, 0, 1, 14),           /* Neo Geo Pocket: a tone, stereo */
    {1, 0, 10, 0, 3, 0, 6, 12, 3, 5, 0, 0, 1, 14, 0, 1, 0},     /* MSX-Audio (YM3526): duty = 0 FM, 1 both play */
    {1, 4, 9, 0, 4, 0, 6, 12, 3, 5, 0, 0, 0, 14, 0, 1, 0},     /* Yamaha OPZ: duty = algorithm 0..7, pan = the wave 0..7 */
    IV(0, 0, 0, 3, 14, 3, 5, 0, 0, 0, 11),           /* SN76477: wave = the mixer 0..3, pan = the slow oscillator 0..3 */
    IV(5, 0, 0, 0, 12, 2, 5, 0, 0, 1, 12),           /* Namco C30: a 4 bit wave, stereo */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    IV(6, 0, 0, 3, 14, 3, 5, 0, 0, 0, 7),           /* MSM5232: wave = the footages 16' 8' 4' 2' as four bits, pan = 5 1/3' and 2 2/3' as two */
    I(0, 6, 0, 0, 15, 1, 5, 0, 0, 1),           /* ZX Spectrum beeper: duty = the pulse width 0..7, no volume */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), IV(0, 0, 0, 4, 15, 4, 5, 0, 0, 1, 14),
    {1, 4, 9, 0, 4, 0, 6, 12, 3, 5, 0, 0, 1, 14, 0, 1, 0},     /* YM2203 FM: like the YM2151, one speaker */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 1),           /* YM2203 SSG */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    {3, 8, 0, 8, 0, 0, 8, 12, 7, 5, 0, 0, 1, 10, 0, 1, 0},     /* SID: the settings are nibbles (sid.c SP_*): wave 3 = pulse, 6581; pulse width 8 = the square; cutoff 8, filter off */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
};
static const char *const KIND_NAME[KINDS] = {"PU", "TRI", "PU", "WAV", "TN", "PCE", "TIA", "PU", "SAW", "AY", "FM", "VB", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "POK", "SPK", "OPL", "WSG", "N16", "5B", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "MM5", "FDS", "SCC", "YM", "DRM", "DRM", "DRM", "DRM", "VR7", "OPM", "OP3", "WS", "DRM", "DRM", "DRM", "DRM", "SAA", "VIC", "TED", "LYN", "DRM", "DRM", "DRM", "DRM", "FM", "SSG", "FM", "SSG", "NGP", "MA", "OPZ", "77", "C30", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "ORG", "ZX", "DRM", "DRM", "FM", "SSG", "DRM", "SID", "DRM"};

/* the arp's chords: octave, major, minor, maj7, min7, sus4, power, dim */
static const uint8_t ARP_N[8] = {2, 4, 4, 4, 4, 4, 4, 4};
static const int8_t ARP_IV[8][4] = {
    {0, 12, 0, 12}, {0, 4, 7, 12}, {0, 3, 7, 12}, {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 12}, {0, 7, 12, 19}, {0, 3, 6, 9},
};
static const uint8_t ARP_TICKS[4] = {6, 4, 3, 2};               /* 60 Hz ticks per arp note: 10, 15, 20, 30 a second */
static uint32_t arp_offset(uint32_t shape, uint32_t dir, uint32_t k)      /* semitones above the note, for the k-th arp note */
{
    uint32_t n = ARP_N[shape - 1u], i;
    if (dir == 2u && n > 1u) { k %= 2u * n - 2u; i = k < n ? k : 2u * n - 2u - k; }
    else { i = k % n; if (dir == 1u) i = n - 1u - i; }
    return (uint32_t)ARP_IV[shape - 1u][i];
}

/* ---- the channels */
typedef struct { uint8_t kind; inst_t in; } chan_t;
static chan_t chan[MAXCH];
static uint32_t chan_n;
static volatile uint32_t chan_lock;             /* set while the main loop changes the channel list: the audio side stands still */

/* ---- the event queue: main loop -> audio ISR (one writer each side) */
enum { EV_ON, EV_OFF };
static struct { uint8_t op, ch, a, b; } evq[32];
static volatile uint32_t ev_w, ev_r;
static void ev_push(uint32_t op, uint32_t ch, uint32_t a, uint32_t b)
{
    uint32_t w = ev_w;
    if (w - ev_r >= 32u) return;
    evq[w & 31u].op = (uint8_t)op; evq[w & 31u].ch = (uint8_t)ch; evq[w & 31u].a = (uint8_t)a; evq[w & 31u].b = (uint8_t)b;
    ev_w = w + 1u;
}
static void chip_note(uint32_t c, uint32_t note, uint32_t vel) { ev_push(EV_ON, c, note, vel); }
static void chip_off(uint32_t c) { ev_push(EV_OFF, c, 0, 0); }

/* ---- per-channel note state (tick) and oscillator (render) */
enum { ST_ATK, ST_DEC, ST_SUS, ST_REL, ST_IDLE };
static struct {
    uint8_t on, note, vel, stage, vph, arp_t, arp_off;
    uint8_t trig;                               /* a note began: the SID's gate rises (sid.c clears it) */
    uint16_t age;                               /* ticks since the note began */
    uint16_t env;                               /* Q8, 0..15 */
    uint32_t arp_k;
    int32_t bend;                               /* Q8 semitones from sweep */
    inst_t in;
} vc[MAXCH];
static struct { uint32_t ph, inc; const uint8_t *wav; uint8_t duty, vol, pan, wshift; uint32_t ph2, ph3, ph4, inc2, inc3, inc4, idx, rng, fbm; uint16_t gain; int32_t y1, y2; uint8_t alg, fb; } osc[MAXCH];
#define AWAKE 30u                               /* ticks a chip keeps rendering after its last voice ends */
static uint8_t awake[CHIPS], booted;
static uint32_t tick_left, tick_len = FS / 60u;

static uint32_t interp32(const uint32_t *T, uint32_t pq)
{
    uint32_t n = pq >> 8, fr = pq & 255u, a = T[n], b = T[n < 127u ? n + 1u : n];
    return a + (uint32_t)((((int32_t)(b - a)) >> 8) * (int32_t)fr);
}
static uint32_t interp(const uint16_t *T, uint32_t pq)  /* a table indexed by note, between notes by the Q8 fraction */
{
    uint32_t n = pq >> 8, fr = pq & 255u;
    int32_t a = T[n], b = T[n < 127u ? n + 1u : n];
    return (uint32_t)(a + (((b - a) * (int32_t)fr) >> 8));
}

/* ---- the analog output stage, per chip: what the real board does to the DAC's signal.
 *   NES (front loader): high-pass 90 Hz, high-pass 440 Hz, low-pass 14 kHz   (nesdev wiki, APU Mixer)
 *   Famicom: high-pass 37 Hz (its RF stage varies and is not modelled)
 *   Game Boy (DMG): output capacitor, charge factor 0.999958 per 4.19 MHz cycle, about 28 Hz   (Pan Docs)
 *   clean: only a 14 Hz DC blocker. SMS: nothing. */
enum { AN_CLEAN, AN_HW, AN_ALT, AN_MODES };
static volatile uint32_t analog_mode[CHIPS] = {AN_HW, AN_HW};
typedef struct { int32_t x1, y1; } hpf_t;
#define STEREO_IDX(ch) ((ch) == CHIP_OPM ? 0 : (ch) == CHIP_WS ? 1 : (ch) == CHIP_SAA ? 2 : (ch) == CHIP_LYNX ? 3 : (ch) == CHIP_PC98 ? 4 : (ch) == CHIP_NEO ? 5 : (ch) == CHIP_NGP ? 6 : (ch) == CHIP_C30 ? 7 : -1)       /* the later chips with a right channel of their own */
static hpf_t hp_mm[2];
static hpf_t hp_nes[2], hp_gb[2], hp_x[2], hp_vb[2], hp_md[2], hp_nx[30], hp_nr[8];       /* ... and the VRC6 and the AY keep a DC blocker */               /* NES: two stages; Game Boy: left, right */
static int32_t lp_sp, lp_ws;
typedef struct { int32_t y, x; } lpb_t;
static lpb_t lp_nes, lp_mm, lp_fds;
static int32_t hpf(hpf_t *f, int32_t x, int32_t b)
{
    int32_t t = f->y1 + x - f->x1, y;
    if (t > 0) y = t - ((t * b + 65535) >> 16); else y = t + ((-t * b + 65535) >> 16);       /* the leak rounds up, towards zero: with a floor a DC offset of up to +500 stayed for good */
    f->x1 = x; f->y1 = y;
    return y;
}
/* the NES 14 kHz and FDS 2 kHz low-passes: bilinear (the one pole's own shape at the chip's corner, k = t / (1 + t), t = tan(pi fc / FS), Q14); the plain one pole above sags 1 to 2 dB too much at 4 to 10 kHz */
#define K_NES_LP14K 9955
#define K_FDS_LP2K 2055
static int32_t lpb(lpb_t *f, int32_t x, int32_t k)
{
    int32_t o;
    x = x > 100000 ? 100000 : x < -100000 ? -100000 : x;        /* (the products below stay inside 32 bits) */
    o = f->y + (((x + f->x - 2 * f->y) * k) >> 14);
    f->x = x; f->y = o;
    return o;
}
static int32_t lpf(int32_t *y, int32_t x, int32_t a)
{
    *y += ((x - *y) * a) >> 14;
    return *y;
}
static void post(uint32_t chip, int32_t *out, uint32_t n)
{
    uint32_t i, m = analog_mode[chip];
    if (chip >= CHIP_SMS && chip != CHIP_VRC6 && chip != CHIP_AY && chip != CHIP_VB && chip != CHIP_MD && chip < CHIP_POKEY) return;
    for (i = 0; i < n; i++) {
        int32_t l = out[2u * i], r = out[2u * i + 1u];
        if (chip == CHIP_NES) {
            if (m == AN_HW) l = lpb(&lp_nes, hpf(&hp_nes[1], hpf(&hp_nes[0], l, B_NES_HP90), B_NES_HP440), K_NES_LP14K);
            else if (m == AN_ALT) l = hpf(&hp_nes[0], l, B_FAM_HP37);
            else l = hpf(&hp_nes[0], l, B_DC_HP14);
            r = l;
        } else if (chip == CHIP_VB) {
            l = hpf(&hp_vb[0], l, B_DC_HP14); r = hpf(&hp_vb[1], r, B_DC_HP14);
        } else if (chip == CHIP_MD) {                    /* the YM2612's FM leaves a little DC (feedback, the DAC step): it would click at every note off */
            l = hpf(&hp_md[0], l, B_DC_HP14); r = hpf(&hp_md[1], r, B_DC_HP14);
        } else if (chip >= CHIP_POKEY) {                 /* the later chips: a DC blocker each; the PC speaker's little cone, the Namco DAC's filter */
            hpf_t *h = &hp_nx[chip - CHIP_POKEY];
            if (chip == CHIP_SPK) l = lpf(&lp_sp, hpf(h, l, B_SPK_HP), A_SPK_LP);
            else if (chip == CHIP_WSG) l = lpf(&lp_ws, hpf(h, l, B_DC_HP14), A_WSG_LP);
            else if (chip == CHIP_FDS) l = lpb(&lp_fds, hpf(h, l, B_DC_HP14), K_FDS_LP2K);
            else if (chip == CHIP_MMC5) {                                                  /* the cartridge's pulses go out through the console's own filters: it follows the NES chip's analog setting */
                const uint32_t nm = analog_mode[CHIP_NES];
                if (nm == AN_HW) l = lpb(&lp_mm, hpf(&hp_mm[1], hpf(&hp_mm[0], l, B_NES_HP90), B_NES_HP440), K_NES_LP14K);
                else if (nm == AN_ALT) l = hpf(&hp_mm[0], l, B_FAM_HP37);
                else l = hpf(&hp_mm[0], l, B_DC_HP14);
            }
            else l = hpf(h, l, B_DC_HP14);
            r = STEREO_IDX(chip) >= 0 ? hpf(&hp_nr[STEREO_IDX(chip)], r, B_DC_HP14) : l;
        } else if (chip != CHIP_GB) {
            l = hpf(&hp_x[chip == CHIP_AY], l, B_DC_HP14); r = l;
        } else {
            int32_t b = m == AN_CLEAN ? B_DC_HP14 : B_GB_HPDMG;
            l = hpf(&hp_gb[0], l, b); r = hpf(&hp_gb[1], r, b);
        }
        out[2u * i] = l; out[2u * i + 1u] = r;
    }
}

/* ---- the oscillators: a note's pitch and level into a voice's oscillator */
static const uint8_t NES_DUTY[4] = {0x40, 0x60, 0x78, 0x9F};     /* 8 steps, bit = step */
static const uint8_t GB_DUTY[4] = {0x01, 0x81, 0x87, 0x7E};
/* the Game Boy wave: 32 steps of 4 bits made from the shape (a blend of pulse, saw, triangle, sine), twisted, folded and crushed */
static uint8_t wtab[MAXCH][64];                 /* 32 steps; the FDS reads all 64 */
static uint32_t wkey[MAXCH];
static void wave_make_s(const inst_t *in, uint8_t *out, int32_t top, int32_t n)       /* top 15: 4 bit; 63: the Virtual Boy's 6 bit; n steps, 32 or 64 (the FDS) */
{
    int32_t i, h = n / 2, k = n / 32, lv = (top + 1) - (int32_t)(in->wc & 15u) * ((top + 1) / 16), sh = (int32_t)(in->wave & 15u), a = sh / 5, fr = (sh % 5) * 256 / 5;
    if (a > 2) { a = 2; fr = 256; }
    if (lv < 2) lv = 2;
    for (i = 0; i < n; i++) {
        int32_t vb = n == 32 ? VIB[i] : SIN_T[(i * 16) & 1023] * 127 >> 14, ph, v[4], x, g;
        ph = (i + (vb * (int32_t)(in->wt & 15u) * k) / 238) & (n - 1);
        v[0] = ph < h ? 127 : -127;
        v[1] = -127 + ph * 254 / (n - 1);
        v[2] = ph < h ? -127 + ph * 254 / (h - 1) : 127 - (ph - h) * 254 / (h - 1);
        v[3] = n == 32 ? VIB[(ph + 24) & 31] : SIN_T[((ph + 48) * 16) & 1023] * 127 >> 14;      /* a sine that starts at the bottom, like the others */
        if (a == 0 && fr) v[0] = -v[0];                      /* the pulse between pulse and saw is upside down: its fundamental then adds to the saw's instead of cancelling it (a 14 dB hole at 3) */
        x = v[a] + (v[a + 1] - v[a]) * fr / 256;
        g = 256 + (int32_t)(in->wf & 15u) * 64;
        x = x * g >> 8;
        while (x > 127 || x < -127) x = x > 127 ? 254 - x : -254 - x;
        x = ((x + 127) * (lv - 1) + 127) / 254;             /* 0 .. lv - 1 */
        out[i] = (uint8_t)(x * top / (lv - 1));
    }
}
static void wave_make_n(const inst_t *in, uint8_t *out, int32_t top) { wave_make_s(in, out, top, 32); }
static void wave_make(const inst_t *in, uint8_t *out) { wave_make_n(in, out, 15); }
static const uint8_t NZ_M[16] = {1, 1, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 16};       /* the noise clock as a multiple of the note: the NOISE knob 0..15 */
static uint32_t opll_kf(uint32_t note, uint32_t ksr)      /* the chip's key scaling of its envelope rates: x 2^(t / 4), t = 2 block + fnum bit 8 (a quarter of it unless the operator's KSR bit is set), as Q8 */
{
    const uint32_t b = note <= 30u ? 0u : note >= 103u ? 7u : (note - 31u) / 12u + 1u, k = 2u * b + (note >= 19u), t = ksr ? k : k >> 2;
    return ((256u << (t >> 2)) * (4u + (t & 3u))) >> 2;
}
static uint32_t att375(uint32_t a)                        /* an attenuation in the OPLL's 0.375 dB steps as a gain, Q16 */
{
    return a > 253u ? 0u : a & 1u ? DB_ATT[a >> 1] * 245u >> 8 : DB_ATT[a >> 1];
}
static uint32_t tre_ph, vib_ph;                 /* the OPLL's two slow oscillators, 16 bit phases, stepped by the 60 Hz tick */
static uint32_t fm_idx(const inst_t *wi, uint32_t age)       /* the modulation depth now: the INDEX knob, falling at the DECAY knob's dB a second */
{
    uint32_t u = (uint32_t)FM_DEC[wi->wt & 15u] * age / 60u;
    return FM_IDX[wi->wf & 15u] * (DB_ATT[u > 127u ? 127u : u] >> 8) >> 8;
}
static uint32_t vol_q;                          /* the voice's level before it is rounded to a step: Q4 (16 to a 3 dB step), 0..240; the FM kinds use it */
static uint32_t fm_gain(uint32_t q)             /* its gain in 0.75 dB steps (the chips' TL step), Q16 of the old PCE_AMP[15] / 256; the last step fades to silence */
{
    if (q >= 16u) return DB_ATT[(240u - q + 2u) >> 2] * 100u >> 8;
    return (DB_ATT[56] * 100u >> 8) * q >> 4;
}
static uint32_t fm_gain_att(uint32_t q, uint32_t att)    /* the same with an extra attenuation in 0.375 dB steps (key scale, tremolo) */
{
    return att ? fm_gain(q) * att375(att) >> 16 : fm_gain(q);
}
#include "sid.c"
static const uint8_t S5B_NP[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 19, 22, 25, 28, 31};       /* the noise period registers the PERIOD knob picks (0 = follows the note) */
static void set_osc(uint32_t c, uint32_t pq, uint32_t vol)
{
    const inst_t *in = &vc[c].in;
    uint32_t t;
    switch (chan[c].kind) {
    case KD_NES_PU: t = interp(NES_PT, pq); osc[c].inc = K_NES / (t + 1u); osc[c].duty = in->duty & 3u; osc[c].vol = (uint8_t)vol; break;
    case KD_NES_TRI: t = interp(NES_TT, pq); osc[c].inc = (K_NES / (t + 1u)) >> 1; osc[c].vol = vol > 0u; break;
    case KD_GB_PU: t = interp(GB_PX, pq); osc[c].inc = K_GB / (2048u - t); osc[c].duty = in->duty & 3u; osc[c].vol = (uint8_t)vol; osc[c].pan = in->pan; break;
    case KD_GB_WAV:
        t = interp(GB_WX, pq); osc[c].inc = (K_GB / (2048u - t)) >> 1; osc[c].pan = in->pan;
        {   const inst_t *wi = &chan[c].in;                 /* the wave is read live: turn a knob while a note rings */
            uint32_t key = (1u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make(wi, wtab[c]); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        osc[c].wshift = vol >= 12u ? 0u : vol >= 8u ? 1u : vol >= 4u ? 2u : 8u;     /* the four volumes of the wave channel */
        break;
    case KD_PCE:                                        /* the HuC6280: 32 samples a cycle, f = clk / (32 N), the wave read live like the Game Boy's; the volume in 3 dB steps */
        t = interp(PCE_NT, pq); osc[c].inc = K_SMS / (t ? t : 1u); osc[c].pan = in->pan; osc[c].vol = (uint8_t)vol;
        {   const inst_t *wi = &chan[c].in;
            uint32_t key = (4u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make_n(wi, wtab[c], 31); wkey[c] = key; }        /* the PSG's wave is 5 bits */
            osc[c].wav = wtab[c];
        }
        break;
    case KD_VB_WAV:                                     /* the VSU: 32 samples of 6 bits, stereo volume; the wave is read live */
        osc[c].inc = interp32(VB_INC, pq); osc[c].pan = in->pan; osc[c].vol = (uint8_t)vol;
        {   const inst_t *wi = &chan[c].in;
            uint32_t key = (2u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make_n(wi, wtab[c], 63); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        break;
    case KD_TIA: {                                      /* a pattern of bits stepped round: tonal ones play the note as their cycle, the long noisy ones as their step rate */
        uint32_t w = chan[c].in.wave > 10u ? 10u : chan[c].in.wave, len = TIA_LEN[w];
        t = interp32(NOTE_INC, pq);
        osc[c].inc = len > 186u ? t / len * 32u : t; osc[c].duty = (uint8_t)w; osc[c].vol = (uint8_t)vol;
        break; }
    case KD_VRC6_PU: osc[c].inc = interp32(VRC6P_INC, pq); osc[c].duty = in->duty & 7u; osc[c].vol = (uint8_t)vol; break;
    case KD_VRC6_SAW: osc[c].inc = interp32(VRC6S_INC, pq); osc[c].vol = (uint8_t)vol; break;
    case KD_AY: {                                       /* the noise clock stops where the chip's does (clock / 16 = 110.8 kHz); it used to wrap round 2^32 for high notes with a big NOISE step */
        const uint32_t m = NZ_M[chan[c].in.wave & 15u], lim = 0xA1000000u;
        osc[c].inc = interp32(AY_INC, pq); osc[c].inc2 = osc[c].inc > lim / m ? lim : osc[c].inc * m; osc[c].duty = chan[c].in.wave & 15u; osc[c].vol = (uint8_t)vol; break; }
    case KD_POKEY: {                                    /* a distortion as a cycle of bits, like the TIA; the long noisy ones play the note as their step rate */
        uint32_t w = chan[c].in.wave > 7u ? 7u : chan[c].in.wave;
        t = interp32(NOTE_INC, pq);
        osc[c].inc = w == 3u ? t / POKEY_LEN[3] * 32u : w < 4u ? t : t * 16u;       /* the long 465 step cycle runs at 32 x the note, like the noises */ osc[c].duty = (uint8_t)w; osc[c].vol = (uint8_t)vol;
        break; }
    case KD_MSM: {                                      /* an organ: the voice's square at 16', 8', 4' and 2' (octaves off one counter), 5 1/3' and 2 2/3' (a fifth and an octave and a fifth above 8'), mixed */
        const uint32_t m = chan[c].in.wave & 15u, q = chan[c].in.pan & 3u, cnt = (m & 1u) + (m >> 1 & 1u) + (m >> 2 & 1u) + (m >> 3) + (q & 1u) + (q >> 1);
        t = interp32(NOTE_INC, pq);
        osc[c].inc = t >> 1; osc[c].inc2 = (t >> 1) * 3u; osc[c].duty = (uint8_t)(m | q << 4); osc[c].alg = (uint8_t)(256u / (cnt + 1u)); osc[c].vol = (uint8_t)vol;
        break; }
    case KD_ZX: osc[c].inc = interp32(NOTE_INC, pq); osc[c].duty = chan[c].in.duty & 7u; osc[c].vol = vol > 0u; break;       /* a pulse; the speaker is one bit, so no volume */
    case KD_SPK: osc[c].inc = interp32(SPK_INC, pq); osc[c].vol = vol > 0u; break;        /* the timer's divisor is a whole number; no volume: on or off */
    case KD_OPL: case KD_OPL1: {                        /* two operators: A (the ratio, with feedback) and C (the note); the OPL1 only has FM or both playing */
        const inst_t *wi = &chan[c].in;
        t = interp32(NOTE_INC, pq);
        osc[c].inc = t; osc[c].inc2 = (t >> 1) * OPL_RAT[wi->wave & 15u];
        osc[c].idx = fm_idx(wi, vc[c].age); osc[c].fbm = FM_FB[wi->wc & 15u]; osc[c].gain = (uint16_t)fm_gain(vol_q); osc[c].duty = 0u; osc[c].vol = (uint8_t)vol;
        osc[c].alg = chan[c].kind == KD_OPL1 ? (wi->duty ? 3u : 0u) : (uint8_t)(wi->duty & 3u);
        break; }
    case KD_SN77: {                                     /* a square, noise, or both; the slow oscillator swings the pitch (pan = how fast; the siren swings it twice as far) */
        static const uint8_t SLF[4] = {2, 3, 6, 12};
        const uint32_t m = chan[c].in.wave & 3u;
        osc[c].inc = interp32(NOTE_INC, pq); osc[c].inc2 = osc[c].inc * 8u; osc[c].inc3 = SLF[in->pan & 3u] * SN77_SLF;
        osc[c].duty = (uint8_t)m; osc[c].alg = (uint8_t)((in->pan & 3u) || m == 3u); osc[c].fb = (uint8_t)(m == 3u); osc[c].vol = (uint8_t)vol;
        break; }
    case KD_NGP: osc[c].inc = K_NGP / interp(NGP_N, pq); osc[c].vol = (uint8_t)vol; osc[c].pan = in->pan; break;
    case KD_MD: case KD_PC98_FM: case KD_NEO_FM: case KD_OPN_FM: case KD_OPM: case KD_OPL3: case KD_OPZ: {         /* four operators: 1 (the ratio, with feedback), 2 (the note), 3 (an octave up), 4 (the note; the OPM family detunes it a hair) */
        const inst_t *wi = &chan[c].in;
        const uint32_t k = chan[c].kind, opl3 = k == KD_OPL3, opz = k == KD_OPZ, det = k != KD_MD && !opl3 && !opz;
        t = interp32(NOTE_INC, pq);
        osc[c].inc = det ? t + (t >> 9) : t; osc[c].inc2 = (t >> 1) * (opl3 ? OPL_RAT : FM_RAT)[wi->wave & 15u]; osc[c].inc3 = t; osc[c].inc4 = t * 2u;
        osc[c].idx = fm_idx(wi, vc[c].age); osc[c].fbm = FM_FB[wi->wc & 15u]; osc[c].vol = (uint8_t)vol; osc[c].gain = (uint16_t)fm_gain(vol_q);
        osc[c].alg = (uint8_t)(opl3 ? 8u + (wi->duty & 3u) : wi->duty & 7u);
        osc[c].duty = opl3 ? wi->pan & 7u : opz ? (wi->pan & 7u) | 8u : 0u; osc[c].pan = opl3 || opz || k == KD_OPN_FM ? 1u : in->pan;
        break; }
    case KD_SID: sid_set(c, pq, vol); break;
    case KD_PC98_SSG: case KD_NEO_SSG: case KD_OPN_SSG: osc[c].inc = interp32(chan[c].kind == KD_NEO_SSG ? SSGN_INC : SSG_INC, pq); osc[c].inc2 = osc[c].inc > 0xB5000000u / NZ_M[chan[c].in.wave & 15u] ? 0xB5000000u : osc[c].inc * NZ_M[chan[c].in.wave & 15u]; osc[c].duty = chan[c].in.wave & 15u; osc[c].vol = (uint8_t)vol; break;       /* (the noise clock stops at clock / 16, it must not wrap) */
    case KD_SAA: {                                      /* the noise register steps at 15.6, 31.25 or 62.5 kHz (8 MHz clock), or on every half cycle of the tone (the lowest NOISE steps) */
        const uint32_t nz = chan[c].in.wave & 15u;
        osc[c].inc = interp32(SAA_INC, pq); osc[c].inc2 = nz <= 4u ? osc[c].inc << 1 : nz <= 8u ? 15625u * SN77_SLF : nz <= 11u ? 31250u * SN77_SLF : 62500u * SN77_SLF; osc[c].duty = (uint8_t)nz; osc[c].vol = (uint8_t)vol; osc[c].pan = in->pan; break; }
    case KD_VIC: osc[c].inc = interp32(VIC_INC, pq); osc[c].inc2 = osc[c].inc >= (1u << 27) ? 0xFFFFFFFFu : osc[c].inc < 3293750u ? 105400000u : osc[c].inc << 5;       /* the noise register that matches the note's soprano one: its shift register steps at 32 x the note, between 4.3 and 176 kHz */
        osc[c].duty = chan[c].in.wave & 1u; osc[c].vol = (uint8_t)vol; break;
    case KD_TED: osc[c].inc = interp32(TED_INC, pq); osc[c].inc2 = osc[c].inc * 2u; osc[c].duty = chan[c].in.wave & 1u; osc[c].vol = (uint8_t)vol; break;
    case KD_LYNX: {                                     /* the square plays the note; a short shift register cycle is a tone (the note is the cycle's rate), a long one is noise */
        const uint32_t w = chan[c].in.wave & 7u, len = LYNX_LEN[w];
        t = interp32(NOTE_INC, pq);
        if (w >= 5u) {                                  /* the long cycles are tuned like the short ones: the whole cycle is the note (the timer's 1 MHz caps it); inc2 = register steps per output frame, Q16 */
            if (t > LYNX_CAP[w - 5u]) t = LYNX_CAP[w - 5u];
            osc[c].inc2 = (uint32_t)(((uint64_t)(t * OS) * len) >> 16);
            if (!osc[c].inc2) osc[c].inc2 = 1u;
        } else if (w) t = t > 0xFFFFFFFFu / len ? 0xFFFFFFFFu : t * len;
        osc[c].inc = t; osc[c].duty = (uint8_t)w; osc[c].vol = (uint8_t)vol; osc[c].pan = in->pan;
        break; }
    case KD_WSG: case KD_N163: case KD_WS: case KD_C30:              /* a 4 bit wave, 32 steps, read live; the volume is linear */
        osc[c].inc = interp32(chan[c].kind == KD_WS ? WS_INC : NOTE_INC, pq); osc[c].vol = (uint8_t)vol; osc[c].pan = in->pan;
        {   const inst_t *wi = &chan[c].in;
            uint32_t key = (1u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make(wi, wtab[c]); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        break;
    case KD_MMC5: t = interp(NES_PT, pq); osc[c].inc = K_NES / (t + 1u); osc[c].duty = in->duty & 3u; osc[c].vol = (uint8_t)vol; break;
    case KD_FDS: case KD_SCC:                           /* FDS: a 6 bit wave, 64 steps like the chip's RAM; SCC: an 8 bit wave; both read live. The FDS also bends its phase by its modulator (pan = depth) */
        osc[c].inc = interp32(chan[c].kind == KD_FDS ? FDS_INC : SCC_INC, pq); osc[c].vol = (uint8_t)vol; osc[c].alg = chan[c].kind == KD_FDS ? chan[c].in.pan & 3u : 0u;
        {   const inst_t *wi = &chan[c].in;
            uint32_t fds = chan[c].kind == KD_FDS, key = ((fds ? 2u : 3u) << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { if (fds) wave_make_s(wi, wtab[c], 63, 64); else wave_make_n(wi, wtab[c], 255); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        break;
    case KD_OPLL: case KD_VRC7: {                       /* one of the 15 instruments: two operators with the chip's ROM values: multiples, wave bits, feedback, the modulator's level, key scale and envelope */
        const uint32_t n = (chan[c].in.wave > 14u ? 14u : chan[c].in.wave) + (chan[c].kind == KD_VRC7 ? 15u : 0u), age = vc[c].age, nt = pq >> 8, kl = KSL_TMP[nt > 127u ? 127u : nt];
        uint32_t u = OPLL_P[n].kslm ? kl >> (3u - OPLL_P[n].kslm) : 0u, uc = OPLL_P[n].kslc ? kl >> (3u - OPLL_P[n].kslc) : 0u;       /* attenuations here are in the chip's 0.375 dB steps */
        const uint32_t kfm = opll_kf(vc[c].note, OPLL_P[n].fl >> 4 & 1u);          /* the modulator's key scaled rates */
        if (OPLL_P[n].dru) {                            /* the modulator falls from its level at its decay rate to the sustain level, then holds, or goes on at the release rate */
            const uint32_t dru = (uint32_t)OPLL_P[n].dru * kfm >> 8, tsl = (uint32_t)OPLL_P[n].slu * 60u / (dru ? dru : 1u);
            uint32_t d = age <= tsl ? dru * age / 60u : OPLL_P[n].eg ? OPLL_P[n].slu : OPLL_P[n].slu + ((uint32_t)OPLL_P[n].rru * kfm >> 8) * (age - tsl) / 60u;
            u += d * 2u;
        }
        if (OPLL_P[n].mta) { const uint32_t ma = ((uint32_t)OPLL_P[n].mta << 8) / kfm + 1u; if (age < ma) u += 254u * (ma - age) / ma; }          /* a slow modulator attack (the VRC7 bells) */
        {   const uint32_t fl = OPLL_P[n].fl, tp = tre_ph & 0xFFFFu, am = ((tp < 32768u ? tp : 65535u - tp) * 13u) >> 16;       /* the chip's tremolo (3.7 Hz, 4.9 dB) and vibrato (6.1 Hz, 11 cents), per operator flag */
            const int32_t dp = ((int32_t)SIN_T[(vib_ph >> 6) & 1023u] * 28) >> 14;
            if (fl & 1u) u += am;
            if (fl & 4u) uc += am;
            int32_t qc = (int32_t)pq + (fl & 8u ? dp : 0), qm = (int32_t)pq + (fl & 2u ? dp : 0);
            qc = qc < 0 ? 0 : qc > (127 << 8) ? 127 << 8 : qc; qm = qm < 0 ? 0 : qm > (127 << 8) ? 127 << 8 : qm;
            t = interp32(NOTE_INC, (uint32_t)qc);
            osc[c].inc = (t >> 1) * OPLL_P[n].cm;
            if (qm != qc) t = interp32(NOTE_INC, (uint32_t)qm);
            osc[c].inc2 = (t >> 1) * OPLL_P[n].mm;
        }
        osc[c].idx = (OPLL_P[n].idx >> 6) * (att375(u) >> 6) >> 4; osc[c].fbm = OPLL_FB[OPLL_P[n].fb & 7u];
        osc[c].alg = OPLL_P[n].ws & 1u; osc[c].duty = OPLL_P[n].ws >> 1 & 1u; osc[c].gain = (uint16_t)fm_gain_att(vol_q, uc); osc[c].wshift = 1; osc[c].vol = (uint8_t)vol;
        break; }
    case KD_S5B: {                                      /* PERIOD 0 follows the note; 1..15 is the chip's noise period register (clock / 16 P), whatever the note */
        const uint32_t np = S5B_NP[chan[c].in.wf & 15u];
        osc[c].inc = interp32(S5B_INC, pq);
        osc[c].inc2 = np ? 0xA2000000u / np : osc[c].inc > 0xA2000000u / NZ_M[chan[c].in.wave & 15u] ? 0xA2000000u : osc[c].inc * NZ_M[chan[c].in.wave & 15u];
        osc[c].inc3 = osc[c].inc; osc[c].duty = chan[c].in.wave & 15u; osc[c].alg = chan[c].in.pan & 7u; osc[c].vol = (uint8_t)vol; break;
    }
    default: osc[c].inc = K_SMS / interp(SMS_N, pq); osc[c].vol = (uint8_t)vol; break;
    }
}

/* the voices of a chip that sound now */
static uint32_t active_of(uint32_t chip, uint8_t *list, uint32_t drum)
{
    uint32_t c, n = 0;
    for (c = 0; c < chan_n; c++)
        if (KIND_CHIP[chan[c].kind] == chip && (vc[c].stage != ST_IDLE || (chan[c].kind == KD_NES_TRI && osc[c].duty)) && (uint32_t)IS_DRUM(chan[c].kind) == drum) list[n++] = (uint8_t)c;       /* (the NES triangle plays on to its trough) */
    return n;
}
static int32_t nes_mix(int32_t psum, int32_t tsum)      /* the APU's two nonlinear mixers; past their tables the last slope goes on */
{
    int32_t p = psum <= 30 ? NES_PULSE[psum] : NES_PULSE[30] + (psum - 30) * (NES_PULSE[30] - NES_PULSE[29]);
    int32_t t = tsum <= 15 ? NES_TND[3 * tsum] : NES_TND[45] + (tsum - 15) * (NES_TND[45] - NES_TND[42]);
    return p + t;
}
static void r_nes(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0, m;
        for (s = 0; s < OS; s++) {
            int32_t psum = 0, tsum = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], st;
                osc[c].ph += osc[c].inc;
                if (chan[c].kind == KD_NES_PU) { if ((NES_DUTY[osc[c].duty] >> (osc[c].ph >> 29)) & 1u) psum += osc[c].vol; }
                else if (osc[c].vol || osc[c].duty) {        /* the triangle starts and stops at its trough: no step, no pop (the real one holds its level, which the high-pass then bleeds) */
                    int32_t lv;
                    if (!osc[c].duty) { osc[c].duty = 1; osc[c].ph = 15u << 27; }
                    st = osc[c].ph >> 27; lv = (int32_t)(st < 16u ? 15u - st : st - 16u);
                    if (!osc[c].vol && lv <= 1) osc[c].duty = 0; else tsum += lv;
                }
            }
            acc += nes_mix(psum, tsum);
        }
        m = acc >> 2;
        out[2u * i] += m; out[2u * i + 1u] += m;
    }
}
static void r_gb(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t v;
                osc[c].ph += osc[c].inc;
                if (chan[c].kind == KD_GB_PU) v = (GB_DUTY[osc[c].duty] >> (osc[c].ph >> 29)) & 1u ? osc[c].vol : 0;
                else v = osc[c].wshift < 8u ? osc[c].wav[osc[c].ph >> 27] >> osc[c].wshift : 0;
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l * 150;      /* the sum of 4 steps x 150 = the average x 600 */
        out[2u * i + 1u] += r * 150;
    }
}
static const uint16_t PCE_V[16] = {0, 13, 18, 25, 36, 51, 71, 101, 143, 201, 285, 402, 568, 802, 1133, 1600};       /* 3 dB a step (PCE_AMP, shared with the FM and the kits, is 8 bit: its lowest steps repeat) */
static hpf_t hp_pc[2];                          /* the PC Engine's DC blocker: its wave editor can make a wave with a mean (like the other wave chips', whose post() has one) */
static void r_pce(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t v;
                osc[c].ph += osc[c].inc;
                v = (((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 31) * (int32_t)PCE_V[osc[c].vol & 15u]) >> 5;
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += hpf(&hp_pc[0], 2 * l + (l >> 1), B_DC_HP14); out[2u * i + 1u] += hpf(&hp_pc[1], 2 * r + (r >> 1), B_DC_HP14);
    }
}
static const uint8_t TIA_DAC[16] = {0, 10, 19, 27, 35, 43, 50, 57, 63, 69, 75, 80, 86, 91, 95, 100};       /* the TIA's volume DAC is a resistor network: level = 3 v / (30 + v), not linear (Stella's mixing table) */
static void r_tia(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], x = (osc[c].ph >> 16) * TIA_LEN[osc[c].duty] >> 16;
                int32_t a = TIA_DAC[osc[c].vol & 15u];
                osc[c].ph += osc[c].inc;
                acc += TIA_BITS[osc[c].duty][x >> 3] >> (x & 7u) & 1u ? a : -a;
            }
        out[2u * i] += acc * 9; out[2u * i + 1u] += acc * 9;
    }
}
static void r_vrc6(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                if (chan[c].kind == KD_VRC6_PU) { if ((osc[c].ph >> 28) <= osc[c].duty) acc += (int32_t)osc[c].vol; }
                else acc += (int32_t)((((osc[c].ph >> 16) * 7u >> 16) * ((osc[c].vol * 42u) / 15u) & 255u) >> 3);        /* 7 steps of the rate A up to 42, an 8 bit accumulator read from its top 5 bits: the saw peaks at 31, twice the pulse */
            }
        out[2u * i] += acc * 144; out[2u * i + 1u] += acc * 144;
    }
}
static void r_ay_t(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na, const uint8_t *amp8, const uint16_t *amp16, uint32_t g)       /* the AY's tone and noise: 0 tone, 1..14 the tone gated by noise (AND, as the chip's mixer does), 15 noise alone */
{
    uint32_t i, s, k;
    int32_t lv[MAXCH];
    for (k = 0; k < na; k++) lv[k] = (amp16 ? (int32_t)amp16[osc[act[k]].vol & 15u] : 16 * (int32_t)amp8[osc[act[k]].vol & 15u]) * (int32_t)g >> 4;       /* each voice's level, once a block */
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], nz = osc[c].duty, old = osc[c].ph2;
                int32_t t = osc[c].ph >> 31 ? 1 : 0, b, v;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 3) & 1u) << 16));
                if (!osc[c].rng) osc[c].rng = 1u;
                b = osc[c].rng & 1u ? 1 : 0;
                v = nz == 0u ? t : nz == 15u ? b : t & b;
                acc += v ? lv[k] : -lv[k];
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_ay(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na) { r_ay_t(out, n, act, na, 0, AY_AMP, 16u); }
#define FMOS 2u
#define FMS(ph, m) SIN_T[(((ph) + (m)) >> 22) & 1023u]       /* a sine at a phase pushed by a modulation */
#define FMM(x, i) ((uint32_t)(x) * (i) << 2)                  /* a modulator's output times the depth, as a phase */
static void r_vb(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t v;
                osc[c].ph += osc[c].inc;
                v = ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 63) * (int32_t)osc[c].vol;       /* 6 bit wave, 4 bit volume, centred */
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l + (l >> 1); out[2u * i + 1u] += r + (r >> 1);
    }
}
static void r_pokey(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* 0..3 run from bit cycles; 4..7 are shift registers: 17 bit, that gated by the 5 bit poly, 9 bit, that gated */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], w = osc[c].duty, b;
                int32_t a = TIA_AMP[osc[c].vol & 15u];
                if (w < 4u) {
                    uint32_t x = (osc[c].ph >> 16) * POKEY_LEN[w] >> 16;
                    osc[c].ph += osc[c].inc;
                    b = POKEY_BITS[w][x >> 3] >> (x & 7u) & 1u;
                } else {
                    uint32_t old = osc[c].ph;
                    osc[c].ph += osc[c].inc;
                    if (osc[c].ph < old) {                                          /* a step of the divider */
                        uint32_t go = 1;
                        if (w & 1u) { osc[c].ph2 = osc[c].ph2 % 31u; go = POKEY_P5[osc[c].ph2]; osc[c].ph2++; }
                        if (go) {
                            if (!osc[c].rng) osc[c].rng = w >= 6u ? 0x1FFu : 0x1FFFFu;
                            osc[c].rng = w >= 6u ? (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 4) & 1u) << 8)) : (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 5) & 1u) << 16));
                            osc[c].y2 = (int32_t)(osc[c].rng & 1u);
                        }
                    }
                    b = (uint32_t)osc[c].y2;
                }
                acc += b ? a : -a;
            }
        out[2u * i] += acc * 9; out[2u * i + 1u] += acc * 9;
    }
}
static void r_spk(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* one bit: every voice shares it (OR) */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++) {
            uint32_t bit = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                if (osc[c].vol && osc[c].ph >> 31) bit = 1;
            }
            acc += bit ? 2400 : -2400;
        }
        out[2u * i] += acc >> 2; out[2u * i + 1u] += acc >> 2;
    }
}
static int32_t opl_wave(int32_t x, uint32_t type) { return type == 1u ? (x > 0 ? x : 0) : type == 2u ? (x < 0 ? -x : x) : x; }       /* the OPL2's sine, half sine, absolute sine */
static void r_opl(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* two operators: A modulates C (alg 0..2: C's wave), or both play (3); the feedback carries A's level, like the chips */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < FMOS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t a, o;
                osc[c].ph += osc[c].inc << 1; osc[c].ph2 += osc[c].inc2 << 1;
                a = FMS(osc[c].ph2, (uint32_t)((osc[c].y1 + osc[c].y2) >> 1) * osc[c].fbm << 2);
                if (osc[c].duty && a < 0) a = 0;                                     /* a half sine modulator */
                osc[c].y2 = osc[c].y1; osc[c].y1 = osc[c].alg == 3u ? a : (a * (int32_t)(osc[c].idx >> 4)) >> 14;
                if (osc[c].alg < 3u) o = opl_wave(FMS(osc[c].ph, FMM(a, osc[c].idx)), osc[c].alg);
                else o = (a + FMS(osc[c].ph, 0u)) >> 1;
                { const int32_t d = (int32_t)osc[c].gain - (int32_t)osc[c].rng; osc[c].rng += (uint32_t)(d > 0 ? (d + 31) >> 5 : d >> 5); }      /* rng is free here: the level glides to its target */
                { const int32_t x = (o * (int32_t)osc[c].rng) >> 16; acc += osc[c].wshift ? (x * 23) >> 4 : x; }       /* (the OPLL's voices are 3 dB up) */
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
#define N163_W4(c) ((((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 15) * (int32_t)osc[c].vol * 77) / 4)       /* one voice's DAC level (a 163 voice at full level: about 12 dB over an APU pulse, nesdev +11, NSFPlay / Mesen 6x) */
static void r_wave4(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na, uint32_t n163)       /* the Namco chips: 4 bit waves, linear volume. The 163 has one DAC that takes its voices in turn, 15 CPU clocks each: a voice is 1/N of the time (so 1/N of the level, N = the voices sounding) and is heard 119 kHz / N times a second, the way the chip's output is a stream of short slices (images of that rate fold into the band) */
{
    static uint32_t pos;                                /* where the DAC is: slots, Q16 */
    const uint32_t nn = na > 8u ? 8u : na ? na : 1u, step = 44331u * na / nn, lim = na << 16;       /* one oscillator step (1/176.4 kHz) = 0.6764 slots of 15 clocks */
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++) {
            int32_t v = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                if (!n163) v += ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 15) * (int32_t)osc[c].vol * 7;
            }
            if (!n163) acc += v;
            else if (na < 2u) acc += N163_W4(act[0]);
            else {
                uint32_t p1, e;
                while (pos >= lim) pos -= lim;
                p1 = pos + step; e = ((pos >> 16) + 1u) << 16;
                if (p1 <= e) acc += N163_W4(act[pos >> 16]);       /* one voice's slot covers the whole step */
                else {                                  /* the step crosses into the next slot: each voice by its share of the step */
                    const uint32_t k1 = (pos >> 16) + 1u == na ? 0u : (pos >> 16) + 1u;
                    acc += (N163_W4(act[pos >> 16]) * (int32_t)(e - pos) + N163_W4(act[k1]) * (int32_t)(p1 - e)) / (int32_t)step;
                }
                pos = p1;
            }
        }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static int32_t fm4_wave(uint32_t p, uint32_t w)       /* an operator's wave at a phase: sine, or the OPL3's eight (w 0..7); the OPZ's (sine, sine squared, half, half squared, alternating, alternating squared, camel, camel squared) are OPZ_T tables */
{
    int32_t s = SIN_T[(p >> 22) & 1023u], f;
    if (!w) return s;
    switch (w) {
    case 1: return s > 0 ? s : 0;
    case 2: return s < 0 ? -s : s;
    case 3: return (p >> 30) & 1u ? 0 : (s < 0 ? -s : s);
    case 4: return p >> 31 ? 0 : SIN_T[(p >> 21) & 1023u];
    case 5: if (p >> 31) return 0; s = SIN_T[(p >> 21) & 1023u]; return s < 0 ? -s : s;
    case 6: return s >= 0 ? 16384 : -16384;
    default: {                                          /* derived square: from full level it halves every 32 of 512 steps, mirrored and negative in the second half */
        int32_t x = (int32_t)((p >> 22) & 511u), pos = p >> 31 ? 511 - x : x; f = OPL3_SAW[pos & 31] >> (pos >> 5);
        return p >> 31 ? -f : f;
    }
    }
}
static void r_fm4(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na, uint32_t mode)       /* the YM2151 / OPN / TX81Z (8 algorithms), the YM2612 (the same, through its 9 bit DAC), the OPL3 (4 connections); mode 0 stereo, 1 mono, 2 YM2612. One step a sample, two while the modulation is deep (it would alias) */
{
    static const uint8_t NORM[12] = {255, 255, 255, 255, 128, 85, 85, 64, 166, 165, 180, 100};         /* one over the number of carriers (OPM family); the OPL3's four follow the chip's own levels, measured */
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (k = 0; k < na; k++) {
            uint32_t c = act[k], alg = osc[c].alg, ix = osc[c].idx, w = osc[c].duty, ns = ix > FM_IDX[10] ? 2u : 1u, sh = 3u - ns;
            const int16_t *wt = !w ? SIN_T : (w & 8u) ? ((w & 7u) ? OPZ_T + ((w & 7u) - 1u) * 1024u : SIN_T) : 0;       /* the OPZ's waves are tables like the sine; the OPL3's are computed */
            int32_t v = 0, g;
            { const int32_t d = (int32_t)osc[c].gain - (int32_t)osc[c].rng; osc[c].rng += (uint32_t)(d > 0 ? (d + 31) >> 5 : d >> 5); } g = (int32_t)osc[c].rng;      /* rng is free here: the level glides to its target (no steps at the 60 Hz tick) */
#define OPW(p) (wt ? (int32_t)wt[((p) >> 22) & 1023u] : fm4_wave((p), w))
#define OPX(ph, x) OPW((ph) + FMM(x, ix))
            for (s = 0; s < ns; s++) {
                int32_t a, o, o2, o3;
                osc[c].ph += osc[c].inc << sh; osc[c].ph2 += osc[c].inc2 << sh; osc[c].ph3 += osc[c].inc3 << sh; osc[c].ph4 += osc[c].inc4 << sh;
                a = OPW(osc[c].ph2 + ((uint32_t)((osc[c].y1 + osc[c].y2) >> 1) * osc[c].fbm << 2));
                osc[c].y2 = osc[c].y1;
                osc[c].y1 = (alg == 7u || alg == 9u || alg == 11u) ? a : (a * (int32_t)(ix >> 4)) >> 14;     /* the feedback is the operator's output with its level: a modulator's TL shrinks it */
                switch (alg) {
                case 0: case 8: o = OPX(osc[c].ph, OPX(osc[c].ph4, OPX(osc[c].ph3, a))); break;                                       /* 1 > 2 > 3 > 4 */
                case 1: o = OPX(osc[c].ph, OPX(osc[c].ph4, a + OPX(osc[c].ph3, 0u))); break;                                          /* (1 + 2) > 3 > 4 */
                case 2: o = OPX(osc[c].ph, a + OPX(osc[c].ph4, OPX(osc[c].ph3, 0u))); break;                                          /* (1 + (2 > 3)) > 4 */
                case 3: o = OPX(osc[c].ph, OPX(osc[c].ph3, a) + OPX(osc[c].ph4, 0u)); break;                                          /* ((1 > 2) + 3) > 4 */
                case 4: case 10: o = OPX(osc[c].ph3, a) + OPX(osc[c].ph, OPX(osc[c].ph4, 0u)); break;                                 /* (1 > 2) + (3 > 4) */
                case 5: o = OPX(osc[c].ph3, a) + OPX(osc[c].ph4, a) + OPX(osc[c].ph, a); break;                                       /* 1 > 2, 3, 4 */
                case 6: o = OPX(osc[c].ph3, a) + OPX(osc[c].ph4, 0u) + OPX(osc[c].ph, 0u); break;                                     /* 1 > 2, then 3 and 4 */
                case 7: o = a + OPX(osc[c].ph3, 0u) + OPX(osc[c].ph4, 0u) + OPX(osc[c].ph, 0u); break;                                /* all four */
                case 9: o = a + OPX(osc[c].ph, OPX(osc[c].ph4, OPX(osc[c].ph3, 0u))); break;                                          /* 1 + (2 > 3 > 4) */
                default: o2 = OPX(osc[c].ph3, 0u); o3 = OPX(osc[c].ph4, o2); o = a + o3 + OPX(osc[c].ph, 0u); break;                  /* 1 + (2 > 3) + 4 */
                }
                o = (o * NORM[alg < 12u ? alg : 0u] >> 8) * g >> 16;
                v += o;
            }
#undef OPX
#undef OPW
            if (mode == 2u) {                                   /* the YM2612's 9 bit DAC: full scale is 255 steps; it truncates, clips, and leaves a gap of 7 steps at zero (+4 / -3, the ladder effect) */
                int32_t d = (ns == 1u ? v : v / 2) * 41 >> 10;
                if (d > 255) d = 255; if (d < -256) d = -256;
                v = (d ? d * 25 + (d > 0 ? 100 : -75) : 0) * 3;
            } else v = ns == 1u ? v * 3 : v * 3 / 2;
            if (mode == 1u || osc[c].pan != 2u) l += v;
            if (mode == 1u || osc[c].pan != 0u) r += v;
        }
        out[2u * i] += l; out[2u * i + 1u] += r;
    }
}
static void r_ws(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the WonderSwan: 4 bit waves, a volume and a side each; the mix is sampled at 24 kHz */
{
    static uint32_t cnt;
    static int32_t hl, hr;
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t al = 0, ar = 0;
        for (s = 0; s < OS; s++) {
            int32_t l = 0, r = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t v;
                osc[c].ph += osc[c].inc;
                v = ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 15) * (int32_t)osc[c].vol * 7;
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
            cnt += 24000u; if (cnt >= FS * OS) { cnt -= FS * OS; hl = l; hr = r; }
            al += hl; ar += hr;
        }
        out[2u * i] += al; out[2u * i + 1u] += ar;
    }
}
static void r_saa(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the Sam Coupe's chip: a square gated by the noise, noise (15 = noise alone), linear 4 bit levels, a side each */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], nz = osc[c].duty, old = osc[c].ph2;
                int32_t tb = osc[c].ph >> 31 ? 1 : -1, nb, v, a = (int32_t)(osc[c].vol & 15u) * 80;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 7) & 1u) << 17));      /* 18 bit */
                if (!osc[c].rng) osc[c].rng = 1u;
                nb = osc[c].rng & 1u ? 1 : -1;
                v = nz == 0u ? tb * a : nz == 15u ? nb * a : (tb < 0 ? -a : nb > 0 ? a : 0);       /* tone and noise: a low tone is low, a high tone follows the noise bit (up or middle) */
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l; out[2u * i + 1u] += r;
    }
}
static void r_vic(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the VIC-20: a square, or the noise voice: a 16 bit shift register whose rising edges step an 8 bit ring counter; that counter's bit is the output, so the noise is a jittery square about 1/64 of the register's rate */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], old = osc[c].ph2, b;
                int32_t a = (int32_t)(osc[c].vol & 15u) * 80;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].ph2 < old && osc[c].duty) {                  /* (the register only runs for the noise voice) */
                    uint32_t r = osc[c].rng ? osc[c].rng : 0xFFFFu;
                    if ((r & 1u) && !osc[c].y2) osc[c].y1 = (int32_t)((((uint32_t)osc[c].y1 << 1) | (~(uint32_t)osc[c].y1 >> 7 & 1u)) & 255u);
                    osc[c].y2 = (int32_t)(r & 1u);
                    osc[c].rng = (r << 1 | ((r >> 3 ^ r >> 12 ^ r >> 14 ^ r >> 15) & 1u)) & 0xFFFFu;
                }
                b = osc[c].duty ? ((uint32_t)osc[c].y1 & 1u) : (osc[c].ph >> 31);
                acc += b ? a : -a;
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_ted(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the Plus/4: a square or an 8 bit shift register noise, 9 levels */
{
    uint32_t i, s, k;
    int32_t av[MAXCH];
    for (k = 0; k < na; k++) {
        const uint32_t c = act[k], q = osc[c].vol ? 1u + ((osc[c].vol - 1u) * 7u + 7u) / 14u : 0u;       /* 9 levels 0..8, knob 1 is the quietest step */
        av[c] = q ? (int32_t)(((q << 10) - 438u) * 79u >> 9) : 0;       /* the chip's steps: 1024 q - 438 */
    }
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], old = osc[c].ph2;
                int32_t a = av[c], b;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng << 1 | (((osc[c].rng >> 7) ^ (osc[c].rng >> 5) ^ (osc[c].rng >> 4) ^ (osc[c].rng >> 3)) & 1u)) & 255u;
                if (!osc[c].rng) osc[c].rng = 1u;
                b = osc[c].duty ? (osc[c].rng & 1u) : (osc[c].ph >> 31);
                acc += b ? a : -a;
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static uint32_t lynx_cum(const uint16_t *cum, uint32_t len, uint32_t ph)       /* the ones in a cycle's first ph (of 2^32) of it, Q16 */
{
    const uint64_t x = (uint64_t)ph * len;
    const uint32_t i = (uint32_t)(x >> 32), f = (uint32_t)x >> 16;
    return ((uint32_t)cum[i] << 16) + (uint32_t)(cum[i + 1u] - cum[i]) * f;
}
static void r_lynx(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* Mikey: a 12 bit shift register with chosen taps, or a square; a side each */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (k = 0; k < na; k++) {                      /* the cycles of 63, 126 and 4095 steps: a frame's average of the cycle's bits (a running count, two lookups), the way the chip's output is integrated */
            uint32_t c = act[k], w = osc[c].duty, p0, p1, len, sum;
            const uint16_t *cum;
            int32_t v;
            if (w < 5u) continue;
            len = LYNX_LEN[w]; cum = LYNX_CUM + (w == 5u ? 0u : w == 6u ? 64u : 191u);
            p0 = osc[c].ph; p1 = p0 + osc[c].inc * OS; osc[c].ph = p1;
            sum = lynx_cum(cum, len, p1) - lynx_cum(cum, len, p0) + (p1 < p0 ? (uint32_t)cum[len] << 16 : 0u);
            v = (int32_t)(osc[c].vol & 15u) * 80;
            v = v * (int32_t)OS * ((int32_t)((sum << 8) / osc[c].inc2) * 2 - 256) / 256;       /* (the other waves add their level on each of the OS steps) */
            if (osc[c].pan != 2u) l += v;
            if (osc[c].pan != 0u) r += v;
        }
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], old = osc[c].ph, b;
                int32_t v;
                if (osc[c].duty >= 5u) continue;
                osc[c].ph += osc[c].inc;
                if (!osc[c].rng) osc[c].rng = 0xFFFu;
                if (osc[c].duty) {
                    if (osc[c].ph < old) {
                        uint32_t x = osc[c].rng & LYNX_MASK[osc[c].duty];
                        x ^= x >> 8; x ^= x >> 4; x ^= x >> 2; x ^= x >> 1;
                        osc[c].rng = (osc[c].rng << 1 | (x & 1u)) & 0xFFFu;
                    }
                    b = osc[c].rng & 1u;
                } else b = osc[c].ph >> 31;
                v = (int32_t)(osc[c].vol & 15u) * 80;
                if (!b) v = -v;
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l; out[2u * i + 1u] += r;
    }
}
static void r_two(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na, uint32_t kfm)       /* a chip with FM and an AY-style SSG: each kind through its own renderer */
{
    uint8_t a[MAXCH], b[MAXCH];
    uint32_t k, nf = 0, ns = 0;
    for (k = 0; k < na; k++) { if (chan[act[k]].kind == kfm) a[nf++] = act[k]; else b[ns++] = act[k]; }
    if (nf) r_fm4(out, n, a, nf, 0);
    if (ns) r_ay_t(out, n, b, ns, 0, YM_AMP, 25u);          /* the SSG is a YM2149: 3 dB a step; 4 dB up, to sit with the AY */
}
static void r_ngp(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the Neo Geo Pocket's SN76489 twin: tones with a side each */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t v;
                osc[c].ph += osc[c].inc;
                v = osc[c].ph >> 31 ? SMS_AMP[osc[c].vol] : -SMS_AMP[osc[c].vol];
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l >> 2; out[2u * i + 1u] += r >> 2;
    }
}
static void r_sn77(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the SN76477: a square (its pitch swung by the slow oscillator), noise (through a filter), or both ANDed, as its mixer does */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], old = osc[c].ph2, m = osc[c].duty;
                int32_t a = (int32_t)(osc[c].vol & 15u) * 64, step = (int32_t)osc[c].inc, v;
                if (osc[c].alg) {
                    uint32_t tp = osc[c].ph3 >> 16;
                    int32_t tri = (int32_t)(tp < 32768u ? tp : 65535u - tp) - 16384;     /* -16384 .. 16383 */
                    step += ((int32_t)(osc[c].inc >> 15) * tri) * (osc[c].fb ? 2 : 1);
                    osc[c].ph3 += osc[c].inc3;
                }
                osc[c].ph += (uint32_t)step; osc[c].ph2 += osc[c].inc2;
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 3) & 1u) << 30));          /* 31 bit, like the chip's */
                if (!osc[c].rng) osc[c].rng = 0x7FFFFFFFu;
                if (m == 2u) { osc[c].y1 += (((osc[c].rng & 1u ? a : -a) - osc[c].y1) * 5) >> 4; v = osc[c].y1; }       /* the noise goes through an RC filter */
                else if (m == 1u) v = (osc[c].ph >> 31) && (osc[c].rng & 1u) ? a : -a;
                else v = osc[c].ph >> 31 ? a : -a;
                acc += v;
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_c30(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* Namco System 1: 4 bit waves, a side each */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t v;
                osc[c].ph += osc[c].inc;
                v = ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 15) * (int32_t)osc[c].vol * 7;
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l; out[2u * i + 1u] += r;
    }
}
static void r_msm(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the OKI organ chip: six footages of one note, switched on or off, summed */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], m = osc[c].duty;
                int32_t sum = 0, a = (int32_t)(osc[c].vol & 15u) * 80;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (m & 1u) sum += osc[c].ph >> 31 ? 1 : -1;
                if (m & 2u) sum += osc[c].ph >> 30 & 1u ? 1 : -1;
                if (m & 4u) sum += osc[c].ph >> 29 & 1u ? 1 : -1;
                if (m & 8u) sum += osc[c].ph >> 28 & 1u ? 1 : -1;
                if (m & 16u) sum += osc[c].ph2 >> 31 ? 1 : -1;
                if (m & 32u) sum += osc[c].ph2 >> 30 & 1u ? 1 : -1;
                acc += (sum * a * (int32_t)osc[c].alg) >> 7;
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_zx(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the Spectrum's beeper: every voice is a pulse and all of them share one bit (OR), so chords crunch */
{
    static const uint8_t THR[8] = {4, 8, 16, 32, 64, 96, 128, 160};
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++) {
            uint32_t bit = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                if (osc[c].vol && (osc[c].ph >> 24) < THR[osc[c].duty & 7u]) bit = 1;
            }
            acc += bit ? 1040 : -1040;
        }
        out[2u * i] += acc >> 1; out[2u * i + 1u] += acc >> 1;
    }
}
static void r_mmc5(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the cartridge's two extra pulses, the NES's own pulse mixer, no triangle */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++) {
            int32_t psum = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                if ((NES_DUTY[osc[c].duty] >> (osc[c].ph >> 29)) & 1u) psum += osc[c].vol;
            }
            acc += nes_mix(psum, 0);
        }
        acc >>= 2;
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_fds(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* a 6 bit wave; the modulator (a sine at the note) pushes the wave's phase */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], ph;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc;
                ph = osc[c].ph + (osc[c].alg ? (uint32_t)((int32_t)SIN_T[osc[c].ph2 >> 22] * (1 << (13u + osc[c].alg))) : 0u);
                acc += ((int32_t)osc[c].wav[ph >> 26] * 2 - 63) * (int32_t)osc[c].vol;
            }
        out[2u * i] += acc * 3; out[2u * i + 1u] += acc * 3;
    }
}
static void r_scc(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* an 8 bit wave, 32 steps, a 4 bit volume */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                acc += ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 255) * (int32_t)osc[c].vol * 7 >> 4;
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static const uint16_t YM32[32] = {0, 9, 11, 13, 15, 18, 21, 25, 30, 36, 43, 51, 60, 71, 85, 101, 120, 143, 169, 201, 239, 285, 338, 402, 478, 568, 675, 802, 953, 1133, 1346, 1600};       /* the 5B's 32 envelope levels, 1.5 dB a step (the 16 volumes are the odd ones); same scale as YM_AMP */
static void r_s5b(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the AY's tone and noise, and the buzz: its envelope run as a wave at the note's pitch, 32 steps like the chip's */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], nz = osc[c].duty, old = osc[c].ph2, lv = osc[c].vol & 15u, amp;
                int32_t t = osc[c].alg ? 1 : osc[c].ph >> 31 ? 1 : 0, b, v;          /* the buzz is the envelope alone: the tone is switched off in the mixer, as the chip's buzz basses do */
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].alg) {
                    uint32_t e;
                    if (osc[c].alg == 3u) { e = osc[c].ph3 >> 26; if (e > 31u) e = 63u - e; }         /* the triangle: 32 steps up, 32 down */
                    else if (osc[c].alg < 3u) { e = osc[c].ph3 >> 27; if (osc[c].alg == 1u) e = 31u - e; }              /* the saws: 32 steps down or up */
                    else {                                                              /* once, a pluck: 4 down and silent, 5 up and high, 6 down and high, 7 up and silent (the chip's shapes 9, 13, 11, 15) */
                        const uint32_t p = osc[c].ph3, end = p == 0xFFFFFFFFu;
                        e = p >> 27;
                        osc[c].ph3 = end || p + 81000u < p ? 0xFFFFFFFFu : p + 81000u;       /* about 0.3 s, whatever the note */
                        e = osc[c].alg == 4u ? (end ? 0u : 31u - e) : osc[c].alg == 5u ? (end ? 31u : e) : osc[c].alg == 6u ? (end ? 31u : 31u - e) : (end ? 0u : e);
                    }
                    if (osc[c].alg < 4u) osc[c].ph3 += osc[c].inc3;
                    amp = YM32[e * lv / 15u];
                } else amp = YM_AMP[lv];
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 3) & 1u) << 16));
                if (!osc[c].rng) osc[c].rng = 1u;
                b = osc[c].rng & 1u ? 1 : 0;
                v = nz == 0u ? t : nz == 15u ? b : t & b;
                if (v) acc += (int32_t)amp * 27 / 8;                                     /* the 5B is a YM2149: the output is 0 or the level; a full tone is 0.36 of the mixer scale (nesdev NSFe: 5B volume 12 = -1.3 dB re an APU pulse) */
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_sms(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0, m;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                acc += osc[c].ph >> 31 ? SMS_AMP[osc[c].vol] : -SMS_AMP[osc[c].vol];
            }
        m = acc >> 2;
        out[2u * i] += m; out[2u * i + 1u] += m;
    }
}

/* ---- drums: a channel is a whole kit, the key picks the drum (its pitch class), up to four hits ring at once.
 * A hit is a tone with a pitch that falls, noise through a filter, an envelope each (exponential, per sample), a 1 ms attack so it never pops.
 * The kit says what the tone and noise are made of: the NES (triangle, 15 / 7 step noise, 4 bit), the Game Boy (pulse, 4 bit), Mega Drive FM (sines, six square metal),
 * the OPLL (the Yamaha XOR of fast squares), the Atari TIA (pulse, 5 / 4 step poly counters). */
typedef struct { uint16_t f0, f1, td, nd, cut; uint8_t pd, ta, na, filt, metal, burst; } drum_t;
static const drum_t DRUM[13] = {              /* by pitch class: C kick, C# kick 2, D snare, D# clap, E tom, F tom, F# hat, G tom, G# open hat, A cowbell, A# crash, B rim */
    {160,  48, 150,  10, 3000, 28, 100,   6, 2, 0, 0}, {120,  40, 220,   0,    0, 20, 100,   0, 0, 0, 0},
    {220, 170,  90, 150,  900, 18,  55,  90, 2, 0, 0}, {  0,   0,   0, 130, 1100,  0,   0, 100, 2, 0, 1},
    {130,  85, 260,  20, 3000, 60, 100,   4, 2, 0, 0}, {180, 120, 220,  20, 3000, 55, 100,   4, 2, 0, 0},
    {  0,   0,   0,  40, 6500,  0,   0, 100, 2, 1, 0}, {250, 170, 180,  20, 3000, 50, 100,   4, 2, 0, 0},
    {  0,   0,   0, 320, 6500,  0,   0, 100, 2, 1, 0}, {560, 560, 160,   0,    0,  0,  60,   0, 0, 3, 0},
    {  0,   0,   0, 900, 5000,  0,   0, 100, 2, 1, 0}, {520, 400,  25,  12, 2000,  4,  80,  40, 2, 0, 0},
    {  0,   0,   0, 700, 7500,  0,   0,  70, 2, 1, 0},
};
typedef struct {
    uint8_t on, type, wave, bits, filt, metal, width, burst;
    uint16_t age;
    int32_t pe, te, ne, ta, na, vel, a1, lp;
    uint32_t pdm, tdm, ndm, ph, f0, f1, lfsr, nph, ninc, sq[6], sqi[6];
} dv_t;
#define DVN 8u                                       /* hits that can ring at once on one channel */
static dv_t dv[MAXCH][DVN];
/* a drum channel's step is a set of drums (11 lanes) and which of them are accented; see dstep_hits in seq.c */
static const uint8_t DR_LANE_TYPE[11] = {0, 2, 4, 5, 7, 11, 3, 6, 8, 10, 12};       /* lanes BD SD LT MT HT RS CP CH OH CR RD: their drum in DRUM[] */
static uint32_t dv_dm(uint32_t ms) { uint32_t t = ms * 44u; return t < 2u ? 0u : 65536u - 65536u / t; }        /* decay multiplier per sample, Q16 */
static void dv_hit(uint32_t c, uint32_t note, uint32_t vel)
{
    static const uint8_t WAVE[38] = {1, 2, 2, 3, 2, 3, 2, 0, 0, 2, 2, 0, 1, 3, 2, 1, 1, 3, 0, 0, 0, 0, 1, 2, 2, 2, 3, 2, 2, 2, 0, 0, 2, 1, 0, 2, 2, 2}, BITS[38] = {4, 4, 8, 5, 5, 5, 5, 12, 6, 6, 2, 12, 4, 4, 5, 4, 6, 8, 12, 12, 12, 12, 4, 5, 6, 4, 8, 5, 5, 8, 12, 12, 3, 4, 6, 2, 5, 12};        /* by console: NES GB SMS PCE TIA VRC6 AY MD VB POKEY SPK OPL WSG N163 S5B MMC5 FDS SCC OPLL VRC7 OPM OPL3 WS SAA VIC TED LYNX PC98 NEO NGP OPL1 OPZ SN77 C30 MSM ZX OPN SID */
    static const uint16_t MET[6] = {205, 304, 370, 523, 540, 800};
    uint32_t type = note < 13u ? note : note % 12u, kit = DRUM_KIT(chan[c].kind), k, j = 0;
    const drum_t *d = &DRUM[type];
    dv_t *v;
    if (type == 6u) for (k = 0; k < DVN; k++) if (dv[c][k].on && dv[c][k].type == 8u) dv[c][k].on = 0;     /* a closed hat chokes an open one */
    for (k = 0; k < DVN && dv[c][k].on; k++) {}
    if (k == DVN) { for (k = 1; k < DVN; k++) if (dv[c][k].age > dv[c][j].age) j = k; k = j; }
    v = &dv[c][k];
    memset(v, 0, sizeof *v);
    v->on = 1; v->type = (uint8_t)type; v->wave = WAVE[kit]; v->bits = BITS[kit]; v->filt = d->filt; v->burst = d->burst;
    v->width = (kit == 4u ? (type == 2u || type == 3u ? 5u : 4u) : ((kit == 0u || kit == 1u || kit == 5u || kit == 6u || kit == 14u || kit == 15u) && d->metal == 1u ? 7u : 15u));
    v->metal = kit == 7u || kit == 20u || kit == 27u || kit == 28u || kit == 31u || kit == 36u ? d->metal : (kit == 11u || kit == 18u || kit == 19u || kit == 21u || kit == 30u) ? (d->metal == 1u ? 2u : d->metal) : d->metal == 3u ? 3u : 0u;     /* Mega Drive: six squares; Master System: the Yamaha XOR; the rest make hats from noise */
    v->vel = (int32_t)vel + 1;
    v->f0 = d->f0 * 97392u; v->f1 = d->f1 * 97392u;
    v->pe = d->pd ? 32767 : 0; v->pdm = dv_dm(d->pd);
    v->te = 32767; v->tdm = dv_dm(d->td); v->ta = d->ta;
    v->ne = 32767; v->ndm = dv_dm(d->nd); v->na = d->na;
    v->lfsr = (1u << (v->width - 1u)) | 1u;
    v->ninc = (d->cut ? d->cut * 3u : 16000u) * 97392u;
    v->a1 = d->cut ? (int32_t)(205887u * d->cut / (44100u + 6u * d->cut)) : 16384;
    if (v->metal == 3u) { v->sqi[0] = 560u * 97392u; v->sqi[1] = 845u * 97392u; }
    else if (v->metal == 2u) { v->sqi[0] = 1046u * 97392u; v->sqi[1] = 16742u * 97392u; v->sqi[2] = 33484u * 97392u; }
    else for (k = 0; k < 6u; k++) v->sqi[k] = (uint32_t)MET[k] * 97392u;
}
static int32_t dv_run(dv_t *v)
{
    int32_t out = 0, t;
    if (v->age < 65535u) v->age++;
    if (v->ta) {                                            /* the tone: its pitch falls from f0 to f1 */
        v->ph += v->f1 + ((v->f0 - v->f1) >> 15) * (uint32_t)v->pe;
        v->pe = (int32_t)(((uint32_t)v->pe * v->pdm) >> 16);
        if (v->wave == 0u) t = SIN_T[v->ph >> 22];
        else if (v->wave == 1u) { t = (int32_t)(v->ph >> 16); t = (t < 32768 ? t : 65535 - t) - 16384; }
        else if (v->wave == 3u) t = (int32_t)(v->ph >> 17) - 16384;
        else t = v->ph >> 31 ? 16384 : -16384;
        out = (t * v->te >> 15) * v->ta / 100;
        v->te = (int32_t)(((uint32_t)v->te * v->tdm) >> 16);
    }
    if (v->na) {                                            /* the noise: a shift register, or squares for the metal, through a filter */
        t = 0;
        if (v->metal) {
            uint32_t i, n = v->metal == 3u ? 2u : v->metal == 2u ? 3u : 6u;
            if (v->metal == 2u) { for (i = 0; i < 3u; i++) v->sq[i] += v->sqi[i]; t = (((v->sq[0] >> 31) | (v->sq[1] >> 31)) ^ (v->sq[2] >> 31)) ? 9000 : -9000; }
            else { for (i = 0; i < n; i++) { v->sq[i] += v->sqi[i]; t += v->sq[i] >> 31 ? (v->metal == 3u ? 2200 : 5200) : (v->metal == 3u ? -2200 : -5200); } }
        } else {
            uint32_t old = v->nph;
            v->nph += v->ninc;
            if (v->nph < old) v->lfsr = v->lfsr >> 1 | (((v->lfsr ^ v->lfsr >> 1) & 1u) << (v->width - 1u));
            t = v->lfsr & 1u ? 12000 : -12000;
        }
        if (v->filt) { v->lp += ((t - v->lp) * v->a1) >> 15; t = v->filt == 2u ? t - v->lp : v->lp; }
        if (v->burst && (v->age == 441u || v->age == 882u)) v->ne = 32767;      /* a clap is three hits */
        out += (t * v->ne >> 15) * v->na / 100;
        v->ne = (int32_t)(((uint32_t)v->ne * v->ndm) >> 16);
    }
    if (v->age < 44u) out = out * (int32_t)v->age / 44;     /* 1 ms in: no click */
    if (v->bits < 12u) { int32_t sh = 15 - (int32_t)v->bits; out = ((out + (1 << (sh - 1))) >> sh) * (1 << sh); }       /* the DAC's steps, rounded: a floor left a negative step at the end of every hit */
    if ((!v->ta || v->te < 80) && (!v->na || v->ne < 80) && (!v->burst || v->age > 1000u)) v->on = 0;
    return out * v->vel >> 7;
}
static void r_drum(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, k, j;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (k = 0; k < na; k++) {
            uint32_t c = act[k];
            const int32_t g = DRUM_KIT(chan[c].kind) == 13u ? 28 : 11;       /* Q3: the kits are 2.8 dB up, to sit with the voices; the Namco 163 kit is 8 dB more, with its voices */
            for (j = 0; j < DVN; j++) if (dv[c][j].on) acc += ((dv_run(&dv[c][j]) * (int32_t)PCE_AMP[osc[c].vol & 15u] >> 8) * g) >> 3;
        }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}

/* ---- the 60 Hz tick: envelope, vibrato, sweep and arp, then the oscillator */
static void chip_clear(uint32_t chip)
{
    uint32_t c;
    for (c = 0; c < chan_n; c++)
        if (KIND_CHIP[chan[c].kind] == chip) { memset(&vc[c], 0, sizeof vc[c]); vc[c].stage = ST_IDLE; memset(&osc[c], 0, sizeof osc[c]); osc[c].pan = 1; memset(dv[c], 0, sizeof dv[c]); memset(&sid[c], 0, sizeof sid[c]); }
    switch (chip) {
    case CHIP_NES: memset(hp_nes, 0, sizeof hp_nes); memset(&lp_nes, 0, sizeof lp_nes); break;
    case CHIP_GB: memset(hp_gb, 0, sizeof hp_gb); break;
    case CHIP_VRC6: memset(&hp_x[0], 0, sizeof hp_x[0]); break;
    case CHIP_AY: memset(&hp_x[1], 0, sizeof hp_x[1]); break;
    case CHIP_VB: memset(hp_vb, 0, sizeof hp_vb); break;
    case CHIP_MD: memset(hp_md, 0, sizeof hp_md); break;
    case CHIP_PCE: memset(hp_pc, 0, sizeof hp_pc); break;
    case CHIP_SPK: lp_sp = 0; memset(&hp_nx[1], 0, sizeof hp_nx[1]); break;
    case CHIP_WSG: lp_ws = 0; memset(&hp_nx[3], 0, sizeof hp_nx[3]); break;
    case CHIP_MMC5: memset(hp_mm, 0, sizeof hp_mm); memset(&lp_mm, 0, sizeof lp_mm); memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); break;
    case CHIP_FDS: memset(&lp_fds, 0, sizeof lp_fds); memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); break;
    case CHIP_OPL: case CHIP_N163: case CHIP_S5B: case CHIP_POKEY: case CHIP_SCC: case CHIP_OPLL: case CHIP_VRC7: case CHIP_OPL3: memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); break;
    case CHIP_VIC: case CHIP_TED: memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); break;
    case CHIP_OPZ: case CHIP_SN77: case CHIP_OPL1: case CHIP_MSM: case CHIP_ZX: case CHIP_OPN: case CHIP_SID: memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); break;
    case CHIP_OPM: case CHIP_WS: case CHIP_SAA: case CHIP_LYNX: case CHIP_PC98: case CHIP_NEO: case CHIP_NGP: case CHIP_C30: memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); memset(&hp_nr[STEREO_IDX(chip)], 0, sizeof hp_nr[0]); break;
    default: break;
    }
}
static volatile int32_t fx_pitch;                /* all voices, Q8 semitones: the pitch effects (fx.c) */
static volatile uint8_t fx_solo_ch = 255;        /* solo (fx.c): the one channel left sounding (255: none) */
static volatile uint8_t duck_ch = 255;           /* while recording: the channel being played stays loud, the others are ducked (255: none) */
static uint32_t tick_voice(uint32_t c)           /* returns 1 while the voice still sounds */
{
    const inst_t *in = &vc[c].in;
    int32_t pq, lfo = 0;
    uint32_t vol, kf = 256u;
    const uint32_t opll = IS_OPLL(chan[c].kind), pi = (in->wave > 14u ? 14u : in->wave) + (chan[c].kind == KD_VRC7 ? 15u : 0u);
    if (opll) kf = opll_kf(vc[c].note, OPLL_P[pi].fl >> 5 & 1u);       /* the carrier's key scaled rates */
    if (IS_DRUM(chan[c].kind)) {                    /* the hits ring on their own: the voice sounds while one does */
        uint32_t j, on = 0;
        for (j = 0; j < DVN; j++) on |= dv[c][j].on;
        osc[c].vol = in->vol & 15u; vc[c].env = on ? 3840u : 0u; vc[c].stage = on ? ST_SUS : ST_IDLE;
        return on;
    }
    if (chan[c].kind == KD_SID) { vc[c].env = 3840u; vc[c].stage = vc[c].on || vc[c].trig || sid_busy(c) ? (vc[c].on ? ST_SUS : ST_REL) : ST_IDLE; }       /* the SID's own envelope runs in the renderer */
    else switch (vc[c].stage) {
    case ST_ATK:
        if (chan[c].kind == KD_OPL || chan[c].kind == KD_OPL1 || chan[c].kind == KD_OPL3 || opll) {      /* the OPL family's attack: the attenuation falls exponentially from silence (a late start, a fast middle, a long tail), ATK = the ticks to -3 dB */
            const uint32_t n = in->atk & 15u, q = n * 1024u / kf;       /* the attack in quarter ticks (a higher note's key scaling shortens it) */
            const uint32_t k = ATK_K[q > 63u ? 63u : q];
            if (vc[c].age <= (n * 56u + 128u) / kf) break;
            vc[c].env += ((3840u - vc[c].env) * k + 2048u) >> 12;
            if (vc[c].env >= 3740u) { vc[c].env = 3840u; vc[c].stage = ST_DEC; }
            break;
        }
        vc[c].env += (in->atk ? 3840u / in->atk : 3840u) * kf >> 8;
        if (vc[c].env >= 3840u) { vc[c].env = 3840u; vc[c].stage = ST_DEC; }
        break;
    case ST_DEC:
        if (vc[c].env > (uint32_t)in->sus * 256u) {
            uint32_t d = opll ? ENV_EXP[in->dec & 15u] * kf >> 8 : 3840u / ((uint32_t)in->dec * 2u + 1u);       /* the ROM instruments fall in dB a second: the ear hears seconds, not ticks */
            vc[c].env = vc[c].env - (uint32_t)in->sus * 256u > d ? (uint16_t)(vc[c].env - d) : (uint16_t)(in->sus * 256u);
        }
        if (vc[c].env <= (uint32_t)in->sus * 256u) vc[c].stage = ST_SUS;
        break;
    case ST_REL: {
        uint32_t d = opll ? ENV_EXP[(OPLL_P[pi].ceg || in->rel < 4u ? in->rel : 4u) & 15u] * kf >> 8 : 3840u / ((uint32_t)in->rel * 2u + 1u);     /* a percussive carrier lets go at the chip's fixed rate 7 (knob 4), whatever its ROM release */
        if (vc[c].env > d) vc[c].env = (uint16_t)(vc[c].env - d); else { vc[c].env = 0; vc[c].stage = ST_IDLE; }
        break; }
    case ST_SUS:
        if (opll && vc[c].on && !OPLL_P[pi].ceg) {     /* a percussive instrument never holds */
            uint32_t d = ENV_EXP[in->rel & 15u] * kf >> 8;
            if (vc[c].env > d) vc[c].env = (uint16_t)(vc[c].env - d); else { vc[c].env = 0; vc[c].stage = ST_IDLE; }
        }
        break;
    default: break;
    }
    if (vc[c].age < 65535u) vc[c].age++;
    vc[c].bend += (int32_t)in->sweep * 4;
    if (in->vib_dep) { vc[c].vph = (uint8_t)(vc[c].vph + in->vib_spd * 8u); lfo = (VIB[vc[c].vph >> 3] * (int32_t)in->vib_dep) >> 3; }
    if (in->arp && in->arp <= 8u && vc[c].on) {                  /* the arp: a new chord note every few ticks while the key is down */
        if (++vc[c].arp_t >= ARP_TICKS[in->arp_rate & 3u]) { vc[c].arp_t = 0; vc[c].arp_k++; }
        vc[c].arp_off = (uint8_t)arp_offset(in->arp, in->arp_dir, vc[c].arp_k);
    }
    pq = (((int32_t)vc[c].note + vc[c].arp_off) << 8) + vc[c].bend + lfo + fx_pitch;
    pq = pq < 0 ? 0 : pq > (127 << 8) ? 127 << 8 : pq;
    vol = ((uint32_t)vc[c].env * ((uint32_t)vc[c].vel + 1u)) >> 15;
    if (vol > 15u) vol = 15u;
    vol = (vol * ((in->vol > 15u ? 15u : (uint32_t)in->vol) + 1u)) >> 4;
    vol_q = ((uint32_t)vc[c].env * ((uint32_t)vc[c].vel + 1u)) >> 11;
    if (vol_q > 240u) vol_q = 240u;
    vol_q = (vol_q * ((in->vol > 15u ? 15u : (uint32_t)in->vol) + 1u)) >> 4;
    if (duck_ch != 255u && c != duck_ch) { vol = vol * 5u >> 4; vol_q = vol_q * 5u >> 4; }       /* about -10 dB */
    if (fx_solo_ch != 255u && c != fx_solo_ch) { vol = 0; vol_q = 0; }
    set_osc(c, (uint32_t)pq, vol);
    if (chan[c].kind == KD_SID) vc[c].env = (uint16_t)(sid[c].env * 15u);       /* what the meter shows */
    return vc[c].stage != ST_IDLE;
}
static void fx_tick(void);
static void fx_master(int32_t *out, uint32_t n);
static void tick(void)
{
    uint32_t chip, c;
    fx_tick();
    tre_ph += 4041u; vib_ph += 6630u;
    for (chip = 0; chip < CHIPS; chip++) {
        uint32_t busy = 0;
        if (!awake[chip]) continue;
        for (c = 0; c < chan_n; c++)
            if (KIND_CHIP[chan[c].kind] == chip) busy |= tick_voice(c);
        if (busy) awake[chip] = AWAKE;
        else if (!--awake[chip]) chip_clear(chip);   /* silent for a while: stop rendering, start clean next time */
    }
}
static void ev_do(uint32_t op, uint32_t c, uint32_t a, uint32_t b)
{
    uint32_t chip;
    if (c >= chan_n) return;
    chip = KIND_CHIP[chan[c].kind];
    if (op == EV_ON) {
        awake[chip] = AWAKE;
        vc[c].in = chan[c].in;
        if (IS_DRUM(chan[c].kind)) dv_hit(c, a, b);
        if (chan[c].kind == KD_S5B) osc[c].ph3 = 0;       /* the envelope starts with the note, as a write to the chip's register 13 does */
        vc[c].on = 1; vc[c].trig = 1; vc[c].age = 0; vc[c].note = (uint8_t)a; vc[c].vel = (uint8_t)b; vc[c].bend = 0; vc[c].vph = 0;
        vc[c].arp_t = 0; vc[c].arp_k = 0; vc[c].arp_off = 0;
        vc[c].stage = ST_ATK; vc[c].env = 0;
        if (!vc[c].in.atk) { vc[c].env = 3840u; vc[c].stage = ST_DEC; }
        tick_voice(c);                           /* sound at once, not up to 16 ms later */
    } else if (awake[chip]) {
        vc[c].on = 0;
        vc[c].stage = vc[c].stage == ST_IDLE ? ST_IDLE : ST_REL;
    }
}
#include "seq.c"
#include "fx.c"

static void opll_set(inst_t *in, uint32_t kind, uint32_t n)      /* an instrument: the sound, and the envelope its ROM says (tweak it on the other pages after) */
{
    if (n > 14u) n = 14u;
    in->wave = (uint8_t)n;
    if (kind == KD_VRC7) n += 15u;
    in->atk = OPLL_P[n].atk; in->dec = OPLL_P[n].dec; in->sus = OPLL_P[n].sus; in->rel = OPLL_P[n].rel;
}

/* ---- changing the channel list (main loop; the audio side waits) */
static int chan_add(uint32_t kind)               /* the new channel's number, or -1 when full */
{
    uint32_t c;
    if (chan_n >= MAXCH || kind >= KINDS) return -1;
    chan_lock = 1;
    c = chan_n;
    chan[c].kind = (uint8_t)kind; chan[c].in = INST_DEF[kind];
    if (IS_OPLL(kind)) opll_set(&chan[c].in, kind, 0u);
    memset(&vc[c], 0, sizeof vc[c]); vc[c].stage = ST_IDLE;
    memset(&osc[c], 0, sizeof osc[c]); osc[c].pan = 1; wkey[c] = 0; memset(dv[c], 0, sizeof dv[c]); memset(&sid[c], 0, sizeof sid[c]);
    chan_n++;
    chan_lock = 0;
    return (int)c;
}
static void chan_remove(uint32_t c)
{
    uint32_t i;
    if (c >= chan_n) return;
    chan_lock = 1;
    for (i = c; i + 1u < chan_n; i++) { chan[i] = chan[i + 1u]; vc[i] = vc[i + 1u]; osc[i] = osc[i + 1u]; memcpy(dv[i], dv[i + 1u], sizeof dv[i]); sid[i] = sid[i + 1u]; }
    for (i = 0; i < MAXCH; i++) wkey[i] = 0;
    chan_n--;
    memset(&vc[chan_n], 0, sizeof vc[chan_n]); vc[chan_n].stage = ST_IDLE; memset(dv[chan_n], 0, sizeof dv[chan_n]); memset(&sid[chan_n], 0, sizeof sid[chan_n]);
    seq_chan_removed(c);
    chan_lock = 0;
}

/* the soft knee on the master: many voices at once never wrap */
static int32_t soft(int32_t x)
{
    if (x > 20000) { x = 20000 + ((x - 20000) >> 2); return x > 32767 ? 32767 : x; }
    if (x < -20000) { x = -20000 + ((x + 20000) >> 2); return x < -32767 ? -32767 : x; }
    return x;
}

/* the app's audio entry: events, the tick every 1/60 s (sample accurate), then each sounding chip into the mix */
static void chip_render(int32_t *out, uint32_t n)
{
    static int32_t mix[2u * 256u];
    static uint8_t act[MAXCH], actd[MAXCH];
    uint32_t i = 0, chip, k, na, nd;
    if (chan_lock) return;
    if (!booted) { for (chip = 0; chip < CHIPS; chip++) chip_clear(chip); booted = 1; }
    while (ev_r != ev_w) {
        ev_do(evq[ev_r & 31u].op, evq[ev_r & 31u].ch, evq[ev_r & 31u].a, evq[ev_r & 31u].b);
        ev_r++;
    }
    while (i < n) {
        uint32_t run;
        while (seq_run && !seq_hold && !seq_left) { seq_tick(); seq_left = seq_period(); }
        run = n - i < tick_left ? n - i : tick_left;
        if (seq_run && !seq_hold && run > seq_left) run = seq_left;
        if (run > 256u) run = 256u;
        for (chip = 0; chip < CHIPS; chip++) {
            if (!awake[chip]) continue;
            na = active_of(chip, act, 0); nd = active_of(chip, actd, 1);
            memset(mix, 0, 8u * run);
            if (na) switch (chip) {
            case CHIP_NES: r_nes(mix, run, act, na); break;
            case CHIP_GB: r_gb(mix, run, act, na); break;
            case CHIP_PCE: r_pce(mix, run, act, na); break;
            case CHIP_TIA: r_tia(mix, run, act, na); break;
            case CHIP_VRC6: r_vrc6(mix, run, act, na); break;
            case CHIP_AY: r_ay(mix, run, act, na); break;
            case CHIP_MD: r_fm4(mix, run, act, na, 2); break;
            case CHIP_VB: r_vb(mix, run, act, na); break;
            case CHIP_POKEY: r_pokey(mix, run, act, na); break;
            case CHIP_SPK: r_spk(mix, run, act, na); break;
            case CHIP_OPL: r_opl(mix, run, act, na); break;
            case CHIP_WSG: r_wave4(mix, run, act, na, 0); break;
            case CHIP_N163: r_wave4(mix, run, act, na, 1); break;
            case CHIP_S5B: r_s5b(mix, run, act, na); break;
            case CHIP_MMC5: r_mmc5(mix, run, act, na); break;
            case CHIP_FDS: r_fds(mix, run, act, na); break;
            case CHIP_SCC: r_scc(mix, run, act, na); break;
            case CHIP_OPLL: case CHIP_VRC7: r_opl(mix, run, act, na); break;
            case CHIP_OPM: r_fm4(mix, run, act, na, 0); break;
            case CHIP_OPL3: r_fm4(mix, run, act, na, 1); break;
            case CHIP_WS: r_ws(mix, run, act, na); break;
            case CHIP_SAA: r_saa(mix, run, act, na); break;
            case CHIP_VIC: r_vic(mix, run, act, na); break;
            case CHIP_TED: r_ted(mix, run, act, na); break;
            case CHIP_LYNX: r_lynx(mix, run, act, na); break;
            case CHIP_PC98: r_two(mix, run, act, na, KD_PC98_FM); break;
            case CHIP_OPN: r_two(mix, run, act, na, KD_OPN_FM); break;
            case CHIP_SID: r_sid(mix, run, act, na); break;
            case CHIP_NEO: r_two(mix, run, act, na, KD_NEO_FM); break;
            case CHIP_NGP: r_ngp(mix, run, act, na); break;
            case CHIP_OPL1: r_opl(mix, run, act, na); break;
            case CHIP_OPZ: r_fm4(mix, run, act, na, 1); break;
            case CHIP_SN77: r_sn77(mix, run, act, na); break;
            case CHIP_C30: r_c30(mix, run, act, na); break;
            case CHIP_MSM: r_msm(mix, run, act, na); break;
            case CHIP_ZX: r_zx(mix, run, act, na); break;
            default: r_sms(mix, run, act, na); break;
            }
            post(chip, mix, run);
            if (nd) r_drum(mix, run, actd, nd);                 /* the console's drum kit, past its analog stage */
            for (k = 0; k < 2u * run; k++) out[2u * i + k] += mix[k];
        }
        i += run; tick_left -= run;
        if (seq_run && !seq_hold) seq_left -= run;
        if (!tick_left) { tick_left = tick_len; tick(); }
    }
    fx_master(out, n);
    for (k = 0; k < 2u * n; k++) out[k] = soft(out[k]);
}

/* what the screen shows: 0..15 for a channel */
static uint32_t chip_level(uint32_t c) { return c < chan_n ? (uint32_t)vc[c].env >> 8 : 0u; }
