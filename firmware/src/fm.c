/* SPDX-License-Identifier: GPL-3.0-only */
/* The FM chips, operator by operator: OPM, OPN family, OPZ, OPL3 (four operators), OPL2, OPL1, OPLL and VRC7 (two). Included by chips.c.
 * A channel holds the chip's own register set (fmp_t: per operator TL, MUL, DT1, DT2, KS, AR, D1R, D1L, D2R, RR, AM, ...; per channel ALG, FB, LFO ...).
 * Each operator has its own envelope: the real rate law (envelope.c of the chips: rate = 2R + key scale, or 4R + key scale on the OPL), run at our 60 Hz tick, the level smoothed
 * per sample. Levels are attenuations in 0.09375 dB units (the chips' 10 bit envelope), Q8 in the voice. Integers only. */

/* what a family has: the largest value of every operator and channel field; 0 = the chip has no such field */
static const uint8_t FMXO[FAMS][NOPF] = {
    /*        TL  MUL DT1 DT2 KS  AR  D1R D1L D2R RR  AM  WAV FIN SHF FIX KSL EGT KSR VIB */
    /* -    */ {0},
    /* OPM  */ {127, 15, 6, 3, 3, 31, 31, 15, 31, 15, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    /* OPN  */ {127, 15, 6, 0, 3, 31, 31, 15, 31, 15, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    /* OPN3 */ {127, 15, 6, 0, 3, 31, 31, 15, 31, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* OPZ  */ {127, 15, 6, 3, 3, 31, 31, 15, 31, 15, 1, 7, 15, 3, 1, 0, 0, 0, 0},
    /* OPL3 */ {63, 15, 0, 0, 0, 15, 15, 15, 0, 15, 1, 7, 0, 0, 0, 3, 1, 1, 1},
    /* OPL2 */ {63, 15, 0, 0, 0, 15, 15, 15, 0, 15, 1, 3, 0, 0, 0, 3, 1, 1, 1},
    /* OPL1 */ {63, 15, 0, 0, 0, 15, 15, 15, 0, 15, 1, 0, 0, 0, 0, 3, 1, 1, 1},
    /* OPLL */ {63, 15, 0, 0, 0, 15, 15, 15, 0, 15, 1, 1, 0, 0, 0, 3, 1, 1, 1},
};
static const uint8_t FMXC[FAMS][NCHF] = {
    /*        ALG FB PAN PMS AMS LFS LFW PMD AMD DAM DVB INST PED EDIT */
    /* -    */ {0},
    /* OPM  */ {7, 7, 2, 7, 3, 255, 3, 127, 127, 0, 0, 0, 0, 0},
    /* OPN  */ {7, 7, 2, 7, 3, 8, 0, 0, 0, 0, 0, 0, 0, 0},
    /* OPN3 */ {7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* OPZ  */ {7, 7, 0, 7, 3, 255, 3, 127, 127, 0, 0, 0, 0, 0},
    /* OPL3 */ {3, 7, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    /* OPL2 */ {1, 7, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    /* OPL1 */ {1, 7, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    /* OPLL */ {0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 1, 1},
};
#define FM_NOPS(fam) ((fam) >= FAM_OPL2 ? 2u : 4u)
#define FM_OPLFAM(fam) ((fam) >= FAM_OPL3)
static const uint8_t FM_CARRY[12] = {8, 8, 8, 8, 10, 14, 14, 15, 8, 9, 10, 13};     /* the carriers of an algorithm, a bit an operator (OP1 = bit 0); 8..11 are the OPL3's four connections */
static const uint8_t FM_NORM[12] = {255, 255, 255, 255, 128, 85, 85, 64, 166, 165, 180, 100};         /* one over the number of carriers (OPM family); the OPL3's four follow the chip's own levels, measured */
static const uint16_t FM_DT2F[4] = {256, 362, 402, 443};                  /* OPM / OPZ DT2: 0, +600, +781, +950 cents, Q8 */
static const uint16_t FM_PMQ_OPM[8] = {0, 13, 26, 51, 128, 256, 1024, 1792};
static const uint16_t FM_PMQ_OPN[8] = {0, 9, 17, 26, 36, 51, 102, 205};      /* the YM2612 family's: 0, 3.4, 6.7, 10, 14, 20, 40, 80 cents */
static const uint16_t FM_AMU_OPM[4] = {0, 255, 510, 1020};                    /* the AM depth at AMD 127 in attenuation units (0, 23.9, 47.8, 95.6 dB) */
static const uint16_t FM_AMU_OPN[4] = {0, 15, 63, 126};                       /* 0, 1.4, 5.9, 11.8 dB */
static const uint16_t FM_LFO_OPN[8] = {382, 533, 577, 611, 660, 923, 4611, 6922};      /* the YM2612's LFO in 1/100 Hz */
static const uint8_t FM_NCODE_OPM[12] = {3, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3};          /* the key code's note part, by semitone from C */
static const uint8_t FM_NCODE_OPN[12] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3};

static uint32_t fm_alg(uint32_t fam, const fmp_t *p)       /* the wiring: 0..7 OPM family, 8..11 the OPL3's connections, 0 / 1 the two operator chips' FM / additive */
{
    return fam == FAM_OPL3 ? 8u + p->ch[CF_ALG] : p->ch[CF_ALG];
}
static uint32_t fm_carry(uint32_t fam, uint32_t alg) { return FM_NOPS(fam) == 2u ? (alg ? 3u : 2u) : FM_CARRY[alg > 11u ? 0u : alg]; }       /* which operators reach the output, a bit each */
static uint32_t fm_foc;                         /* the screen: what the knobs edit: 0 the channel, 1.. an operator (SEL turns it) */
typedef struct { uint32_t ph[4], inc[4]; int32_t lvt[4], lvc[4], y1, y2; uint32_t fbm; const int16_t *wt[4]; uint8_t wv[4], alg, ns, wsh; } fmo_t;
static fmo_t fmo[MAXCH];                        /* what the renderer reads: phases, increments, levels (target and smoothed) */

static uint32_t fm_grp(uint32_t kind)           /* the envelope clock: 0 OPM / OPZ, 1 YM2612, 2 YM2203 / YM2608 / YM2610, 3 the OPL family */
{
    switch (kind) {
    case KD_OPM: case KD_OPZ: return 0u;
    case KD_MD: return 1u;
    case KD_OPL: case KD_OPL1: case KD_OPL3: case KD_OPLL: case KD_VRC7: return 3u;
    default: return 2u;
    }
}
static uint32_t fm_bits(uint32_t mx) { uint32_t b = 0; while (mx) { b++; mx >>= 1; } return b; }
static uint32_t fm_plen(uint32_t kind)          /* the bytes a channel's patch takes in a project (bit packed, the fields the chip has) */
{
    const uint32_t fam = fm_fam(kind), nop = FM_NOPS(fam);
    uint32_t f, bits = 0;
    if (fam == FAM_NONE) return 0u;
    for (f = 0; f < NOPF; f++) bits += nop * fm_bits(FMXO[fam][f]);
    for (f = 0; f < NCHF; f++) bits += fm_bits(FMXC[fam][f]);
    return (bits + 7u) / 8u;
}
static void fm_pack(uint32_t kind, const fmp_t *p, uint8_t *out)
{
    const uint32_t fam = fm_fam(kind), nop = FM_NOPS(fam);
    uint32_t j, f, pos = 0, len = fm_plen(kind);
    memset(out, 0, len);
#define FM_PUT(v, mx) do { uint32_t w_ = fm_bits(mx), b_; for (b_ = 0; b_ < w_; b_++, pos++) if (((v) >> b_) & 1u) out[pos >> 3] |= (uint8_t)(1u << (pos & 7u)); } while (0)
    for (j = 0; j < nop; j++) for (f = 0; f < NOPF; f++) { const uint32_t v = p->op[j][f]; FM_PUT(v, FMXO[fam][f]); }
    for (f = 0; f < NCHF; f++) { const uint32_t v = p->ch[f]; FM_PUT(v, FMXC[fam][f]); }
#undef FM_PUT
}
static void fm_unpack(uint32_t kind, fmp_t *p, const uint8_t *in)       /* every field clamped to what the chip allows */
{
    const uint32_t fam = fm_fam(kind), nop = FM_NOPS(fam);
    uint32_t j, f, pos = 0;
#define FM_GET(dst, mx) do { uint32_t w_ = fm_bits(mx), b_, v_ = 0; for (b_ = 0; b_ < w_; b_++, pos++) v_ |= (uint32_t)((in[pos >> 3] >> (pos & 7u)) & 1u) << b_; (dst) = (uint8_t)(v_ > (mx) ? (mx) : v_); } while (0)
    for (j = 0; j < nop; j++) for (f = 0; f < NOPF; f++) FM_GET(p->op[j][f], FMXO[fam][f]);
    for (f = 0; f < NCHF; f++) FM_GET(p->ch[f], FMXC[fam][f]);
#undef FM_GET
}

/* the OPLL / VRC7: an instrument's 8 bytes (b0 / b1 flags and multiple of modulator / carrier, b2 modulator key scale and level, b3 carrier key scale, waves and feedback, b4 / b5 attack and decay,
 * b6 / b7 sustain and release) become the two operators; the carrier has no level of its own (the channel's VOL is the chip's volume nibble) */
static void fm_opll_load(fmp_t *p, uint32_t kind, uint32_t n)
{
    const uint8_t *b = OPLL_RAW + ((kind == KD_VRC7 ? 15u : 0u) + (n > 14u ? 14u : n)) * 8u;
    uint32_t j;
    for (j = 0; j < 2u; j++) {
        uint8_t *o = p->op[j];
        o[OF_AM] = b[j] >> 7 & 1u; o[OF_VIB] = b[j] >> 6 & 1u; o[OF_EGT] = b[j] >> 5 & 1u; o[OF_KSR] = b[j] >> 4 & 1u; o[OF_MUL] = b[j] & 15u;
        o[OF_AR] = b[4u + j] >> 4; o[OF_D1R] = b[4u + j] & 15u; o[OF_D1L] = b[6u + j] >> 4; o[OF_RR] = b[6u + j] & 15u;
    }
    p->op[0][OF_KSL] = b[2] >> 6; p->op[0][OF_TL] = b[2] & 63u; p->op[1][OF_KSL] = b[3] >> 6; p->op[1][OF_TL] = 0;
    p->op[0][OF_WAVE] = b[3] >> 3 & 1u; p->op[1][OF_WAVE] = b[3] >> 4 & 1u;
    p->ch[CF_FB] = b[3] & 7u; p->ch[CF_INST] = (uint8_t)(n + 1u); p->ch[CF_EDIT] = 0;
}
static void fm_default(uint32_t kind, fmp_t *p)  /* a plain pluck: a modulator on a carrier, twice; the values a driver would set up (all-zero registers make no sound) */
{
    const uint32_t fam = fm_fam(kind);
    uint32_t j;
    memset(p, 0, sizeof *p);
    for (j = 0; j < 4u; j++) p->op[j][OF_DT1] = FMXO[fam][OF_DT1] ? 3u : 0u;       /* (no detune: the middle of the seven) */
    if (fam == FAM_OPM || fam == FAM_OPN || fam == FAM_OPN3 || fam == FAM_OPZ) {
        for (j = 0; j < 4u; j++) {
            const uint32_t mod = !(j & 1u);
            uint8_t *o = p->op[j];
            o[OF_TL] = mod ? 24u : 0u; o[OF_MUL] = j == 2u ? 2u : 1u; o[OF_AR] = 31u; o[OF_D1R] = mod ? 15u : 18u; o[OF_D1L] = mod ? 15u : 3u; o[OF_RR] = mod ? 7u : 9u;
        }
        p->ch[CF_ALG] = 4u; p->ch[CF_PAN] = FMXC[fam][CF_PAN] ? 1u : 0u;
    } else if (fam == FAM_OPL3) {
        for (j = 0; j < 4u; j++) {
            const uint32_t mod = !(j & 1u);
            uint8_t *o = p->op[j];
            o[OF_TL] = mod ? 24u : 0u; o[OF_MUL] = j == 2u ? 2u : 1u; o[OF_AR] = 15u; o[OF_D1R] = mod ? 6u : 8u; o[OF_D1L] = mod ? 15u : 3u; o[OF_RR] = 8u; o[OF_EGT] = 1u;
        }
        p->ch[CF_ALG] = 2u; p->ch[CF_PAN] = 1u;
    } else if (fam == FAM_OPL2 || fam == FAM_OPL1) {
        uint8_t *m = p->op[0], *c = p->op[1];
        m[OF_TL] = 20u; m[OF_MUL] = 1u; m[OF_AR] = 15u; m[OF_D1R] = 8u; m[OF_D1L] = 8u; m[OF_RR] = 8u;
        c[OF_TL] = 0u; c[OF_MUL] = 1u; c[OF_AR] = 15u; c[OF_D1R] = 5u; c[OF_D1L] = 15u; c[OF_RR] = 6u;
        p->ch[CF_FB] = 3u;
    } else if (fam == FAM_OPLL) fm_opll_load(p, kind, 0u);
}

/* ---- the old settings (knobs RATIO, INDEX, FB, DECAY and the voice's envelope) as an operator patch: projects older than the operator editor load through this */
static uint32_t fm_find_rate(uint32_t g, uint32_t target)     /* the rate 4..63 whose fall (Q8 units a tick) is nearest to target, by ratio; 0 for none */
{
    uint32_t r, best = 4u, bd = 0xFFFFFFFFu;
    if (!target) return 0u;
    for (r = 4u; r < 64u; r++) {
        const uint32_t v = FMR_DEC[g * 64u + r], d = v > target ? v * 256u / target : target * 256u / (v ? v : 1u);
        if (d < bd) { bd = d; best = r; }
    }
    return best;
}
static uint32_t fm_find_atk(uint32_t g, uint32_t k)           /* the rate 4..61 whose attack step (Q16) is nearest */
{
    uint32_t r, best = 4u, bd = 0xFFFFFFFFu;
    for (r = 4u; r < 62u; r++) {
        const uint32_t v = FMR_ATK[g * 64u + r], d = v > k ? v * 256u / (k ? k : 1u) : k * 256u / (v ? v : 1u);
        if (d < bd) { bd = d; best = r; }
    }
    return best;
}
static uint32_t fm_rate_to_r(uint32_t fam, uint32_t r, uint32_t rel)      /* a rate as the register value: 2R, 4R + 2 (release) on the OPM family, 4R on the OPL */
{
    uint32_t v;
    if (!r) return 0u;
    if (FM_OPLFAM(fam)) v = (r + 2u) / 4u;
    else v = rel ? r / 4u : (r + 1u) / 2u;
    return v > (rel || FM_OPLFAM(fam) ? 15u : 31u) ? (rel || FM_OPLFAM(fam) ? 15u : 31u) : v;
}
static void fmc_carrier(fmp_t *p, uint32_t j, uint32_t fam, uint32_t g, uint32_t atk, uint32_t dbps16_dec, uint32_t sus, uint32_t dbps16_rel, uint32_t fq)
{       /* fq: the old envelope's steps were 3 dB at full VOL and velocity 127, smaller below (Q8: the VOL knob and a typical velocity of 110) */
    uint8_t *o = p->op[j];
    const uint32_t xq = 181535u / (atk ? atk : 1u), arate = atk ? fm_find_atk(g, (xq << 12) / ((65536u + xq / 2u) >> 4)) : 62u;        /* the attack's step per tick for a rise in atk ticks: 1 - exp(-2.77 / atk) */
    o[OF_TL] = 0; o[OF_AR] = (uint8_t)(atk ? fm_rate_to_r(fam, arate, 0u) : FMXO[fam][OF_AR]);
    o[OF_D1R] = (uint8_t)fm_rate_to_r(fam, fm_find_rate(g, (dbps16_dec * fq >> 8) * 727u >> 8), 0u);
    o[OF_D1L] = (uint8_t)(((15u - (sus > 15u ? 15u : sus)) * fq + 128u) >> 8); o[OF_D2R] = 0;
    o[OF_RR] = (uint8_t)fm_rate_to_r(fam, fm_find_rate(g, (dbps16_rel * fq >> 8) * 727u >> 8), 1u);
    if (!o[OF_RR]) o[OF_RR] = 1u;
    if (FM_OPLFAM(fam)) o[OF_EGT] = 1u;
}
static void fm_from_classic(uint32_t kind, const inst_t *in, fmp_t *p)
{
    const uint32_t fam = fm_fam(kind), g = fm_grp(kind), nop = FM_NOPS(fam), opl = FM_OPLFAM(fam);
    uint32_t j, i, best = 0, bd = 0xFFFFFFFFu, alg;
    fm_default(kind, p);
    if (fam == FAM_OPLL) {                      /* the ROM instrument; if the old ENV page changed the carrier's envelope, keep that */
        const uint32_t n = in->wave > 14u ? 14u : in->wave, pi = n + (kind == KD_VRC7 ? 15u : 0u);
        fm_opll_load(p, kind, n);
        if (in->atk != OPLL_P[pi].atk || in->dec != OPLL_P[pi].dec || in->sus != OPLL_P[pi].sus || in->rel != OPLL_P[pi].rel) {
            fmc_carrier(p, 1u, fam, g, in->atk, (uint32_t)ENV_EXP[in->dec & 15u] * 11u, in->sus, (uint32_t)ENV_EXP[in->rel & 15u] * 11u, ((uint32_t)in->vol + 1u) * 111u / 8u);
            p->op[1][OF_EGT] = OPLL_P[pi].ceg ? 1u : 0u; p->ch[CF_EDIT] = 1u;
        }
        return;
    }
    for (i = 0; i < 8u; i++) { const uint32_t d = OPLL_FB[i] > FM_FB[in->wc & 15u] ? OPLL_FB[i] - FM_FB[in->wc & 15u] : FM_FB[in->wc & 15u] - OPLL_FB[i]; if (d < bd) { bd = d; best = i; } }
    p->ch[CF_FB] = (uint8_t)best;
    if (fam == FAM_OPL3) alg = 8u + (in->duty & 3u);
    else if (fam == FAM_OPL2) alg = in->duty == 3u ? 1u : 0u;
    else if (fam == FAM_OPL1) alg = in->duty ? 1u : 0u;
    else alg = in->duty & 7u;
    p->ch[CF_ALG] = (uint8_t)(fam == FAM_OPL3 ? alg - 8u : alg);
    if (FMXC[fam][CF_PAN] && fam != FAM_OPL3) p->ch[CF_PAN] = in->pan > 2u ? 2u : in->pan;       /* (the old OPL3 had no pan: that byte was its wave) */
    for (j = 0; j < nop; j++) {
        uint8_t *o = p->op[j];
        const uint32_t car = fm_carry(fam, alg) >> j & 1u;
        const uint32_t dbps_dec = 43200u / (2u * (in->dec & 15u) + 1u), dbps_rel = 43200u / (2u * (in->rel & 15u) + 1u);
        o[OF_MUL] = (uint8_t)(j == 0u ? (in->wave & 15u) : j == 2u ? 2u : 1u);
        if (FMXO[fam][OF_WAVE]) o[OF_WAVE] = (uint8_t)(fam == FAM_OPL3 || fam == FAM_OPZ ? (in->pan & 7u) : (fam == FAM_OPL2 && j == 1u && in->duty < 3u ? in->duty : 0u));
        if (car) fmc_carrier(p, j, fam, g, in->atk, dbps_dec, in->sus, dbps_rel, ((uint32_t)in->vol + 1u) * 111u / 8u);
        else {                                  /* a modulator: the INDEX knob is its level, the DECAY knob its fall to silence */
            const uint32_t tl = in->wf ? 4u * (15u - (in->wf & 15u)) : 127u;
            o[OF_TL] = (uint8_t)(tl > FMXO[fam][OF_TL] ? FMXO[fam][OF_TL] : tl);
            o[OF_AR] = FMXO[fam][OF_AR]; o[OF_D1L] = 15u; o[OF_D2R] = 0u;
            o[OF_D1R] = (uint8_t)fm_rate_to_r(fam, fm_find_rate(g, (uint32_t)FM_DEC[in->wt & 15u] * 12u * 727u >> 8), 0u);
            o[OF_RR] = o[OF_D1R] ? o[OF_D1R] : 7u;
            if (!opl) o[OF_RR] = (uint8_t)(o[OF_D1R] ? (o[OF_D1R] + 1u) / 2u : 7u);
            if (opl) o[OF_EGT] = 1u;
        }
    }
}

/* ---- keys, rates, the LFO */
static uint32_t fm_kc(uint32_t fam, uint32_t note)      /* the key code the chip makes of a note: OPM / OPZ 0..31 (octave, note code), OPN family 0..31, the OPL family 0..15 (block, fnum bit 9) */
{
    if (FM_OPLFAM(fam)) { const uint32_t b = note <= 30u ? 0u : note >= 103u ? 7u : (note - 31u) / 12u + 1u; return 2u * b + (note >= 19u); }
    if (fam == FAM_OPM || fam == FAM_OPZ) { const int32_t o = note >= 13u ? (int32_t)(note - 1u) / 12 - 1 : 0; return (uint32_t)((o > 7 ? 7 : o) * 4) + FM_NCODE_OPM[note % 12u]; }
    { const int32_t o = note >= 12u ? (int32_t)note / 12 - 1 : 0; return (uint32_t)((o > 7 ? 7 : o) * 4) + FM_NCODE_OPN[note % 12u]; }
}
static uint32_t fm_rate(uint32_t fam, uint32_t R, uint32_t rks, uint32_t rel)      /* the 6 bit rate of a register value R and the key scaling */
{
    uint32_t r;
    if (FM_OPLFAM(fam)) { if (!R) return 0u; r = 4u * R + rks; }
    else if (rel) r = 4u * R + 2u + rks;
    else { if (!R) return 0u; r = 2u * R + rks; }
    return r > 63u ? 63u : r;
}
static uint32_t fm_lfo_u(uint32_t w, uint32_t ph, uint32_t n)       /* the OPM / OPZ LFO's wave at a phase, 0..255: saw, square, triangle, noise */
{
    switch (w & 3u) {
    case 0: return ph >> 24;
    case 1: return ph >> 31 ? 255u : 0u;
    case 2: return ph >> 31 ? 255u - ((ph >> 23) & 255u) : (ph >> 23) & 255u;
    default: return ((n * 2654435761u) >> 24) & 255u;
    }
}

#include "opl_waves.h"
/* ---- the voice: key on, the envelope at 60 Hz, then the oscillator for the renderer */
enum { OS_OFF, OS_ATK, OS_DEC, OS_SUS, OS_REL };
#define FM_MAXA (1023u << 8)
static void fm_keyon(uint32_t c)
{
    const uint32_t nop = FM_NOPS(fm_fam(chan[c].kind));
    uint32_t j;
    vc[c].lph = 0;
    for (j = 0; j < nop; j++) { if (vc[c].os[j] == OS_OFF) vc[c].oa[j] = FM_MAXA; vc[c].os[j] = OS_ATK; }
}
static uint32_t fm_env(uint32_t c)              /* one 60 Hz tick of every operator's envelope; 1 while the voice sounds */
{
    const uint32_t kind = chan[c].kind, fam = fm_fam(kind), nop = FM_NOPS(fam), g = fm_grp(kind), opl = FM_OPLFAM(fam), opll = fam == FAM_OPLL;
    const fmp_t *p = &chan[c].fm;
    const uint32_t note = vc[c].note + vc[c].arp_off > 127u ? 127u : vc[c].note + vc[c].arp_off, kc = fm_kc(fam, note), carry = fm_carry(fam, fm_alg(fam, p));
    uint32_t j, alive = 0;
    if (fam == FAM_OPM || fam == FAM_OPZ) vc[c].lph += (((16u + (p->ch[CF_LFS] & 15u)) << (p->ch[CF_LFS] >> 4)) * 3728u);
    else if (p->ch[CF_LFS]) vc[c].lph += (uint32_t)FM_LFO_OPN[(p->ch[CF_LFS] - 1u) & 7u] * 715828u;
    for (j = 0; j < nop; j++) {
        const uint8_t *o = p->op[j];
        const uint32_t rks = opl ? (o[OF_KSR] ? kc : kc >> 2) : kc >> (3u - o[OF_KS]), sl = o[OF_D1L] == 15u ? 992u << 8 : (uint32_t)o[OF_D1L] * 32u << 8;
        uint32_t st = vc[c].os[j], a = vc[c].oa[j], r;
        if (st == OS_OFF) continue;
        if (!vc[c].on && st != OS_REL) st = OS_REL;
        switch (st) {
        case OS_ATK:
            r = fm_rate(fam, o[OF_AR], rks, 0u);
            if (r >= 62u) { a = 0; st = OS_DEC; }
            else if (r) { a -= ((a >> 2) * FMR_ATK[g * 64u + r]) >> 14; if (a < 256u) { a = 0; st = OS_DEC; } }
            break;
        case OS_DEC:
            r = fm_rate(fam, o[OF_D1R], rks, 0u);
            if (a < sl) a += FMR_DEC[g * 64u + r];
            if (a >= sl) { a = sl; st = OS_SUS; }       /* a fast fall can step past the level in one 60 Hz tick: it holds at D1L */
            break;
        case OS_SUS:
            if (opl) { if (!o[OF_EGT]) a += FMR_DEC[g * 64u + fm_rate(fam, o[OF_RR], rks, 0u)]; }       /* percussive: it goes on falling at the release rate */
            else a += FMR_DEC[g * 64u + fm_rate(fam, o[OF_D2R], rks, 0u)];
            break;
        default:
            r = opll ? fm_rate(fam, p->ch[CF_PED] ? 5u : o[OF_EGT] ? o[OF_RR] : 7u, rks, 0u) : fm_rate(fam, o[OF_RR], rks, 1u);
            a += FMR_DEC[g * 64u + r];
            break;
        }
        if (a >= FM_MAXA) { a = FM_MAXA; st = OS_OFF; }
        vc[c].os[j] = (uint8_t)st; vc[c].oa[j] = a;
        if ((carry >> j & 1u) && st != OS_OFF) alive = 1;
    }
    return alive;
}
static void fm_osc(uint32_t c, uint32_t pq, uint32_t vol)      /* each tick, after the envelope: pitch, levels and wiring into the renderer's oscillator */
{
    const uint32_t kind = chan[c].kind, fam = fm_fam(kind), nop = FM_NOPS(fam), opl = FM_OPLFAM(fam), opll = fam == FAM_OPLL;
    const fmp_t *p = &chan[c].fm;
    fmo_t *f = &fmo[c];
    const uint32_t alg = fm_alg(fam, p), carry = fm_carry(fam, alg);
    uint32_t j, maxmod = 0, amu = 0, amd = 127u;
    int32_t pm = 0, vibq = 0, lastq = 0;
    uint32_t lastt = 0;
    if (pq > (127u << 8)) pq = 127u << 8;
    {   const uint32_t note = pq >> 8, kc = fm_kc(fam, note);
        const uint32_t lph = vc[c].lph;
        if (fam == FAM_OPM || fam == FAM_OPZ || (fam == FAM_OPN && p->ch[CF_LFS])) {      /* the LFO: vibrato on every operator, tremolo on those with AM */
            const uint32_t opm = fam != FAM_OPN, u = fm_lfo_u(opm ? p->ch[CF_LFW] : 2u, lph, lph >> 8);
            const uint32_t pms = p->ch[CF_PMS] & 7u, ams = p->ch[CF_AMS] & 3u, pmd = opm ? p->ch[CF_PMD] : 127u;
            amd = opm ? p->ch[CF_AMD] : 127u;
            pm = (int32_t)((uint32_t)(opm ? FM_PMQ_OPM[pms] : FM_PMQ_OPN[pms]) * pmd >> 7) * ((int32_t)u - 128) >> 7;
            amu = ((uint32_t)(opm ? FM_AMU_OPM[ams] : FM_AMU_OPN[ams]) * amd >> 7) * u >> 8;
        } else if (opl && !opll) {              /* the OPL's two free running LFOs: tremolo 3.7 Hz (1 or 4.8 dB), vibrato 6.1 Hz (7 or 14 cents) */
            const uint32_t tp = tre_ph & 0xFFFFu, tri = (tp < 32768u ? tp : 65535u - tp) * 2u;
            amu = (tri * (p->ch[CF_DAM] ? 51u : 11u)) >> 16;
            vibq = ((int32_t)SIN_T[(vib_ph >> 6) & 1023u] * (p->ch[CF_DVB] ? 36 : 18)) >> 14;
        } else if (opll) {                      /* the OPLL's fixed ones: tremolo 3.7 Hz, 4.9 dB; vibrato 6.1 Hz, 11 cents */
            const uint32_t tp = tre_ph & 0xFFFFu, tri = (tp < 32768u ? tp : 65535u - tp) * 2u;
            amu = (tri * 52u) >> 16;
            vibq = ((int32_t)SIN_T[(vib_ph >> 6) & 1023u] * 28) >> 14;
        }
        for (j = 0; j < nop; j++) {
            const uint8_t *o = p->op[j];
            int32_t q = (int32_t)pq + pm + (opl && o[OF_VIB] ? vibq : 0);
            uint32_t t, inc, a, mulx, lv;
            if (q < 0) q = 0; else if (q > (127 << 8)) q = 127 << 8;
            if (j == 0u || q != lastq) { lastt = interp32(NOTE_INC, (uint32_t)q); lastq = q; }       /* (the operators share a pitch unless the OPL's vibrato touches one) */
            t = lastt;
            if (opl) inc = (t >> 1) * OPL_RAT[o[OF_MUL] & 15u];
            else if (fam == FAM_OPZ) {
                const uint32_t m16 = (o[OF_MUL] ? (uint32_t)o[OF_MUL] * 16u : 8u) + o[OF_FINE];
                if (o[OF_FIX]) inc = (((o[OF_MUL] ? (uint32_t)o[OF_MUL] << 4 : 8u) | o[OF_FINE]) << (o[OF_DT1] > 3u ? o[OF_DT1] - 3u : 0u)) * 24348u;       /* a fixed frequency in hertz */
                else inc = (t >> 4) * m16;
            } else { mulx = o[OF_MUL] ? (uint32_t)o[OF_MUL] * 2u : 1u; inc = (t >> 1) * mulx; }
            if (!opl) {
                if (o[OF_DT2]) inc = (inc >> 8) * FM_DT2F[o[OF_DT2] & 3u];
                if (o[OF_DT1] != 3u && !(fam == FAM_OPZ && o[OF_FIX])) {            /* DT1: a few phase units, more for higher keys */
                    const uint32_t d = o[OF_DT1] > 3u ? o[OF_DT1] - 3u : 3u - o[OF_DT1], dt = (uint32_t)FM_DT1[d * 32u + kc] * FM_DTU[fam == FAM_OPM || fam == FAM_OPZ ? 0u : kind == KD_MD ? 1u : 2u];
                    if (o[OF_DT1] > 3u) inc += dt; else inc = inc > dt ? inc - dt : 0u;
                }
            }
            f->inc[j] = inc;
            a = vc[c].oa[j] >> 8;
            if (fam == FAM_OPZ) a >>= o[OF_SHIFT] & 3u;
            if (!(opll && j == 1u)) a += (uint32_t)o[OF_TL] * 8u;
            if (opl && o[OF_KSL]) {             /* key scale level: the OPL's raw 1 and 2 are the other way round from the OPLL's */
                const uint32_t kl = KSL_TMP[note > 127u ? 127u : note], s = opll ? o[OF_KSL] : (uint32_t)(o[OF_KSL] == 1u ? 2u : o[OF_KSL] == 2u ? 1u : 3u);
                a += (kl >> (3u - s)) * 4u;
            }
            if (o[OF_AM]) a += amu;
            if (a >= 1016u) lv = 0u; else lv = (DB_ATT[a >> 3] >> 2) * (FM_FINE8[a & 7u] >> 2) >> 12;
            f->lvt[j] = (int32_t)lv;
            if (!(carry >> j & 1u) && (int32_t)lv > maxmod) maxmod = lv;
            { const uint32_t w = o[OF_WAVE];
              f->wv[j] = (uint8_t)w;
              f->wt[j] = fam == FAM_OPZ ? (w ? OPZ_T + (w - 1u) * 1024u : SIN_T) : (opl && w) ? (w <= 3u ? OPL_W[w - 1u] : 0) : SIN_T; }
        }
    }
    f->alg = (uint8_t)alg; f->fbm = OPLL_FB[p->ch[CF_FB] & 7u]; f->ns = maxmod >= 11654u ? 2u : 1u; f->wsh = opll;
    osc[c].gain = (uint16_t)fm_gain(vol_q); osc[c].vol = (uint8_t)vol;
    osc[c].pan = FMXC[fam][CF_PAN] ? p->ch[CF_PAN] : 1u;
}

/* ---- the renderers: a voice at a time (its phases, levels and feedback live in locals for the whole block: the device pays for every load and store) */
#define FOPL(w, wv, lc, p) (((fast ? (int32_t)(w)[((p) >> 22) & 1023u] : (((w) != 0) ? (int32_t)(w)[((p) >> 22) & 1023u] : fm4_wave((p), (wv)))) * (lc)) >> 16)
#define GLIDE(lc, lt) do { const int32_t d_ = (lt) - (lc); if (d_) (lc) += d_ > 0 ? (d_ + 31) >> 5 : d_ >> 5; } while (0)       /* the level glides to its target, a 32nd of the way a sample */
static inline __attribute__((always_inline)) void r_fm4v(int32_t *out, uint32_t n, uint32_t c, uint32_t mode, const uint32_t fast, const uint32_t one)
{
    uint32_t i, s;
    {
        fmo_t *f = &fmo[c];
        const uint32_t ns = one ? 1u : 2u, sh = 3u - ns, alg = f->alg, pan = osc[c].pan, fbm = f->fbm;       /* (one: a constant, so the one step loop is plain code) */
        const uint32_t i0 = f->inc[0] << sh, i1 = f->inc[1] << sh, i2 = f->inc[2] << sh, i3 = f->inc[3] << sh;
        const int32_t gain = (int32_t)osc[c].gain, norm = FM_NORM[alg < 12u ? alg : 0u], t0 = f->lvt[0], t1 = f->lvt[1], t2 = f->lvt[2], t3 = f->lvt[3];
        const int16_t *const w0 = f->wt[0], *const w1 = f->wt[1], *const w2 = f->wt[2], *const w3 = f->wt[3];
        const uint32_t v0 = f->wv[0], v1 = f->wv[1], v2 = f->wv[2], v3 = f->wv[3];
        uint32_t p0 = f->ph[0], p1 = f->ph[1], p2 = f->ph[2], p3 = f->ph[3];
        int32_t l0 = f->lvc[0], l1 = f->lvc[1], l2 = f->lvc[2], l3 = f->lvc[3], y1 = f->y1, y2 = f->y2, rng = (int32_t)osc[c].rng, g2 = 0;
#define O0(x) FOPL(w0, v0, l0, p0 + (uint32_t)(x))
#define O1(x) FOPL(w1, v1, l1, p1 + ((uint32_t)(x) << 20))
#define O2(x) FOPL(w2, v2, l2, p2 + ((uint32_t)(x) << 20))
#define O3(x) FOPL(w3, v3, l3, p3 + ((uint32_t)(x) << 20))
        for (i = 0; i < n; i++) {
            int32_t v = 0;
            { const int32_t d = gain - rng; if (d) rng += d > 0 ? (d + 31) >> 5 : d >> 5; }
            GLIDE(l0, t0); GLIDE(l1, t1); GLIDE(l2, t2); GLIDE(l3, t3); g2 = (rng * norm) >> 8;
            for (s = 0; s < ns; s++) {
                int32_t a, o;
                p0 += i0; p1 += i1; p2 += i2; p3 += i3;
                a = O0(((uint32_t)((y1 + y2) >> 1) * fbm << 2));
                y2 = y1; y1 = a;
                switch (alg) {
                case 0: case 8: o = O3(O2(O1(a))); break;                                    /* 1 > 2 > 3 > 4 */
                case 1: o = O3(O2(a + O1(0))); break;                                        /* (1 + 2) > 3 > 4 */
                case 2: o = O3(a + O2(O1(0))); break;                                        /* (1 + (2 > 3)) > 4 */
                case 3: o = O3(O1(a) + O2(0)); break;                                        /* ((1 > 2) + 3) > 4 */
                case 4: case 10: o = O1(a) + O3(O2(0)); break;                               /* (1 > 2) + (3 > 4) */
                case 5: o = O1(a) + O2(a) + O3(a); break;                                    /* 1 > 2, 3, 4 */
                case 6: o = O1(a) + O2(0) + O3(0); break;                                    /* 1 > 2, then 3 and 4 */
                case 7: o = a + O1(0) + O2(0) + O3(0); break;                                /* all four */
                case 9: o = a + O3(O2(O1(0))); break;                                        /* 1 + (2 > 3 > 4) */
                default: o = a + O2(O1(0)) + O3(0); break;                                   /* 1 + (2 > 3) + 4 */
                }
                v += (o * g2) >> 16;
            }
            if (mode == 2u) {                                   /* the YM2612's 9 bit DAC: full scale is 255 steps; it truncates, clips, and leaves a gap of 7 steps at zero (+4 / -3, the ladder effect) */
                int32_t d = (ns == 1u ? v : v / 2) * 41 >> 10;
                if (d > 255) d = 255; if (d < -256) d = -256;
                v = (d ? d * 25 + (d > 0 ? 100 : -75) : 0) * 3;
            } else v = ns == 1u ? v * 3 : v * 3 / 2;
            if (pan != 2u) out[2u * i] += v;
            if (pan != 0u) out[2u * i + 1u] += v;
        }
#undef O0
#undef O1
#undef O2
#undef O3
        f->ph[0] = p0; f->ph[1] = p1; f->ph[2] = p2; f->ph[3] = p3;
        f->lvc[0] = l0; f->lvc[1] = l1; f->lvc[2] = l2; f->lvc[3] = l3; f->y1 = y1; f->y2 = y2; osc[c].rng = (uint32_t)rng;
    }
}
static void r_fm4x(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na, uint32_t mode)       /* four operators; mode 0 stereo by PAN, 2 the YM2612's 9 bit DAC. One step a sample, two while a modulator is loud (it would alias) */
{
    uint32_t k;
    for (k = 0; k < na; k++) {
        const fmo_t *f = &fmo[act[k]];
        if (f->wt[0] && f->wt[1] && f->wt[2] && f->wt[3]) {       /* every operator reads a table: no checks in the loop */
            if (f->ns == 1u) r_fm4v(out, n, act[k], mode, 1u, 1u); else r_fm4v(out, n, act[k], mode, 1u, 0u);
        } else if (f->ns == 1u) r_fm4v(out, n, act[k], mode, 0u, 1u);
        else r_fm4v(out, n, act[k], mode, 0u, 0u);
    }
}
static inline __attribute__((always_inline)) void r_fm2v(int32_t *out, uint32_t n, uint32_t c, const uint32_t fast)
{
    uint32_t i, s;
    fmo_t *f = &fmo[c];
    const uint32_t i0 = f->inc[0] << 1, i1 = f->inc[1] << 1, fbm = f->fbm, add = f->alg, wsh = f->wsh;
    const int32_t gain = (int32_t)osc[c].gain, t0 = f->lvt[0], t1 = f->lvt[1];
    const int16_t *const w0 = f->wt[0], *const w1 = f->wt[1];
    const uint32_t v0 = f->wv[0], v1 = f->wv[1];
    uint32_t p0 = f->ph[0], p1 = f->ph[1];
    int32_t l0 = f->lvc[0], l1 = f->lvc[1], y1 = f->y1, y2 = f->y2, rng = (int32_t)osc[c].rng;
    for (i = 0; i < n; i++) {
        int32_t acc = 0;
        for (s = 0; s < FMOS; s++) {
            int32_t a, o, x;
            p0 += i0; p1 += i1;
            { const int32_t d = t0 - l0; if (d) l0 += d > 0 ? (d + 31) >> 5 : d >> 5; }       /* the levels glide to their targets, twice a sample */
            { const int32_t d = t1 - l1; if (d) l1 += d > 0 ? (d + 31) >> 5 : d >> 5; }
            a = FOPL(w0, v0, l0, p0 + ((uint32_t)((y1 + y2) >> 1) * fbm << 2));
            y2 = y1; y1 = a;
            if (add) o = (a + FOPL(w1, v1, l1, p1)) >> 1;
            else o = FOPL(w1, v1, l1, p1 + ((uint32_t)a << 20));
            { const int32_t d = gain - rng; if (d) rng += d > 0 ? (d + 31) >> 5 : d >> 5; }
            x = (o * rng) >> 16; acc += wsh ? (x * 23) >> 4 : x;
        }
        out[2u * i] += acc; out[2u * i + 1u] += acc;
    }
    f->ph[0] = p0; f->ph[1] = p1; f->lvc[0] = l0; f->lvc[1] = l1; f->y1 = y1; f->y2 = y2; osc[c].rng = (uint32_t)rng;
}
static void r_fm2x(int32_t *out, uint32_t n, const uint8_t *act, uint32_t na)       /* two operators: 1 modulates 2 (or both play: additive, half level); the feedback carries operator 1's level, like the chips */
{
    uint32_t k;
    for (k = 0; k < na; k++) {
        const fmo_t *f = &fmo[act[k]];
        if (f->wt[0] && f->wt[1]) r_fm2v(out, n, act[k], 1u);
        else r_fm2v(out, n, act[k], 0u);
    }
}
