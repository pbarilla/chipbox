/* SPDX-License-Identifier: GPL-3.0-only */
/* The sound chips: NES 2A03, Game Boy, Master System PSG: three channels each (no noise channels). Included by chipbox.c after platform.c.
 *
 * Layers, top down:
 *   chip_note / chip_off      the app (main loop) -> a small event queue
 *   tick()                    60 Hz, in the audio ISR: instrument envelope, vibrato, sweep -> the chip's registers
 *   r_nes / r_gb / r_sms   the chips themselves, 4x oversampled and averaged down
 * All three chips can sound at once; a chip with nothing playing is not rendered.
 * Tables come from tools/gen_chip_tables.py. Integers only. */
#include "chip_tables.h"

#define OS 4u                                   /* oscillator steps per output frame */
enum { CHIP_NES, CHIP_GB, CHIP_SMS, CHIPS };
static const uint8_t CHIP_NCH[CHIPS] = {3, 3, 3};

/* ---- instruments: what a note sounds like, per chip and channel */
typedef struct {
    uint8_t wave, duty;                         /* wave: GB wave preset; duty: pulse duty */
    uint8_t atk, dec, sus, rel;                 /* 0..15: ticks to rise, ticks to fall (x2), level, ticks to fade (x2) */
    uint8_t vib_spd, vib_dep;                   /* vibrato: speed, depth in 1/16 semitone */
    int8_t sweep;                               /* pitch change per tick, 1/64 semitone */
    uint8_t pan;                                /* 0 left, 1 centre, 2 right (Game Boy) */
    uint8_t vol;                                /* 0..15, the channel's level */
} inst_t;
#define I(w, d, a, de, s, r, vs, vd, sw, p) {w, d, a, de, s, r, vs, vd, sw, p, 15}
static inst_t inst[CHIPS][3] = {
    {I(0, 2, 0, 5, 9, 4, 5, 0, 0, 1), I(0, 1, 0, 5, 9, 4, 5, 0, 0, 1), I(0, 0, 0, 0, 15, 2, 5, 0, 0, 1)},
    {I(0, 2, 0, 5, 9, 4, 5, 0, 0, 1), I(0, 1, 0, 5, 9, 4, 5, 0, 0, 1), I(3, 0, 0, 0, 15, 2, 5, 0, 0, 1)},
    {I(0, 0, 0, 6, 14, 4, 5, 4, 0, 1), I(0, 0, 0, 2, 0, 2, 0, 0, 0, 1), I(0, 0, 0, 0, 15, 3, 0, 0, 0, 1)},   /* SMS: lead, pluck, bass */
};

/* ---- the event queue: main loop -> audio ISR (one writer each side) */
enum { EV_ON, EV_OFF, EV_PITCH };               /* EV_PITCH: a new note on a sounding voice, without restarting it (arps) */
static struct { uint8_t op, ch, a, b; } evq[32];
static volatile uint32_t ev_w, ev_r;
static void ev_push(uint32_t op, uint32_t ch, uint32_t a, uint32_t b)
{
    uint32_t w = ev_w;
    if (w - ev_r >= 32u) return;
    evq[w & 31u].op = (uint8_t)op; evq[w & 31u].ch = (uint8_t)ch; evq[w & 31u].a = (uint8_t)a; evq[w & 31u].b = (uint8_t)b;
    ev_w = w + 1u;
}
static void chip_note(uint32_t chip, uint32_t ch, uint32_t note, uint32_t vel) { ev_push(EV_ON, chip << 2 | ch, note, vel); }
static void chip_off(uint32_t chip, uint32_t ch) { ev_push(EV_OFF, chip << 2 | ch, 0, 0); }

/* ---- per-channel note state (tick) */
enum { ST_ATK, ST_DEC, ST_SUS, ST_REL, ST_IDLE };
static struct {
    uint8_t on, note, vel, stage, vph;
    uint16_t env;                               /* Q8, 0..15 */
    int32_t bend;                               /* Q8 semitones from sweep */
    inst_t in;
} cs[CHIPS][4];
#define AWAKE 30u                               /* ticks a chip keeps rendering after its last voice ends */
static uint8_t awake[CHIPS], booted;
static uint32_t tick_left, tick_len = FS / 60u;

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
static volatile uint32_t analog_mode[CHIPS] = {AN_HW, AN_HW, 0};
typedef struct { int32_t x1, y1; } hpf_t;
static hpf_t hp_nes[2], hp_gb[2];               /* NES: two stages; Game Boy: left, right */
static int32_t lp_nes;
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
static void post(uint32_t ch, int32_t *out, uint32_t n)
{
    uint32_t i, m = analog_mode[ch];
    if (ch >= CHIP_SMS) return;
    for (i = 0; i < n; i++) {
        int32_t l = out[2u * i], r = out[2u * i + 1u];
        if (ch == CHIP_NES) {
            if (m == AN_HW) l = lpf(&lp_nes, hpf(&hp_nes[1], hpf(&hp_nes[0], l, B_NES_HP90), B_NES_HP440), A_NES_LP14K);
            else if (m == AN_ALT) l = hpf(&hp_nes[0], l, B_FAM_HP37);
            else l = hpf(&hp_nes[0], l, B_DC_HP14);
            r = l;
        } else {
            int32_t b = m == AN_CLEAN ? B_DC_HP14 : B_GB_HPDMG;
            l = hpf(&hp_gb[0], l, b); r = hpf(&hp_gb[1], r, b);
        }
        out[2u * i] = l; out[2u * i + 1u] = r;
    }
}

/* ---- NES: 2 pulse, triangle */
static const uint8_t NES_DUTY[4] = {0x40, 0x60, 0x78, 0x9F};     /* 8 steps, bit = step */
static struct {
    uint32_t ph[3], inc[3];
    uint8_t duty[2], vol[2], tri_on;
} nes;
static void s_nes(uint32_t c, uint32_t pq, uint32_t vol, const inst_t *in)
{
    uint32_t t;
    if (c < 2u) { t = interp(NES_PT, pq); nes.inc[c] = K_NES / (t + 1u); nes.duty[c] = in->duty & 3u; nes.vol[c] = (uint8_t)vol; }
    else { t = interp(NES_TT, pq); nes.inc[2] = (K_NES / (t + 1u)) >> 1; nes.tri_on = vol > 0u; }
}
static void r_nes(int32_t *out, uint32_t n)
{
    uint32_t i, s;
    for (i = 0; i < n; i++) {
        int32_t acc = 0, m;
        for (s = 0; s < OS; s++) {
            uint32_t p1, p2, tr = 0, st;
            nes.ph[0] += nes.inc[0]; nes.ph[1] += nes.inc[1]; nes.ph[2] += nes.inc[2];
            p1 = (NES_DUTY[nes.duty[0]] >> (nes.ph[0] >> 29)) & 1u ? nes.vol[0] : 0u;
            p2 = (NES_DUTY[nes.duty[1]] >> (nes.ph[1] >> 29)) & 1u ? nes.vol[1] : 0u;
            if (nes.tri_on) { st = nes.ph[2] >> 27; tr = st < 16u ? 15u - st : st - 16u; }
            acc += NES_PULSE[p1 + p2] + NES_TND[3u * tr];
        }
        m = acc >> 2;
        out[2u * i] += m; out[2u * i + 1u] += m;
    }
}

/* ---- Game Boy: 2 pulse, wave; each channel panned left, centre or right */
static const uint8_t GB_DUTY[4] = {0x01, 0x81, 0x87, 0x7E};
static struct {
    uint32_t ph[3], inc[3];
    const uint8_t *wav;
    uint8_t duty[2], vol[2], wshift, pan[3];
} gb;
static void s_gb(uint32_t c, uint32_t pq, uint32_t vol, const inst_t *in)
{
    uint32_t x;
    gb.pan[c] = in->pan;
    if (c < 2u) { x = interp(GB_PX, pq); gb.inc[c] = K_GB / (2048u - x); gb.duty[c] = in->duty & 3u; gb.vol[c] = (uint8_t)vol; }
    else {
        x = interp(GB_WX, pq); gb.inc[2] = (K_GB / (2048u - x)) >> 1; gb.wav = GB_WAVES[in->wave & 3u];
        gb.wshift = vol >= 12u ? 0u : vol >= 8u ? 1u : vol >= 4u ? 2u : 8u;   /* the four volumes of the wave channel */
    }
}
static void r_gb(int32_t *out, uint32_t n)
{
    uint32_t i, s;
    if (!gb.wav) gb.wav = GB_WAVES[0];
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (s = 0; s < OS; s++) {
            int32_t v[3];
            uint32_t c;
            gb.ph[0] += gb.inc[0]; gb.ph[1] += gb.inc[1]; gb.ph[2] += gb.inc[2];
            v[0] = (GB_DUTY[gb.duty[0]] >> (gb.ph[0] >> 29)) & 1u ? gb.vol[0] : 0;
            v[1] = (GB_DUTY[gb.duty[1]] >> (gb.ph[1] >> 29)) & 1u ? gb.vol[1] : 0;
            v[2] = gb.wshift < 8u ? gb.wav[gb.ph[2] >> 27] >> gb.wshift : 0;
            for (c = 0; c < 3u; c++) {
                if (gb.pan[c] != 2u) l += v[c];
                if (gb.pan[c] != 0u) r += v[c];
            }
        }
        out[2u * i] += l * 150;      /* the sum of 4 steps x 150 = the average x 600 */
        out[2u * i + 1u] += r * 150;
    }
}

/* ---- Master System PSG (SN76489): 3 square tones */
static struct {
    uint32_t ph[3], inc[3];
    uint8_t vol[3];
} sms;
static void s_sms(uint32_t c, uint32_t pq, uint32_t vol, const inst_t *in)
{
    (void)in;
    sms.vol[c] = (uint8_t)vol;
    sms.inc[c] = K_SMS / interp(SMS_N, pq);
}
static void r_sms(int32_t *out, uint32_t n)
{
    uint32_t i, s, c;
    for (i = 0; i < n; i++) {
        int32_t acc = 0, m;
        for (s = 0; s < OS; s++)
            for (c = 0; c < 3u; c++) {
                sms.ph[c] += sms.inc[c];
                acc += sms.ph[c] >> 31 ? SMS_AMP[sms.vol[c]] : -SMS_AMP[sms.vol[c]];
            }
        m = acc >> 2;
        out[2u * i] += m; out[2u * i + 1u] += m;
    }
}

/* ---- the 60 Hz tick: envelope, vibrato and sweep, then the chip's registers */
static void chip_clear(uint32_t ch)
{
    uint32_t c;
    memset(cs[ch], 0, sizeof cs[ch]);
    for (c = 0; c < 4u; c++) cs[ch][c].stage = ST_IDLE;
    switch (ch) {
    case CHIP_NES: memset(&nes, 0, sizeof nes); memset(hp_nes, 0, sizeof hp_nes); lp_nes = 0; break;
    case CHIP_GB: memset(&gb, 0, sizeof gb); memset(hp_gb, 0, sizeof hp_gb); break;
    default: memset(&sms, 0, sizeof sms); break;
    }
}
static uint32_t tick_chip(uint32_t ch)           /* returns 1 while any voice still sounds */
{
    uint32_t c, busy = 0;
    for (c = 0; c < CHIP_NCH[ch]; c++) {
        const inst_t *in = &cs[ch][c].in;
        int32_t pq, lfo = 0;
        uint32_t vol;
        switch (cs[ch][c].stage) {
        case ST_ATK:
            cs[ch][c].env += in->atk ? 3840u / in->atk : 3840u;
            if (cs[ch][c].env >= 3840u) { cs[ch][c].env = 3840u; cs[ch][c].stage = ST_DEC; }
            break;
        case ST_DEC:
            if (cs[ch][c].env > (uint32_t)in->sus * 256u) {
                uint32_t d = 3840u / ((uint32_t)in->dec * 2u + 1u);
                cs[ch][c].env = cs[ch][c].env - (uint32_t)in->sus * 256u > d ? (uint16_t)(cs[ch][c].env - d) : (uint16_t)(in->sus * 256u);
            }
            if (cs[ch][c].env <= (uint32_t)in->sus * 256u) cs[ch][c].stage = ST_SUS;
            break;
        case ST_REL: {
            uint32_t d = 3840u / ((uint32_t)in->rel * 2u + 1u);
            if (cs[ch][c].env > d) cs[ch][c].env = (uint16_t)(cs[ch][c].env - d); else { cs[ch][c].env = 0; cs[ch][c].stage = ST_IDLE; }
            break; }
        default: break;
        }
        cs[ch][c].bend += (int32_t)in->sweep * 4;
        if (in->vib_dep) { cs[ch][c].vph = (uint8_t)(cs[ch][c].vph + in->vib_spd * 8u); lfo = (VIB[cs[ch][c].vph >> 3] * (int32_t)in->vib_dep) >> 3; }
        pq = ((int32_t)cs[ch][c].note << 8) + cs[ch][c].bend + lfo;
        pq = pq < 0 ? 0 : pq > (127 << 8) ? 127 << 8 : pq;
        vol = ((uint32_t)cs[ch][c].env * ((uint32_t)cs[ch][c].vel + 1u)) >> 15;
        if (vol > 15u) vol = 15u;
        vol = (vol * ((uint32_t)in->vol + 1u)) >> 4;
        switch (ch) {
        case CHIP_NES: s_nes(c, (uint32_t)pq, vol, in); break;
        case CHIP_GB: s_gb(c, (uint32_t)pq, vol, in); break;
        default: s_sms(c, (uint32_t)pq, vol, in); break;
        }
        busy |= cs[ch][c].stage != ST_IDLE;
    }
    return busy;
}
static void tick(void)
{
    uint32_t ch;
    for (ch = 0; ch < CHIPS; ch++) {
        if (!awake[ch]) continue;
        if (tick_chip(ch)) awake[ch] = AWAKE;
        else if (!--awake[ch]) chip_clear(ch);   /* silent for a while: stop rendering, start clean next time */
    }
}
static void ev_do(uint32_t op, uint32_t id, uint32_t a, uint32_t b)
{
    uint32_t ch = id >> 2, c = id & 3u;
    if (ch >= CHIPS || c >= CHIP_NCH[ch]) return;
    if (op == EV_ON) {
        awake[ch] = AWAKE;
        cs[ch][c].in = inst[ch][c];
        cs[ch][c].on = 1; cs[ch][c].note = (uint8_t)a; cs[ch][c].vel = (uint8_t)b; cs[ch][c].bend = 0; cs[ch][c].vph = 0;
        cs[ch][c].stage = ST_ATK; cs[ch][c].env = 0;
        if (!cs[ch][c].in.atk) { cs[ch][c].env = 3840u; cs[ch][c].stage = ST_DEC; }
        tick_chip(ch);                           /* sound at once, not up to 16 ms later */
    } else if (op == EV_PITCH) {
        if (awake[ch] && cs[ch][c].on) cs[ch][c].note = (uint8_t)a;
    } else if (awake[ch]) {
        cs[ch][c].on = 0;
        cs[ch][c].stage = ST_REL;
    }
}
#include "seq.c"

/* the app's audio entry: events, the tick every 1/60 s (sample accurate), then each sounding chip into the mix */
static void chip_render(int32_t *out, uint32_t n)
{
    static int32_t mix[2u * 256u];
    uint32_t i = 0, ch, k;
    if (!booted) { for (ch = 0; ch < CHIPS; ch++) chip_clear(ch); booted = 1; }
    while (ev_r != ev_w) {
        ev_do(evq[ev_r & 31u].op, evq[ev_r & 31u].ch, evq[ev_r & 31u].a, evq[ev_r & 31u].b);
        ev_r++;
    }
    while (i < n) {
        uint32_t run;
        while (seq_run && !seq_left) { seq_clock(); seq_left = seq_period(); }
        run = n - i < tick_left ? n - i : tick_left;
        if (seq_run && run > seq_left) run = seq_left;
        if (run > 256u) run = 256u;
        for (ch = 0; ch < CHIPS; ch++) {
            if (!awake[ch]) continue;
            memset(mix, 0, 8u * run);
            switch (ch) {
            case CHIP_NES: r_nes(mix, run); break;
            case CHIP_GB: r_gb(mix, run); break;
            default: r_sms(mix, run); break;
            }
            post(ch, mix, run);
            for (k = 0; k < 2u * run; k++) out[2u * i + k] += mix[k];
        }
        i += run; tick_left -= run;
        if (seq_run) seq_left -= run;
        if (!tick_left) { tick_left = tick_len; tick(); }
    }
}

/* what the screen shows: 0..15 per channel */
static uint32_t chip_level(uint32_t ch, uint32_t c)
{
    if (ch >= CHIPS || c >= 4u) return 0;
    return (uint32_t)cs[ch][c].env >> 8;
}
