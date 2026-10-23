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
enum { CHIP_NES, CHIP_GB, CHIP_SMS, CHIP_PCE, CHIP_TIA, CHIP_VRC6, CHIP_AY, CHIP_MD, CHIP_VB, CHIP_POKEY, CHIP_SPK, CHIP_OPL, CHIP_WSG, CHIP_N163, CHIP_S5B, CHIPS };
enum { KD_NES_PU, KD_NES_TRI, KD_GB_PU, KD_GB_WAV, KD_SMS, KD_PCE, KD_TIA, KD_VRC6_PU, KD_VRC6_SAW, KD_AY, KD_MD, KD_VB_WAV, KD_DRUM0, KD_POKEY = KD_DRUM0 + 9, KD_SPK, KD_OPL, KD_WSG, KD_N163, KD_S5B, KD_DRUMB, KINDS = KD_DRUMB + 6 };       /* the drum kits, one a console, in the order of the chips */
#define IS_DRUM(k) (((k) >= KD_DRUM0 && (k) < KD_POKEY) || (k) >= KD_DRUMB)      /* the kits of the first nine consoles come first, the later ones after the later chips */
#define DRUM_KIT(k) ((k) < KD_POKEY ? (k) - KD_DRUM0 : 9u + (k) - KD_DRUMB)
static const uint8_t KIND_CHIP[KINDS] = {CHIP_NES, CHIP_NES, CHIP_GB, CHIP_GB, CHIP_SMS, CHIP_PCE, CHIP_TIA, CHIP_VRC6, CHIP_VRC6, CHIP_AY, CHIP_MD, CHIP_VB,
                                         CHIP_NES, CHIP_GB, CHIP_SMS, CHIP_PCE, CHIP_TIA, CHIP_VRC6, CHIP_AY, CHIP_MD, CHIP_VB,
                                         CHIP_POKEY, CHIP_SPK, CHIP_OPL, CHIP_WSG, CHIP_N163, CHIP_S5B,
                                         CHIP_POKEY, CHIP_SPK, CHIP_OPL, CHIP_WSG, CHIP_N163, CHIP_S5B};

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
static const inst_t INST_DEF[KINDS] = {
    I(0, 2, 0, 5, 9, 4, 5, 0, 0, 1),            /* NES pulse */
    I(0, 0, 0, 0, 15, 2, 5, 0, 0, 1),           /* NES triangle */
    I(0, 2, 0, 5, 9, 4, 5, 0, 0, 1),            /* Game Boy pulse */
    I(15, 0, 0, 0, 15, 2, 5, 0, 0, 1),          /* Game Boy wave */
    I(0, 0, 0, 6, 14, 4, 5, 4, 0, 1),           /* Master System tone */
    I(5, 0, 0, 0, 12, 2, 5, 0, 0, 1),           /* PC Engine wave: a saw to start with */
    I(5, 0, 0, 4, 12, 3, 5, 0, 0, 1),           /* Atari TIA: wave = the sound type 0..9 */
    I(0, 3, 0, 5, 10, 4, 5, 0, 0, 1),           /* VRC6 pulse: duty 0..7 */
    I(0, 0, 0, 0, 15, 2, 5, 0, 0, 1),           /* VRC6 saw */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 1),           /* AY: wave = the noise mix 0..15 */
    {1, 0, 6, 0, 4, 0, 6, 12, 3, 5, 0, 0, 1, 15, 0, 1, 0},     /* Mega Drive FM: duty = algorithm 0..3, wave = ratio, wf = index, wc = feedback, wt = index decay */
    I(10, 0, 0, 0, 15, 2, 5, 0, 0, 1),          /* Virtual Boy: a wave, a triangle to start with */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),     /* drums: a kit per console */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
    I(1, 0, 0, 4, 12, 3, 5, 0, 0, 1),           /* POKEY: wave = the distortion 0..7 */
    I(0, 0, 0, 0, 15, 1, 5, 0, 0, 1),           /* PC speaker: one square, no volume */
    {1, 0, 7, 0, 3, 0, 6, 12, 3, 5, 0, 0, 1, 15, 0, 1, 0},     /* AdLib OPL2: duty = the pair 0..3, wave = ratio, wf = index, wc = feedback, wt = index decay */
    I(5, 0, 0, 0, 12, 2, 5, 0, 0, 1),           /* Namco WSG: a saw to start with */
    I(10, 0, 0, 0, 15, 2, 5, 0, 0, 1),          /* Namco 163: a triangle */
    I(0, 0, 0, 5, 12, 3, 5, 0, 0, 0),           /* Sunsoft 5B: wave = the noise mix, pan = the buzz (0 off) */
    I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1), I(0, 0, 0, 4, 15, 4, 5, 0, 0, 1),
};
static const char *const KIND_NAME[KINDS] = {"PU", "TRI", "PU", "WAV", "TN", "PCE", "TIA", "PU", "SAW", "AY", "FM", "VB", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM", "POK", "SPK", "OPL", "WSG", "N16", "5B", "DRM", "DRM", "DRM", "DRM", "DRM", "DRM"};

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
    uint16_t age;                               /* ticks since the note began */
    uint16_t env;                               /* Q8, 0..15 */
    uint32_t arp_k;
    int32_t bend;                               /* Q8 semitones from sweep */
    inst_t in;
} vc[MAXCH];
static struct { uint32_t ph, inc; const uint8_t *wav; uint8_t duty, vol, pan, wshift; uint32_t ph2, ph3, inc2, inc3, idx, rng; int32_t y1, y2; uint8_t alg, fb; } osc[MAXCH];
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
static hpf_t hp_nes[2], hp_gb[2], hp_x[2], hp_vb[2], hp_nx[6];       /* ... and the VRC6 and the AY keep a DC blocker */               /* NES: two stages; Game Boy: left, right */
static int32_t lp_nes, lp_sp, lp_ws;
static int32_t hpf(hpf_t *f, int32_t x, int32_t b)
{
    int32_t t = f->y1 + x - f->x1, y = t - ((t * b) >> 16);
    f->x1 = x; f->y1 = y;
    return y;
}
static int32_t lpf(int32_t *y, int32_t x, int32_t a)
{
    *y += ((x - *y) * a) >> 14;
    return *y;
}
static void post(uint32_t chip, int32_t *out, uint32_t n)
{
    uint32_t i, m = analog_mode[chip];
    if (chip >= CHIP_SMS && chip != CHIP_VRC6 && chip != CHIP_AY && chip != CHIP_VB && chip < CHIP_POKEY) return;
    for (i = 0; i < n; i++) {
        int32_t l = out[2u * i], r = out[2u * i + 1u];
        if (chip == CHIP_NES) {
            if (m == AN_HW) l = lpf(&lp_nes, hpf(&hp_nes[1], hpf(&hp_nes[0], l, B_NES_HP90), B_NES_HP440), A_NES_LP14K);
            else if (m == AN_ALT) l = hpf(&hp_nes[0], l, B_FAM_HP37);
            else l = hpf(&hp_nes[0], l, B_DC_HP14);
            r = l;
        } else if (chip == CHIP_VB) {
            l = hpf(&hp_vb[0], l, B_DC_HP14); r = hpf(&hp_vb[1], r, B_DC_HP14);
        } else if (chip >= CHIP_POKEY) {                 /* the later chips: a DC blocker each; the PC speaker's little cone, the Namco DAC's filter */
            hpf_t *h = &hp_nx[chip - CHIP_POKEY];
            if (chip == CHIP_SPK) l = lpf(&lp_sp, hpf(h, l, B_SPK_HP), A_SPK_LP);
            else if (chip == CHIP_WSG) l = lpf(&lp_ws, hpf(h, l, B_DC_HP14), A_WSG_LP);
            else l = hpf(h, l, B_DC_HP14);
            r = l;
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
static uint8_t wtab[MAXCH][32];
static uint32_t wkey[MAXCH];
static void wave_make_n(const inst_t *in, uint8_t *out, int32_t top)       /* top 15: 4 bit; 63: the Virtual Boy's 6 bit */
{
    int32_t i, lv = (top + 1) - (int32_t)(in->wc & 15u) * ((top + 1) / 16), sh = (int32_t)(in->wave & 15u), a = sh / 5, fr = (sh % 5) * 256 / 5;
    if (a > 2) { a = 2; fr = 256; }
    if (lv < 2) lv = 2;
    for (i = 0; i < 32; i++) {
        int32_t ph = (i + (VIB[i] * (int32_t)(in->wt & 15u)) / 238) & 31, v[4], x, g;
        v[0] = ph < 16 ? 127 : -127;
        v[1] = -127 + ph * 254 / 31;
        v[2] = ph < 16 ? -127 + ph * 254 / 15 : 127 - (ph - 16) * 254 / 15;
        v[3] = VIB[(ph + 24) & 31];                          /* a sine that starts at the bottom, like the others */
        x = v[a] + (v[a + 1] - v[a]) * fr / 256;
        g = 256 + (int32_t)(in->wf & 15u) * 64;
        x = x * g >> 8;
        while (x > 127 || x < -127) x = x > 127 ? 254 - x : -254 - x;
        x = ((x + 127) * (lv - 1) + 127) / 254;             /* 0 .. lv - 1 */
        out[i] = (uint8_t)(x * top / (lv - 1));
    }
}
static void wave_make(const inst_t *in, uint8_t *out) { wave_make_n(in, out, 15); }
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
            uint32_t key = (1u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make(wi, wtab[c]); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        break;
    case KD_VB_WAV:                                     /* the VSU: 32 samples of 6 bits, stereo volume; the wave is read live */
        osc[c].inc = interp32(NOTE_INC, pq); osc[c].pan = in->pan; osc[c].vol = (uint8_t)vol;
        {   const inst_t *wi = &chan[c].in;
            uint32_t key = (2u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make_n(wi, wtab[c], 63); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        break;
    case KD_TIA: {                                      /* a pattern of bits stepped round: tonal ones play the note as their cycle, the long noisy ones as their step rate */
        uint32_t w = chan[c].in.wave > 9u ? 9u : chan[c].in.wave, len = TIA_LEN[w];
        t = interp32(NOTE_INC, pq);
        osc[c].inc = len > 186u ? t / len * 32u : t; osc[c].duty = (uint8_t)w; osc[c].vol = (uint8_t)vol;
        break; }
    case KD_VRC6_PU: osc[c].inc = interp32(NOTE_INC, pq); osc[c].duty = in->duty & 7u; osc[c].vol = (uint8_t)vol; break;
    case KD_VRC6_SAW: osc[c].inc = interp32(NOTE_INC, pq); osc[c].vol = (uint8_t)vol; break;
    case KD_AY: osc[c].inc = interp32(NOTE_INC, pq); osc[c].inc2 = osc[c].inc * 8u; osc[c].duty = chan[c].in.wave & 15u; osc[c].vol = (uint8_t)vol; break;
    case KD_MD: {                                       /* three operators: A (the ratio, with feedback), B, C (the carrier at the note) */
        const inst_t *wi = &chan[c].in;
        uint32_t alg = wi->duty & 3u, age = vc[c].age, ix = 4096u / (16u + age * (wi->wt & 15u));
        t = interp32(NOTE_INC, pq);
        osc[c].inc = t; osc[c].inc2 = (t >> 1) * FM_RAT[wi->wave & 15u]; osc[c].inc3 = alg == 1u ? t : t * 2u;
        osc[c].idx = FM_IDX[wi->wf & 15u] * ix >> 8; osc[c].fb = wi->wc & 15u; osc[c].alg = (uint8_t)alg;
        osc[c].vol = (uint8_t)vol; osc[c].pan = in->pan;
        break; }
    case KD_POKEY: {                                    /* a distortion as a cycle of bits, like the TIA; the long noisy ones play the note as their step rate */
        uint32_t w = chan[c].in.wave > 7u ? 7u : chan[c].in.wave, len = POKEY_LEN[w];
        t = interp32(NOTE_INC, pq);
        osc[c].inc = len > 186u ? t / len * 32u : t; osc[c].duty = (uint8_t)w; osc[c].vol = (uint8_t)vol;
        break; }
    case KD_SPK: osc[c].inc = interp32(SPK_INC, pq); osc[c].vol = vol > 0u; break;        /* the timer's divisor is a whole number; no volume: on or off */
    case KD_OPL: {                                      /* two operators: A (the ratio, with feedback) and C (the note) */
        const inst_t *wi = &chan[c].in;
        uint32_t ix = 4096u / (16u + vc[c].age * (wi->wt & 15u));
        t = interp32(NOTE_INC, pq);
        osc[c].inc = t; osc[c].inc2 = (t >> 1) * FM_RAT[wi->wave & 15u];
        osc[c].idx = FM_IDX[wi->wf & 15u] * ix >> 8; osc[c].fb = wi->wc & 15u; osc[c].alg = (uint8_t)(wi->duty & 3u); osc[c].vol = (uint8_t)vol;
        break; }
    case KD_WSG: case KD_N163:                          /* a 4 bit wave, 32 steps, read live; the volume is linear */
        osc[c].inc = interp32(NOTE_INC, pq); osc[c].vol = (uint8_t)vol;
        {   const inst_t *wi = &chan[c].in;
            uint32_t key = (1u << 16) | (wi->wave & 15u) | (wi->wf & 15u) << 4 | (wi->wc & 15u) << 8 | (wi->wt & 15u) << 12;
            if (wkey[c] != key) { wave_make(wi, wtab[c]); wkey[c] = key; }
            osc[c].wav = wtab[c];
        }
        break;
    case KD_S5B: osc[c].inc = interp32(NOTE_INC, pq); osc[c].inc2 = osc[c].inc * 8u; osc[c].inc3 = osc[c].inc; osc[c].duty = chan[c].in.wave & 15u; osc[c].alg = chan[c].in.pan & 3u; osc[c].vol = (uint8_t)vol; break;
    default: osc[c].inc = K_SMS / interp(SMS_N, pq); osc[c].vol = (uint8_t)vol; break;
    }
}

/* the voices of a chip that sound now */
static uint32_t active_of(uint32_t chip, uint8_t *list, uint32_t drum)
{
    uint32_t c, n = 0;
    for (c = 0; c < chan_n; c++)
        if (KIND_CHIP[chan[c].kind] == chip && vc[c].stage != ST_IDLE && (uint32_t)IS_DRUM(chan[c].kind) == drum) list[n++] = (uint8_t)c;
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
                else if (osc[c].vol) { st = osc[c].ph >> 27; tsum += (int32_t)(st < 16u ? 15u - st : st - 16u); }
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
                v = ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 15) * PCE_AMP[osc[c].vol & 15u];
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l + (l >> 2); out[2u * i + 1u] += r + (r >> 2);
    }
}
static void r_tia(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], x = (osc[c].ph >> 16) * TIA_LEN[osc[c].duty] >> 16;
                int32_t a = TIA_AMP[osc[c].vol & 15u];
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
                if (chan[c].kind == KD_VRC6_PU) { if ((osc[c].ph >> 28) <= osc[c].duty) acc += (int32_t)osc[c].vol * 2; }
                else { int32_t v = (int32_t)(((osc[c].ph >> 16) * 7u >> 16) * osc[c].vol * 3u >> 3); acc += v > 31 ? 31 : v; }
            }
        out[2u * i] += acc * 72; out[2u * i + 1u] += acc * 72;
    }
}
static void r_ay(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], nz = osc[c].duty, old = osc[c].ph2;
                int32_t tb = osc[c].ph >> 31 ? 1 : -1, nb;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 3) & 1u) << 16));
                if (!osc[c].rng) osc[c].rng = 1u;
                nb = osc[c].rng & 1u ? 1 : -1;
                acc += (tb * (int32_t)(16u - nz) + nb * (int32_t)nz) * AY_AMP[osc[c].vol & 15u];
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
#define FMOS 2u
#define FMS(ph, m) SIN_T[(((ph) + (m)) >> 22) & 1023u]       /* a sine at a phase pushed by a modulation */
#define FMM(x, i) ((uint32_t)(x) * (i) << 2)                  /* a modulator's output times the depth, as a phase */
static void r_md(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < FMOS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t a, b, o, v;
                osc[c].ph += osc[c].inc << 1; osc[c].ph2 += osc[c].inc2 << 1; osc[c].ph3 += osc[c].inc3 << 1;
                a = FMS(osc[c].ph2, (uint32_t)((osc[c].y1 + osc[c].y2) >> 1) * FM_FB[osc[c].fb] << 2);
                osc[c].y2 = osc[c].y1; osc[c].y1 = a;
                if (osc[c].alg == 0u) o = FMS(osc[c].ph, FMM(a, osc[c].idx));
                else if (osc[c].alg == 1u) { b = FMS(osc[c].ph3, FMM(a, osc[c].idx)); o = FMS(osc[c].ph, FMM(b, osc[c].idx)); }
                else if (osc[c].alg == 2u) o = (FMS(osc[c].ph, FMM(a, osc[c].idx)) + FMS(osc[c].ph3, 0u)) >> 1;
                else o = (a + FMS(osc[c].ph3, 0u) + FMS(osc[c].ph, 0u)) / 3;
                v = o * PCE_AMP[osc[c].vol & 15u] >> 8;
                if (osc[c].pan != 2u) l += v;
                if (osc[c].pan != 0u) r += v;
            }
        out[2u * i] += l + (l >> 1); out[2u * i + 1u] += r + (r >> 1);
    }
}
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
static void r_pokey(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], x = (osc[c].ph >> 16) * POKEY_LEN[osc[c].duty] >> 16;
                int32_t a = TIA_AMP[osc[c].vol & 15u];
                osc[c].ph += osc[c].inc;
                acc += POKEY_BITS[osc[c].duty][x >> 3] >> (x & 7u) & 1u ? a : -a;
            }
        out[2u * i] += acc * 9; out[2u * i + 1u] += acc * 9;
    }
}
static void r_spk(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                if (osc[c].vol) acc += osc[c].ph >> 31 ? 2400 : -2400;
            }
        out[2u * i] += acc >> 2; out[2u * i + 1u] += acc >> 2;
    }
}
static int32_t opl_wave(int32_t x, uint32_t type) { return type == 1u ? (x > 0 ? x : 0) : type == 2u ? (x < 0 ? -x : x) : x; }       /* the OPL2's sine, half sine, absolute sine */
static void r_opl(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < FMOS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                int32_t a, o;
                osc[c].ph += osc[c].inc << 1; osc[c].ph2 += osc[c].inc2 << 1;
                a = FMS(osc[c].ph2, (uint32_t)((osc[c].y1 + osc[c].y2) >> 1) * FM_FB[osc[c].fb] << 2);
                osc[c].y2 = osc[c].y1; osc[c].y1 = a;
                if (osc[c].alg < 3u) o = opl_wave(FMS(osc[c].ph, FMM(a, osc[c].idx)), osc[c].alg);
                else o = (a + FMS(osc[c].ph, 0u)) >> 1;
                acc += o * PCE_AMP[osc[c].vol & 15u] >> 8;
            }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_wave4(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na, uint32_t n163)       /* the Namco chips: 4 bit waves, linear volume; the 163 shares one DAC between its voices, so more voices means a lower rate */
{
    static uint32_t cnt;
    static int32_t hold;
    uint32_t i, s, k, rate = 119318u / (na > 8u ? 8u : na ? na : 1u);
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++) {
            int32_t v = 0;
            for (k = 0; k < na; k++) {
                uint32_t c = act[k];
                osc[c].ph += osc[c].inc;
                v += ((int32_t)osc[c].wav[osc[c].ph >> 27] * 2 - 15) * (int32_t)osc[c].vol * 7;
            }
            if (n163) { cnt += rate; if (cnt >= FS * OS) { cnt -= FS * OS; hold = v; } acc += hold; }
            else acc += v;
        }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}
static void r_s5b(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* the AY's tone and noise, and the buzz: its envelope run as a wave at the note's pitch */
{
    uint32_t i, s, k;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < OS; s++)
            for (k = 0; k < na; k++) {
                uint32_t c = act[k], nz = osc[c].duty, old = osc[c].ph2, lv = osc[c].vol & 15u;
                int32_t tb = osc[c].ph >> 31 ? 1 : -1, nb;
                osc[c].ph += osc[c].inc; osc[c].ph2 += osc[c].inc2;
                if (osc[c].alg) {
                    uint32_t e = osc[c].ph3 >> 28;
                    osc[c].ph3 += osc[c].inc3;
                    e = osc[c].alg == 1u ? 15u - e : osc[c].alg == 2u ? e : e < 8u ? e * 2u : (15u - e) * 2u + 1u;
                    lv = (e > 15u ? 15u : e) * lv / 15u;
                }
                if (osc[c].ph2 < old) osc[c].rng = (osc[c].rng >> 1 | (((osc[c].rng ^ osc[c].rng >> 3) & 1u) << 16));
                if (!osc[c].rng) osc[c].rng = 1u;
                nb = osc[c].rng & 1u ? 1 : -1;
                acc += (tb * (int32_t)(16u - nz) + nb * (int32_t)nz) * AY_AMP[lv];
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
    static const uint8_t WAVE[15] = {1, 2, 0, 3, 2, 3, 2, 0, 0, 2, 2, 0, 1, 3, 2}, BITS[15] = {4, 4, 8, 5, 5, 5, 5, 12, 6, 6, 2, 12, 4, 4, 5};        /* by console: NES GB SMS PCE TIA VRC6 AY MD VB POKEY SPK OPL WSG N163 S5B */
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
    v->width = (kit == 4u ? (type == 2u || type == 3u ? 5u : 4u) : ((kit == 0u || kit == 1u || kit == 5u || kit == 6u || kit == 14u) && d->metal == 1u ? 7u : 15u));
    v->metal = kit == 7u ? d->metal : (kit == 2u || kit == 11u) ? (d->metal == 1u ? 2u : d->metal) : d->metal == 3u ? 3u : 0u;     /* Mega Drive: six squares; Master System: the Yamaha XOR; the rest make hats from noise */
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
    if (v->bits < 12u) { int32_t sh = 15 - (int32_t)v->bits; out = out >> sh << sh; }
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
            for (j = 0; j < DVN; j++) if (dv[c][j].on) acc += dv_run(&dv[c][j]) * (int32_t)PCE_AMP[osc[c].vol & 15u] >> 8;
        }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
}

/* ---- the 60 Hz tick: envelope, vibrato, sweep and arp, then the oscillator */
static void chip_clear(uint32_t chip)
{
    uint32_t c;
    for (c = 0; c < chan_n; c++)
        if (KIND_CHIP[chan[c].kind] == chip) { memset(&vc[c], 0, sizeof vc[c]); vc[c].stage = ST_IDLE; memset(&osc[c], 0, sizeof osc[c]); osc[c].pan = 1; memset(dv[c], 0, sizeof dv[c]); }
    switch (chip) {
    case CHIP_NES: memset(hp_nes, 0, sizeof hp_nes); lp_nes = 0; break;
    case CHIP_GB: memset(hp_gb, 0, sizeof hp_gb); break;
    case CHIP_VRC6: memset(&hp_x[0], 0, sizeof hp_x[0]); break;
    case CHIP_AY: memset(&hp_x[1], 0, sizeof hp_x[1]); break;
    case CHIP_VB: memset(hp_vb, 0, sizeof hp_vb); break;
    case CHIP_SPK: lp_sp = 0; memset(&hp_nx[1], 0, sizeof hp_nx[1]); break;
    case CHIP_WSG: lp_ws = 0; memset(&hp_nx[3], 0, sizeof hp_nx[3]); break;
    case CHIP_OPL: case CHIP_N163: case CHIP_S5B: case CHIP_POKEY: memset(&hp_nx[chip - CHIP_POKEY], 0, sizeof hp_nx[0]); break;
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
    uint32_t vol;
    if (IS_DRUM(chan[c].kind)) {                    /* the hits ring on their own: the voice sounds while one does */
        uint32_t j, on = 0;
        for (j = 0; j < DVN; j++) on |= dv[c][j].on;
        osc[c].vol = in->vol & 15u; vc[c].env = on ? 3840u : 0u; vc[c].stage = on ? ST_SUS : ST_IDLE;
        return on;
    }
    switch (vc[c].stage) {
    case ST_ATK:
        vc[c].env += in->atk ? 3840u / in->atk : 3840u;
        if (vc[c].env >= 3840u) { vc[c].env = 3840u; vc[c].stage = ST_DEC; }
        break;
    case ST_DEC:
        if (vc[c].env > (uint32_t)in->sus * 256u) {
            uint32_t d = 3840u / ((uint32_t)in->dec * 2u + 1u);
            vc[c].env = vc[c].env - (uint32_t)in->sus * 256u > d ? (uint16_t)(vc[c].env - d) : (uint16_t)(in->sus * 256u);
        }
        if (vc[c].env <= (uint32_t)in->sus * 256u) vc[c].stage = ST_SUS;
        break;
    case ST_REL: {
        uint32_t d = 3840u / ((uint32_t)in->rel * 2u + 1u);
        if (vc[c].env > d) vc[c].env = (uint16_t)(vc[c].env - d); else { vc[c].env = 0; vc[c].stage = ST_IDLE; }
        break; }
    default: break;
    }
    if (vc[c].age < 65535u) vc[c].age++;
    vc[c].bend += (int32_t)in->sweep * 4;
    if (in->vib_dep) { vc[c].vph = (uint8_t)(vc[c].vph + in->vib_spd * 8u); lfo = (VIB[vc[c].vph >> 3] * (int32_t)in->vib_dep) >> 3; }
    if (in->arp && vc[c].on) {                  /* the arp: a new chord note every few ticks while the key is down */
        if (++vc[c].arp_t >= ARP_TICKS[in->arp_rate & 3u]) { vc[c].arp_t = 0; vc[c].arp_k++; }
        vc[c].arp_off = (uint8_t)arp_offset(in->arp, in->arp_dir, vc[c].arp_k);
    }
    pq = (((int32_t)vc[c].note + vc[c].arp_off) << 8) + vc[c].bend + lfo + fx_pitch;
    pq = pq < 0 ? 0 : pq > (127 << 8) ? 127 << 8 : pq;
    vol = ((uint32_t)vc[c].env * ((uint32_t)vc[c].vel + 1u)) >> 15;
    if (vol > 15u) vol = 15u;
    vol = (vol * ((uint32_t)in->vol + 1u)) >> 4;
    if (duck_ch != 255u && c != duck_ch) vol = vol * 5u >> 4;       /* about -10 dB */
    if (fx_solo_ch != 255u && c != fx_solo_ch) vol = 0;
    set_osc(c, (uint32_t)pq, vol);
    return vc[c].stage != ST_IDLE;
}
static void fx_tick(void);
static void fx_master(int32_t *out, uint32_t n);
static void tick(void)
{
    uint32_t chip, c;
    fx_tick();
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
        vc[c].on = 1; vc[c].age = 0; vc[c].note = (uint8_t)a; vc[c].vel = (uint8_t)b; vc[c].bend = 0; vc[c].vph = 0;
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

/* ---- changing the channel list (main loop; the audio side waits) */
static int chan_add(uint32_t kind)               /* the new channel's number, or -1 when full */
{
    uint32_t c;
    if (chan_n >= MAXCH || kind >= KINDS) return -1;
    chan_lock = 1;
    c = chan_n;
    chan[c].kind = (uint8_t)kind; chan[c].in = INST_DEF[kind];
    memset(&vc[c], 0, sizeof vc[c]); vc[c].stage = ST_IDLE;
    memset(&osc[c], 0, sizeof osc[c]); osc[c].pan = 1; wkey[c] = 0; memset(dv[c], 0, sizeof dv[c]);
    chan_n++;
    chan_lock = 0;
    return (int)c;
}
static void chan_remove(uint32_t c)
{
    uint32_t i;
    if (c >= chan_n) return;
    chan_lock = 1;
    for (i = c; i + 1u < chan_n; i++) { chan[i] = chan[i + 1u]; vc[i] = vc[i + 1u]; osc[i] = osc[i + 1u]; memcpy(dv[i], dv[i + 1u], sizeof dv[i]); }
    for (i = 0; i < MAXCH; i++) wkey[i] = 0;
    chan_n--;
    memset(&vc[chan_n], 0, sizeof vc[chan_n]); vc[chan_n].stage = ST_IDLE; memset(dv[chan_n], 0, sizeof dv[chan_n]);
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
            case CHIP_MD: r_md(mix, run, act, na); break;
            case CHIP_VB: r_vb(mix, run, act, na); break;
            case CHIP_POKEY: r_pokey(mix, run, act, na); break;
            case CHIP_SPK: r_spk(mix, run, act, na); break;
            case CHIP_OPL: r_opl(mix, run, act, na); break;
            case CHIP_WSG: r_wave4(mix, run, act, na, 0); break;
            case CHIP_N163: r_wave4(mix, run, act, na, 1); break;
            case CHIP_S5B: r_s5b(mix, run, act, na); break;
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
