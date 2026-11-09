/* SPDX-License-Identifier: GPL-3.0-only */
/* Old FM engine against the new: every FM kind with a set of classic patches (the knobs RATIO, INDEX, FB, DECAY, the envelope), one note each, level over the attack, the body and the release.
 * Build the old tree and this tree, run both, compare:
 *   cc -O2 -DPOOL= '-D__attribute__(x)=' -I OLD/firmware/src -I OLD/firmware/gen tests/fm_cmp.c -lm -o fm_old
 *   cc -O2 -DPOOL= '-D__attribute__(x)=' -DNEWFM -Ifirmware/src -Ifirmware/gen tests/fm_cmp.c -lm -o fm_new
 *   ./fm_old > old.txt; ./fm_new > new.txt; python3 tests/fm_cmp.py old.txt new.txt
 * With DUMP=dir it also writes a mono 16 bit WAV of every patch (for spectra). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define FS 44100u
#define PROJ_INST 17u
#define PROJ_EV 6u
#include "engine_stubs.h"
#include "chips.c"
static int32_t blk[512];
static double acc2; static uint32_t accn; static int32_t peak; static int16_t *wavbuf; static uint32_t wavn;
static void render(uint32_t n)
{
    uint32_t i = 0, k;
    while (i < n) {
        uint32_t m = n - i > 128u ? 128u : n - i;
        memset(blk, 0, sizeof blk); chip_render(blk, m);
        for (k = 0; k < m; k++) {
            const double x = (blk[2 * k] + blk[2 * k + 1]) * 0.5;
            acc2 += x * x; accn++;
            if (blk[2 * k] > peak) peak = blk[2 * k]; if (-blk[2 * k] > peak) peak = -blk[2 * k];
            if (wavbuf && wavn < FS * 4u) wavbuf[wavn++] = (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x);
        }
        i += m;
    }
}
static double db(void) { double r = accn ? sqrt(acc2 / accn) : 0; acc2 = 0; accn = 0; return r < 1 ? -99.0 : 20 * log10(r); }
static void wav_write(const char *path)
{
    FILE *f = fopen(path, "wb");
    uint32_t n = wavn * 2u, v;
    if (!f) return;
    fwrite("RIFF", 1, 4, f); v = 36u + n; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); v = 16; fwrite(&v, 4, 1, f);
    { uint16_t a = 1, b = 1; fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f); v = FS; fwrite(&v, 4, 1, f); v = FS * 2u; fwrite(&v, 4, 1, f); a = 2; b = 16; fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f); }
    fwrite("data", 1, 4, f); fwrite(&n, 4, 1, f); fwrite(wavbuf, 2, wavn, f); fclose(f);
}
static const uint32_t KINDS_FM[] = {KD_OPM, KD_MD, KD_OPN_FM, KD_PC98_FM, KD_NEO_FM, KD_OPZ, KD_OPL3, KD_OPL, KD_OPL1, KD_OPLL, KD_VRC7};
int main(void)
{
    const char *dump = getenv("DUMP");
    uint32_t ki, pi;
    wavbuf = malloc(FS * 4u * 2u);
    for (ki = 0; ki < sizeof KINDS_FM / sizeof KINDS_FM[0]; ki++) {
        const uint32_t kind = KINDS_FM[ki];
        for (pi = 0; pi < 24u; pi++) {
            inst_t *in;
            int c;
            double a, b, r;
            uint32_t note = 48u + (pi % 4u) * 12u;
            srand(77u + pi * 13u + kind);
            while (chan_n) chan_remove(chan_n - 1u);
            fx_pitch = 0;
            c = chan_add(kind);
            in = &chan[c].in;
            if (pi) {                                   /* pi 0: the default patch; the rest random knobs */
                in->wave = rand() & 15; in->duty = (uint8_t)(rand() % 8); in->wf = 3 + rand() % 13; in->wc = rand() % 8; in->wt = rand() % 8;
                in->atk = rand() % 6; in->dec = 2 + rand() % 14; in->sus = rand() & 15; in->rel = rand() % 12; in->pan = 1; in->vol = 8 + rand() % 8;
                if (kind == KD_OPL3) in->duty &= 3; if (kind == KD_OPL || kind == KD_OPL1) in->duty &= 3; if (kind == KD_OPLL || kind == KD_VRC7) { in->wave = rand() % 15; in->atk = in->dec = in->sus = in->rel = 0; }
                if (kind == KD_OPZ || kind == KD_OPL3) in->pan = rand() & 3;
            }
#ifdef NEWFM
            if (kind == KD_OPLL || kind == KD_VRC7) { if (pi) fm_from_classic(kind, in, &chan[c].fm); }
            else fm_from_classic(kind, in, &chan[c].fm);
#endif
            if (kind == KD_OPLL || kind == KD_VRC7) {
#ifndef NEWFM
                if (pi) opll_set(in, kind, in->wave);
#else
                if (pi) { inst_t t = *in; t.wave = in->wave; t.atk = OPLL_P[in->wave + (kind == KD_VRC7 ? 15 : 0)].atk; t.dec = OPLL_P[in->wave + (kind == KD_VRC7 ? 15 : 0)].dec; t.sus = OPLL_P[in->wave + (kind == KD_VRC7 ? 15 : 0)].sus; t.rel = OPLL_P[in->wave + (kind == KD_VRC7 ? 15 : 0)].rel; fm_from_classic(kind, &t, &chan[c].fm); }
#endif
            }
            render(FS * 6 / 10); acc2 = 0; accn = 0; peak = 0; wavn = 0;
            chip_note((uint32_t)c, note, 100);
            render(FS / 4); a = db();
            render(FS * 3 / 4); b = db();
            chip_off((uint32_t)c);
            render(FS / 2); r = db();
            printf("%u %u %.1f %.1f %.1f %d\n", kind, pi, a, b, r, peak);
            if (dump) { char nm[200]; snprintf(nm, sizeof nm, "%s/%u_%u.wav", dump, kind, pi); wavn = wavn > FS * 2u ? FS * 2u : wavn; wav_write(nm); }
        }
    }
    return 0;
}
