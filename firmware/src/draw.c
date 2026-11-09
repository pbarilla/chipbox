/* SPDX-License-Identifier: GPL-3.0-only */
/* DRAW: a wave you draw sample by sample, for the chips whose wave is writable RAM. A channel with a drawing (chan_t.wcus) plays chan_t.user instead of the generator's table
 * (set_osc points osc.wav at it). The samples are stored at the chip's real depth, 0 .. wd_top, like wtab. Included by chips.c after wave_make. */
static uint32_t wd_ok(uint32_t kind)                    /* a kind with a writable wave (the Namco WSG's waves are fixed ROM) */
{
    return kind == KD_GB_WAV || kind == KD_PCE || kind == KD_VB_WAV || kind == KD_WS || kind == KD_SCC || kind == KD_N163 || kind == KD_FDS || kind == KD_C30;
}
static uint32_t wd_top(uint32_t kind)                   /* the largest sample: 4, 5, 6 or 8 bits */
{
    return kind == KD_PCE ? 31u : kind == KD_VB_WAV || kind == KD_FDS ? 63u : kind == KD_SCC ? 255u : 15u;
}
static uint32_t wd_bits(uint32_t kind) { const uint32_t t = wd_top(kind); return t == 255u ? 8u : t == 63u ? 6u : t == 31u ? 5u : 4u; }
static uint32_t wd_n(uint32_t c)                        /* the samples in the wave: 64 on the FDS, the N163's LENGTH, else 32 */
{
    static const uint8_t NL[5] = {4, 8, 16, 32, 64};
    const inst_t *wi = &chan[c].in;
    if (chan[c].kind == KD_FDS) return 64u;
    if (chan[c].kind == KD_N163 && wi->duty) return NL[wi->duty > 5u ? 4u : wi->duty - 1u];
    return 32u;
}
static void wd_gen(uint32_t c, uint8_t *out)            /* the table the generator makes now (the same calls as set_osc) */
{
    const inst_t *wi = &chan[c].in;
    const uint32_t k = chan[c].kind, n = wd_n(c);
    if (k == KD_FDS) wave_make_s(wi, out, 63, 64);
    else if (k == KD_N163) {
        if (n == 64u) wave_make_s(wi, out, 15, 64);
        else if (n < 32u) { uint8_t tmp[32]; uint32_t i; wave_make(wi, tmp); for (i = 0; i < n; i++) out[i] = tmp[i * 32u / n]; }
        else wave_make(wi, out);
    } else wave_make_n(wi, out, (int32_t)wd_top(k));
}
static void wd_cur(uint32_t c, uint8_t *out)            /* the wave that sounds: the drawing, or the generator's */
{
    if (chan[c].wcus) memcpy(out, chan[c].user, wd_n(c)); else wd_gen(c, out);
}
static void wd_edit(uint32_t c, uint32_t cur, int32_t d, uint32_t bw, uint32_t sym)      /* the samples under the brush move by d; sym: 0 off, 1 mirror, 2 flipped mirror, 3 repeat the half */
{
    const uint32_t n = wd_n(c), top = wd_top(chan[c].kind), h = n / 2u;
    uint32_t i;
    if (cur >= n) return;
    if (!chan[c].wcus) { wd_gen(c, chan[c].user); chan[c].wcus = 1; }          /* the first edit starts from what plays now */
    for (i = 0; i < bw && cur + i < n; i++) {
        const uint32_t k = cur + i;
        int32_t v = (int32_t)chan[c].user[k] + d;
        v = v < 0 ? 0 : v > (int32_t)top ? (int32_t)top : v;
        chan[c].user[k] = (uint8_t)v;
        if (sym == 1u) chan[c].user[n - 1u - k] = (uint8_t)v;
        else if (sym == 2u) chan[c].user[n - 1u - k] = (uint8_t)(top - (uint32_t)v);
        else if (sym == 3u) chan[c].user[(k + h) % n] = (uint8_t)v;
    }
}
static void wd_resize(uint32_t c, uint32_t from, uint32_t to)      /* LENGTH changed on a drawn N163 wave: the samples are picked again */
{
    uint8_t tmp[64];
    uint32_t i;
    if (!chan[c].wcus || !from || !to || from == to) return;
    memcpy(tmp, chan[c].user, from);
    for (i = 0; i < to; i++) chan[c].user[i] = tmp[i * from / to];
}
static uint32_t wd_plen(uint32_t c) { return wd_n(c) * wd_bits(chan[c].kind) / 8u; }       /* the project's bytes for a drawing */
static void wd_pack(uint32_t c, uint8_t *p)
{
    const uint32_t n = wd_n(c), b = wd_bits(chan[c].kind), top = wd_top(chan[c].kind);
    uint32_t i, j, pos = 0;
    memset(p, 0, wd_plen(c));
    for (i = 0; i < n; i++) {
        const uint32_t v = chan[c].user[i] & top;
        for (j = b; j > 0u; j--, pos++) if (v >> (j - 1u) & 1u) p[pos >> 3] |= (uint8_t)(0x80u >> (pos & 7u));
    }
}
static void wd_unpack(uint32_t c, const uint8_t *p)
{
    const uint32_t n = wd_n(c), b = wd_bits(chan[c].kind), top = wd_top(chan[c].kind);
    uint32_t i, j, pos = 0;
    for (i = 0; i < n; i++) {
        uint32_t v = 0;
        for (j = 0; j < b; j++, pos++) v = v << 1 | (p[pos >> 3] >> (7u - (pos & 7u)) & 1u);
        chan[c].user[i] = (uint8_t)(v & top);
    }
    chan[c].wcus = 1;
}
