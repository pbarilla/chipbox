/* SPDX-License-Identifier: GPL-3.0-only
 * The MOS 6581 / 8580 SID, included by chips.c: one channel is one voice (an oscillator, an envelope) with a filter of its own.
 * The oscillator follows reSID (Dag Lem, GPL-2.0-or-later): a 24 bit accumulator, pulse = the top 12 bits against a 12 bit width, a 23 bit noise register
 * clocked by accumulator bit 19, the combined waveforms read from tables sampled from real chips, the 6581's saw + X pulling the accumulator's top bit down,
 * hard sync and ring modulation from a master oscillator the channel brings along. The envelope is the chip's: rate counter, the exponential counter
 * (decay slows at 93, 54, 26, 14, 6), sustain in 16 steps. The filter is a two pole state variable filter fitted to reSID's 6581 and to its 8580.
 * The patch packs ten 4 bit settings into the five bytes wave, duty, wf, wc, wt (SP_*), so the saved projects keep their size. */
#include "sid_tables.h"

enum { SP_WAVE, SP_MODEL, SP_PW, SP_PWM, SP_SYNC, SP_RING, SP_CUT, SP_RES, SP_MODE, SP_FENV };
static uint32_t sid_get(const inst_t *in, uint32_t n) { return (uint32_t)(&in->wave)[n >> 1] >> ((n & 1u) * 4u) & 15u; }
static void sid_put(inst_t *in, uint32_t n, uint32_t v)
{
    uint8_t *b = &in->wave + (n >> 1);
    *b = (uint8_t)((n & 1u) ? (*b & 15u) | (v << 4) : (*b & 0xf0u) | v);
}
static const uint8_t SID_WF[8] = {1, 2, 3, 4, 5, 6, 7, 8};       /* tri, saw, tri + saw, pulse, pulse + tri, pulse + saw, all three, noise: the register's nibble */

enum { SE_ATK, SE_DEC, SE_REL };
static struct {
    uint32_t cyc, cyc2;                         /* a part of a SID cycle left over, Q16 (the envelope's, the oscillator's) */
    uint16_t rc, rp, pwph;                      /* envelope rate counter and its period; the pulse width LFO's phase, Q16 */
    int32_t pwq, dpw;                           /* pulse width, 12 bit in Q8, and its step each sample: it glides from tick to tick, so a sweep has no stairs */
    uint8_t env, st, exc, exp, hold, gate, mdl, wf, mode, res, sus, atk, dec, rel, sync, ring, run;
    int32_t ic1, ic2, a1, a2, a3, kq;           /* the filter: states, coefficients Q14 */
    uint16_t fkey, fg, pw0;                          /* the coefficients' settings (p, res, model); the passband gain Q10 */
} sid[MAXCH];

static void sid_exp(uint32_t c)                 /* the exponential counter's period follows the level */
{
    switch (sid[c].env) {
    case 0xff: sid[c].exp = 1; break;
    case 0x5d: sid[c].exp = 2; break;
    case 0x36: sid[c].exp = 4; break;
    case 0x1a: sid[c].exp = 8; break;
    case 0x0e: sid[c].exp = 16; break;
    case 0x06: sid[c].exp = 30; break;
    case 0x00: sid[c].exp = 1; sid[c].hold = 1; break;
    default: break;
    }
}
static void sid_env(uint32_t c, uint32_t cyc)   /* the envelope, a number of SID cycles on */
{
    int32_t step = (int32_t)sid[c].rp - (int32_t)sid[c].rc;
    if (step <= 0) step += 0x7fff;
    while (cyc) {
        if ((int32_t)cyc < step) { sid[c].rc = (uint16_t)(sid[c].rc + cyc); if (sid[c].rc & 0x8000u) sid[c].rc = (uint16_t)((sid[c].rc + 1u) & 0x7fffu); return; }
        sid[c].rc = 0; cyc -= (uint32_t)step;
        if (sid[c].st == SE_ATK || ++sid[c].exc == sid[c].exp) {
            sid[c].exc = 0;
            if (sid[c].hold) { step = (int32_t)sid[c].rp; continue; }
            if (sid[c].st == SE_ATK) {
                sid[c].env = (uint8_t)(sid[c].env + 1u);
                if (sid[c].env == 0xff) { sid[c].st = SE_DEC; sid[c].rp = SID_RATE[sid[c].dec]; }
            } else if (sid[c].st == SE_DEC) {
                if (sid[c].env == sid[c].sus * 17u) return;
                sid[c].env--;
            } else sid[c].env--;
            sid_exp(c);
        }
        step = (int32_t)sid[c].rp;
    }
}
#define SID_NW(r) (((r >> 20 & 1u) << 11) | ((r >> 18 & 1u) << 10) | ((r >> 14 & 1u) << 9) | ((r >> 11 & 1u) << 8) | ((r >> 9 & 1u) << 7) | ((r >> 5 & 1u) << 6) | ((r >> 2 & 1u) << 5) | ((r & 1u) << 4))       /* eight register bits make the top eight bits of the wave */
static uint32_t sid_busy(uint32_t c) { return sid[c].run && !(sid[c].hold && sid[c].st == SE_REL); }       /* run: the voice has been keyed since it was last cleared (a fresh voice is not busy: the chip may sleep) */

static void sid_set(uint32_t c, uint32_t pq, uint32_t vol)       /* at each tick: pitch, pulse width, filter */
{
    const inst_t *in = &chan[c].in;                 /* live: the knobs work on a held note */
    const uint32_t wfi = sid_get(in, SP_WAVE), mdl = sid_get(in, SP_MODEL) & 1u, sy = sid_get(in, SP_SYNC), rg = sid_get(in, SP_RING);
    const uint32_t mode = sid_get(in, SP_MODE) & 7u, res = sid_get(in, SP_RES);
    uint32_t inc = interp32(SID_INC, pq), p, key;
    osc[c].inc3 = interp32(SID_INC_C, pq);
    int32_t pw = SID_PWK[sid_get(in, SP_PW)], pwm = (int32_t)sid_get(in, SP_PWM);
    sid[c].wf = SID_WF[wfi & 7u]; sid[c].mdl = (uint8_t)mdl; sid[c].mode = (uint8_t)mode; sid[c].res = (uint8_t)res;
    sid[c].sync = sy != 0u; sid[c].ring = rg != 0u;
    sid[c].atk = in->atk & 15u; sid[c].dec = in->dec & 15u; sid[c].sus = in->sus & 15u; sid[c].rel = in->rel & 15u;
    if (inc > 0x7ffffffu) inc = 0x7ffffffu;
    if (sy && rg) { osc[c].inc = (inc >> 4) * SID_SYNC_R[sy]; osc[c].inc2 = (inc >> 4) * SID_RING_R[rg]; }       /* the master at the ring ratio, the slave at the sync ratio */
    else if (sy) { osc[c].inc = (inc >> 4) * SID_SYNC_R[sy]; osc[c].inc2 = inc; }                                 /* the slave over the master, which is the note */
    else if (rg) { osc[c].inc = inc; osc[c].inc2 = (inc >> 4) * SID_RING_R[rg]; }                                 /* the note, ring modulated by a second one */
    else osc[c].inc = inc;
    if (pwm) {                                  /* a triangle round the width */
        int32_t t;
        sid[c].pwph = (uint16_t)(sid[c].pwph + SID_PWM[pwm]);
        t = (int32_t)(sid[c].pwph >> 7) - 256; if (t < 0) t = -t;       /* 0..256..0 -> a triangle, 256 high */
        pw += ((t - 128) * 0x500) >> 7;
        pw = pw < 0x040 ? 0x040 : pw > 0xfc0 ? 0xfc0 : pw;
    }
    sid[c].pw0 = (uint16_t)pw; sid[c].dpw = (pw * 256 - sid[c].pwq) / 735;
    osc[c].vol = (uint8_t)vol;
    p = sid_get(in, SP_CUT) * 8u + 3u + ((uint32_t)sid[c].env * sid_get(in, SP_FENV) * 4u >> 8);
    if (p > 127u) p = 127u;
    key = p | res << 7 | mdl << 11;
    if (key + 1u != sid[c].fkey) {              /* the coefficients of the two pole filter: g = tan(pi f0 / fs), k = 1 / Q */
        const uint32_t q = SID_Q[mdl * 256u + res * 16u + (p >> 3)], g = SID_G[p];
        const uint32_t k = 1048576u / q;
        const uint32_t d = 16384u + ((g * (g + k)) >> 14);
        sid[c].a1 = (int32_t)(268435456u / d); sid[c].a2 = (int32_t)((g * (uint32_t)sid[c].a1) >> 14); sid[c].a3 = (int32_t)((g * (uint32_t)sid[c].a2) >> 14);
        sid[c].kq = (int32_t)k; sid[c].fg = SID_FGAIN[mdl * 16u + res]; sid[c].fkey = (uint16_t)(key + 1u);
    }
}
static void sid_trig(uint32_t c)                /* a key down: the gate rises */
{
    if (sid[c].hold || !sid[c].env) { sid[c].ic1 = sid[c].ic2 = 0; osc[c].ph = osc[c].ph2 = 0; sid[c].pwph = 0; }       /* from silence: a clean start */
    osc[c].rng = 0x7fffffu;
    sid[c].pwq = (int32_t)sid[c].pw0 * 256; sid[c].dpw = 0;
    sid[c].st = SE_ATK; sid[c].rp = SID_RATE[sid[c].atk]; sid[c].hold = 0; sid[c].exc = 0; sid[c].run = 1;
}
static void r_sid(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)
{
    uint32_t i, s, k, cycl = 1;                 /* cycl: how many voices may run the 6581's saw combinations at the chip's clock (they cost five times as much) */
    for (k = 0; k < na; k++) {
        const uint32_t c = act[k];
        if (vc[c].trig) { vc[c].trig = 0; sid_trig(c); sid[c].gate = 1; }       /* the gate's edges */
        else if (sid[c].gate && !vc[c].on) { sid[c].gate = 0; sid[c].st = SE_REL; sid[c].rp = SID_RATE[sid[c].rel]; }
    }
    for (k = 0; k < na; k++) {
        const uint32_t c = act[k], mdl = sid[c].mdl, wf = sid[c].wf, syn = sid[c].sync, rng = sid[c].ring, mode = sid[c].mode;
        const uint32_t rmask = rng && !(wf & 2u) ? 0x80000000u : 0u, both = syn || rng;
        const uint32_t tab = (wf == 3u ? 0u : wf == 5u ? 1u : wf == 6u ? 2u : 3u) * 4096u + mdl * 16384u;
        const uint32_t percyc = !mdl && (wf == 3u || wf == 6u || wf == 7u) && !both && cycl && cycl--;
        const uint32_t inc = osc[c].inc, inc2 = osc[c].inc2, inc3 = osc[c].inc3, vol = osc[c].vol;
        const int16_t *wd = &SID_WDAC[mdl * 4096u];
        const uint16_t *ed = &SID_EDAC[mdl * 256u];
        const int32_t a1 = sid[c].a1, a2 = sid[c].a2, a3 = sid[c].a3, kq = sid[c].kq, fg = (int32_t)sid[c].fg, dpw = sid[c].dpw;
        const uint32_t stp = both ? 2u : 1u;
        const uint32_t pr = (0xfffu << 16) / ((inc * OS >> 8) | 1u);          /* for the pulse's share: 4095 over a sample's phase step, Q16 */
        uint32_t a = osc[c].ph, m = osc[c].ph2, r = osc[c].rng, cyc2 = sid[c].cyc2, e = ed[sid[c].env];
        int32_t pwq = sid[c].pwq, ic1 = sid[c].ic1, ic2 = sid[c].ic2;
        uint32_t nw = SID_NW(r);
        for (i = 0; i < n; i++) {
            const uint32_t pw = (uint32_t)(pwq += dpw) >> 8;
            int32_t sum = 0, x;
            if (!(i & 3u)) {                    /* the envelope, four samples at a time (about 11 kHz) */
                uint32_t cy = (sid[c].cyc & 0xffffu) + SID_CYC_FS * 4u;
                sid[c].cyc = cy; sid_env(c, cy >> 16); e = ed[sid[c].env];
            }
            if (!both && !percyc && (wf == 2u || wf == 1u || (wf == 4u && !syn))) {          /* the plain waves, four steps in a row */
                if (wf == 2u) { a += inc; sum = wd[a >> 20]; a += inc; sum += wd[a >> 20]; a += inc; sum += wd[a >> 20]; a += inc; sum += wd[a >> 20]; }
                else if (wf == 1u) {
                    uint32_t ix;
                    a += inc; ix = a >> 20; sum = wd[((ix & 0x800u ? ~ix : ix) << 1) & 0xffeu];
                    a += inc; ix = a >> 20; sum += wd[((ix & 0x800u ? ~ix : ix) << 1) & 0xffeu];
                    a += inc; ix = a >> 20; sum += wd[((ix & 0x800u ? ~ix : ix) << 1) & 0xffeu];
                    a += inc; ix = a >> 20; sum += wd[((ix & 0x800u ? ~ix : ix) << 1) & 0xffeu];
                } else {                        /* the pulse: the share of this sample that is high (the box filter), so the edges do not jitter when the width sweeps */
                    const uint32_t P = pw << 20, a1 = a + inc * OS;
                    uint32_t ov;
                    if (a1 >= a) ov = a1 > P ? a1 - (a > P ? a : P) : 0u;
                    else ov = (0u - (a > P ? a : P)) + (a1 > P ? a1 - P : 0u);
                    a = a1;
                    sum = wd[((ov >> 8) * pr >> 16) > 0xfffu ? 0xfffu : (ov >> 8) * pr >> 16] * (int32_t)OS;
                }
            } else if (both && (wf == 1u || wf == 2u)) {          /* sync and ring on a triangle or a saw: two steps a sample */
                for (s = 0; s < OS; s += 2) {
                    const uint32_t mn = m + inc2 * 2u;
                    uint32_t ix;
                    a += inc * 2u;
                    if (syn && (~m & mn & 0x80000000u)) a = 0;
                    m = mn;
                    ix = (a ^ (~m & rmask)) >> 20;
                    sum += wd[wf == 2u ? ix : ((ix & 0x800u ? ~ix : ix) << 1) & 0xffeu] * 2;
                }
            } else if (!both && !percyc && wf != 8u) {          /* the sampled combined waves: two steps a sample, they cost the most */
                for (s = 0; s < OS; s += 2) {
                    uint32_t w;
                    a += inc * 2u;
                    w = (uint32_t)SID_WAVE[tab + (a >> 20)] << 4;
                    if (wf & 4u) w &= (a >> 20) >= pw ? 0xfffu : 0u;
                    if (!mdl && (wf & 2u)) a &= (w << 20) | 0x7fffffffu;
                    sum += wd[w] * 2;
                }
            } else if (wf == 8u && !both) {     /* noise: the register moves on accumulator bit 19's rising edge; the wave is worked out then, not every step */
                for (s = 0; s < OS; s++) {
                    const uint32_t old = a;
                    a += inc;
                    if (~old & a & 0x08000000u) { r = (r << 1 | (((r >> 22) ^ (r >> 17)) & 1u)) & 0x7fffffu; nw = SID_NW(r); }
                    sum += wd[nw];
                }
            } else for (s = 0; s < OS; s += stp) {          /* sync and ring: two steps a sample */
                const uint32_t old = a;
                uint32_t w;
                if (percyc) {                   /* the 6581's saw + X pulls the accumulator down every cycle, so it is run at the chip's clock */
                    uint32_t j, fr = (cyc2 & 0xffffu) + SID_CYC_OS;
                    cyc2 = fr;
                    for (j = fr >> 16; j; j--) {
                        a += inc3;
                        w = (uint32_t)SID_WAVE[tab + (a >> 20)] << 4;
                        if (wf & 4u) w &= (a >> 20) >= pw ? 0xfffu : 0u;
                        a &= (w << 20) | 0x7fffffffu;
                    }
                    sum += wd[w];
                    continue;
                }
                a += inc * stp;
                if (both) {
                    const uint32_t mn = m + inc2 * stp;
                    if (syn && (~m & mn & 0x80000000u)) a = 0;
                    m = mn;
                }
                if (wf == 8u) {
                    if (~old & a & 0x08000000u) { r = (r << 1 | (((r >> 22) ^ (r >> 17)) & 1u)) & 0x7fffffu; nw = SID_NW(r); }
                    w = nw;
                } else if (wf == 4u) {          /* the pulse: the share of this step that is high, so the edges do not jitter when the width sweeps */
                    const uint32_t P = pw << 20;
                    if (syn) w = a >= P ? 0xfffu : 0u;
                    else if (a >= old) w = old >= P ? 0xfffu : a <= P ? 0u : ((a - P) >> 7) * 0xfffu / (inc >> 7 | 1u);
                    else { const uint32_t ov = old >= P ? 0u - old : 0u - P; w = (ov >> 7) * 0xfffu / (inc >> 7 | 1u); }
                    if (w > 0xfffu) w = 0xfffu;
                } else {
                    const uint32_t ix = (a ^ (~m & rmask)) >> 20;
                    if (wf == 1u) w = ((ix & 0x800u ? ~ix : ix) << 1) & 0xffeu;
                    else if (wf == 2u) w = ix;
                    else {
                        w = (uint32_t)SID_WAVE[tab + ix] << 4;
                        if (wf & 4u) w &= (a >> 20) >= pw ? 0xfffu : 0u;
                        if (!mdl && (wf & 2u)) a &= (w << 20) | 0x7fffffffu;       /* the 6581: saw with another wave drags the accumulator's top bit down */
                    }
                }
                sum += wd[w] * (int32_t)stp;
            }
            x = (int32_t)(((int32_t)e * sum * 5) >> 11);
            if (mode) {                         /* the filter */
                const int32_t v3 = x - ic2;
                const int32_t v1 = (a1 * ic1 + a2 * v3) >> 14;
                const int32_t v2 = ic2 + ((a2 * ic1 + a3 * v3) >> 14);
                int32_t y = 0;
                ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
                if (mode & 1u) y += v2;
                if (mdl) {                      /* the 8580: the ideal filter */
                    if (mode & 2u) y += v1;
                    if (mode & 4u) y += x - ((kq * v1) >> 14) - v2;
                } else {                        /* the 6581's band and high pass are weaker, and leak the input */
                    if (mode & 2u) y += ((v1 * 5) >> 3) + ((x * 3) >> 4);
                    if (mode & 4u) y += (((x - ((kq * v1) >> 14) - v2) * 3) >> 3) + (x >> 4);
                }
                x = (y * fg) >> 10;
            }
            x = (x * (int32_t)vol * 273) >> 12;
            out[2u * i] += x; out[2u * i + 1u] += x;
        }
        osc[c].ph = a; osc[c].ph2 = m; osc[c].rng = r; sid[c].cyc2 = cyc2; sid[c].pwq = pwq; sid[c].ic1 = ic1; sid[c].ic2 = ic2;
    }
}
