/* SPDX-License-Identifier: GPL-3.0-only */
/* DRAW checks, native: (1) a drawing that holds the generator's own table renders like the generator (the last 4000 samples within 2 LSB); (2) pack / unpack round trips at every depth and length;
 * (3) a random drawn table sounds like its table: the first harmonics of the rendered note against the table's DFT. cc -O2 -DPOOL= '-D__attribute__(x)=' -Itests -Ifirmware/src -Ifirmware/gen tests/draw_check.c -lm */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define FS 44100u
#include "engine_stubs.h"
#include "chips.c"
static int32_t blk[512];
static double *capture(uint32_t kind, int custom, const uint8_t *tab, uint32_t note, uint32_t nsamp, uint64_t *hash, uint32_t duty)
{
    double *y = malloc(sizeof(double) * nsamp);
    uint64_t h = 1469598103934665603ull;
    uint32_t i, got = 0;
    chan_n = 0; chan_add(kind);
    chan[0].in.duty = (uint8_t)duty; chan[0].in.atk = 0; chan[0].in.sus = 15; chan[0].in.vol = 15; chan[0].in.vib_dep = 0; chan[0].in.arp = 0;
    if (custom == 1) { wd_gen(0, chan[0].user); chan[0].wcus = 1; }
    else if (custom == 2) { memcpy(chan[0].user, tab, wd_n(0)); chan[0].wcus = 1; }
    chip_note(0, note, 127);
    while (got < nsamp) {
        memset(blk, 0, sizeof blk); chip_render(blk, 256);
        for (i = 0; i < 256 && got < nsamp; i++, got++) { y[got] = (double)(blk[2 * i] + blk[2 * i + 1]) * 0.5; h ^= (uint32_t)blk[2 * i]; h *= 1099511628211ull; }
    }
    *hash = h;
    return y;
}
static double mag(const double *y, uint32_t n, double f)
{
    double re = 0, im = 0; uint32_t i;
    for (i = 0; i < n; i++) { const double a = 2.0 * M_PI * f * i / 44100.0; re += y[i] * cos(a); im += y[i] * sin(a); }
    return sqrt(re * re + im * im);
}
int main(void)
{
    static const uint32_t K[8] = {KD_GB_WAV, KD_PCE, KD_VB_WAV, KD_WS, KD_SCC, KD_N163, KD_FDS, KD_C30};
    int bad = 0;
    uint32_t ki, j;
    for (ki = 0; ki < 8; ki++) {
        const uint32_t kind = K[ki], nl = kind == KD_N163 ? 5u : 1u;
        for (j = 0; j < nl; j++) {          /* N163: every LENGTH */
            const uint32_t duty = kind == KD_N163 ? j + 1u : 0u;
            uint64_t h1, h2, h3;
            double *a, *b, *c, f0 = 440.0 * pow(2.0, (45.0 - 69.0) / 12.0), best = 0, bf = f0, dev = 0;
            uint8_t tab[64], t2[64], pk[64];
            uint32_t n, top, i, k, N = 20000, pl;
            chan_n = 0; chan_add(kind); chan[0].in.duty = (uint8_t)duty;
            n = wd_n(0); top = wd_top(kind); pl = wd_plen(0);
            /* 1: the generator's table, drawn, renders the same */
            a = capture(kind, 0, 0, 45, 12000, &h1, duty);
            b = capture(kind, 1, 0, 45, 12000, &h2, duty);
            { double md = 0; uint32_t q; for (q = 8000; q < 12000; q++) { const double dd = fabs(a[q] - b[q]); if (dd > md) md = dd; }
              h1 = md > 2.0 && kind != KD_WS; h2 = 0;
              if (kind == KD_WS) printf("(WS: its 24 kHz sample clock is a static counter, so two runs only match in spectrum; largest tail difference %.0f)\n", md); }                              /* (the filters keep their state between runs: the tails must agree) */
            free(a); free(b);
            if (h1 != h2) { printf("FAIL kind %u duty %u: drawn copy of the generator differs\n", kind, duty); bad++; }
            /* 2: pack / unpack */
            srand(kind * 31u + j);
            for (i = 0; i < n; i++) tab[i] = (uint8_t)(rand() % (top + 1u));
            chan_n = 0; chan_add(kind); chan[0].in.duty = (uint8_t)duty; memcpy(chan[0].user, tab, n); wd_pack(0, pk);
            memset(chan[0].user, 0, 64); wd_unpack(0, pk);
            if (memcmp(chan[0].user, tab, n) || !chan[0].wcus || pl != n * wd_bits(kind) / 8u) { printf("FAIL kind %u duty %u: pack round trip\n", kind, duty); bad++; }
            /* 3: a random table: the harmonics */
            for (i = 0; i < n; i++) t2[i] = (uint8_t)((tab[i] + tab[(i + 1) % n]) / 2u);      /* smoothed: fewer high harmonics than the oversampled hold sees */
            c = capture(kind, 2, t2, 45, N + 4000, &h3, duty);
            for (i = 0; i < 600; i++) { const double f = f0 * (0.97 + 0.06 * i / 600.0), m = mag(c + 4000, N, f); if (m > best) { best = m; bf = f; } }
            for (k = 2; k <= 6; k++) {
                double re = 0, im = 0, X, M, r, rm, d;
                for (i = 0; i < n; i++) { re += t2[i] * cos(2.0 * M_PI * k * i / n); im += t2[i] * sin(2.0 * M_PI * k * i / n); }
                X = sqrt(re * re + im * im);
                { double r1 = 0, i1 = 0, X1; for (i = 0; i < n; i++) { r1 += t2[i] * cos(2.0 * M_PI * i / n); i1 += t2[i] * sin(2.0 * M_PI * i / n); } X1 = sqrt(r1 * r1 + i1 * i1);
                  r = (X / X1) * (sin(M_PI * k / n) / (M_PI * k / n)) / (sin(M_PI / n) / (M_PI / n)); }
                M = mag(c + 4000, N, bf * k); rm = M / best;
                if (r < 0.12) continue;
                d = 20.0 * log10(rm / r); if (d < 0) d = -d; if (d > dev) dev = d;
            }
            printf("kind %2u %-3s n=%2u: harmonics within %.2f dB of the table%s\n", kind, "", n, dev, dev > 2.5 ? "  <- CHECK" : "");
            if (dev > 2.5) bad++;
            free(c);
        }
    }
    printf(bad ? "%d FAILED\n" : "all ok\n", bad);
    return bad != 0;
}
