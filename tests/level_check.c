/* Chipbox: default level check. One fresh channel of every kind at its INST_DEF patch, through the real mixer.
 * Loudness = A-weighted rms (IEC 61672 analog A curve through a bilinear biquad cascade, mono = mean of L and R power),
 * 20 ms frames, only frames within 20 dB of the loudest frame count (the sounding part), power mean, in dB re 1 LSB.
 * Melodic: one held note, the first 250 ms (a short step), notes 60 and 72, velocity 100 and 127, mean of the four dB values.
 * Kits: 12 hits (BD CH SD CH BD CH SD OH BD CH SD CR) 150 ms apart, velocity 100 and 127, mean of the two.
 * Check: 4 voices of a kind at once (chord 60 64 67 72 / the kit pattern x4), peak of the master output.
 * Build: cc -O2 -DPOOL= '-D__attribute__(x)=' -Ifirmware/src -Ifirmware/gen tests/level_check.c -lm -o level_check
 * Run:   level_check sweep   (dB for vol 0..15 per kind)   |   level_check check   (dB at INST_DEF, 4-voice peak)
 * SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define FS 44100u
#define PROJ_INST 17u
#define PROJ_EV 6u
#include "chips.c"

typedef struct { double b0, b1, b2, a1, a2, z1, z2; } bq_t;
static void bq_make(bq_t *q, int nk, double w1, double w2)   /* analog num s^nk, den (s+w1)(s+w2), bilinear */
{
    const double c = 2.0 * FS, a0 = c * c + (w1 + w2) * c + w1 * w2;
    double n0, n1, n2;
    if (nk == 2) { n0 = c * c; n1 = -2.0 * c * c; n2 = c * c; } else { n0 = c; n1 = 0; n2 = -c; }
    q->b0 = n0 / a0; q->b1 = n1 / a0; q->b2 = n2 / a0;
    q->a1 = (2.0 * w1 * w2 - 2.0 * c * c) / a0; q->a2 = (c * c - (w1 + w2) * c + w1 * w2) / a0;
    q->z1 = q->z2 = 0;
}
static double bq_run(bq_t *q, double x) { double y = q->b0 * x + q->z1; q->z1 = q->b1 * x - q->a1 * y + q->z2; q->z2 = q->b2 * x - q->a2 * y; return y; }
static bq_t AW[2][3]; static double AWG;
static void aw_init(void)
{
    const double w1 = 2 * M_PI * 20.598997, w2 = 2 * M_PI * 107.65265, w3 = 2 * M_PI * 737.86223, w4 = 2 * M_PI * 12194.217;
    int c, k; double re = 0, im = 0;
    for (c = 0; c < 2; c++) { bq_make(&AW[c][0], 2, w1, w1); bq_make(&AW[c][1], 1, w2, w3); bq_make(&AW[c][2], 1, w4, w4); }
    { double w = 2 * M_PI * 1000.0 / FS, zr = cos(w), zi = -sin(w), nr = 1, ni = 0;     /* gain at 1 kHz, to normalise */
      for (k = 0; k < 3; k++) { const bq_t *q = &AW[0][k];
        double z2r = zr * zr - zi * zi, z2i = 2 * zr * zi;
        double nre = q->b0 + q->b1 * zr + q->b2 * z2r, nim = q->b1 * zi + q->b2 * z2i, dre = 1 + q->a1 * zr + q->a2 * z2r, dim = q->a1 * zi + q->a2 * z2i;
        double hr = (nre * dre + nim * dim) / (dre * dre + dim * dim), hi = (nim * dre - nre * dim) / (dre * dre + dim * dim);
        double tr = nr * hr - ni * hi, ti = nr * hi + ni * hr; nr = tr; ni = ti; }
      re = nr; im = ni; AWG = 1.0 / sqrt(re * re + im * im); }
}
static int32_t blk[256];
#define MAXS (44100 * 4)
static float sig[2][MAXS]; static uint32_t nsig;
static void render(uint32_t n)
{
    uint32_t i = 0;
    while (i < n) { uint32_t m = n - i > 128u ? 128u : n - i; memset(blk, 0, sizeof blk); chip_render(blk, m);
        for (uint32_t k = 0; k < m && nsig < MAXS; k++, nsig++) { sig[0][nsig] = (float)blk[2 * k]; sig[1][nsig] = (float)blk[2 * k + 1]; }
        i += m; }
}
static void reset(void) { while (chan_n) chan_remove(chan_n - 1u); nsig = 0; fx_pitch = 0; render(FS * 6 / 10); nsig = 0; }   /* silence long enough for every chip to go to sleep and start clean */
static double loud(void)       /* dB re 1 LSB */
{
    static double a[2][MAXS]; uint32_t i, f, nf = nsig / 882u; static double fp[400]; double mx = 0, s = 0; uint32_t cnt = 0;
    for (int c = 0; c < 2; c++) { for (int k = 0; k < 3; k++) AW[c][k].z1 = AW[c][k].z2 = 0;
        for (i = 0; i < nsig; i++) { double x = sig[c][i]; for (int k = 0; k < 3; k++) x = bq_run(&AW[c][k], x); a[c][i] = x * AWG; } }
    for (f = 0; f < nf; f++) { double p = 0; for (i = f * 882u; i < (f + 1u) * 882u; i++) p += a[0][i] * a[0][i] + a[1][i] * a[1][i]; fp[f] = p / 882.0 / 2.0; if (fp[f] > mx) mx = fp[f]; }
    for (f = 0; f < nf; f++) if (fp[f] >= mx * 0.01) { s += fp[f]; cnt++; }
    return cnt ? 10.0 * log10(s / cnt + 1e-9) : -99;
}
static const uint8_t PAT[12] = {0, 6, 2, 6, 0, 6, 2, 8, 0, 6, 2, 10};
static double measure(uint32_t kind, int vol, uint32_t voices, double *peak)
{
    double acc = 0, pk = 0; uint32_t nc = 0, v, cond, i, c;
    static const uint8_t NOTE[2] = {60, 72}, VEL[2] = {100, 127}, CH[4] = {60, 64, 67, 72};
    const uint32_t drum = IS_DRUM(kind);
    for (cond = 0; cond < (voices > 1 ? 1u : 4u); cond++) {
        const uint32_t vel = voices > 1 ? 127u : VEL[cond & 1u], note = NOTE[cond >> 1];
        if (drum && cond >= 2 && voices == 1) continue;
        reset();
        for (v = 0; v < voices; v++) { int ch = chan_add(kind); if (ch < 0) return 0; if (vol >= 0) chan[ch].in.vol = (uint8_t)vol; }
        if (!drum) { for (v = 0; v < voices; v++) chip_note(v, voices > 1 ? CH[v] : note, vel); render(FS / 4); }
        else { for (i = 0; i < 12; i++) { for (v = 0; v < voices; v++) chip_note(v, PAT[i], vel); render(FS * 15 / 100); } }
        for (i = 0; i < nsig; i++) { double a = fabs(sig[0][i]), b = fabs(sig[1][i]); if (a > pk) pk = a; if (b > pk) pk = b; }
        (void)c; acc += loud(); nc++;
    }
    if (peak) *peak = pk;
    return acc / nc;
}
int main(int argc, char **argv)
{
    static const char *const CN[] = {"NES","GB","SMS","PCE","TIA","VRC6","AY","MD","VB","POKEY","SPK","OPL","WSG","N163","S5B","MMC5","FDS","SCC","OPLL","VRC7","OPM","OPL3","WS","SAA","VIC","TED","LYNX","PC98","NEO","NGP","OPL1","OPZ","SN77","C30","MSM","ZX","OPN","SID"};
    int sweep = argc > 1 && !strcmp(argv[1], "sweep"); uint32_t k; int v;
    aw_init();
    for (k = 0; k < KINDS; k++) {
        const uint32_t drum = IS_DRUM(k); double pk;
        printf("%2u %-5s %s ", k, CN[KIND_CHIP[k]], drum ? "kit" : "mel");
        if (sweep) { for (v = 0; v < 16; v++) printf(" %6.1f", measure(k, v, 1, 0)); printf("\n"); }
        else { double l1 = measure(k, -1, 1, 0), l4 = (measure(k, -1, 4, &pk), pk); printf(" vol %2u  %6.1f dB  4-voice peak %6.0f\n", INST_DEF[k].vol, l1, l4); }
        fflush(stdout);
    }
    return 0;
}
