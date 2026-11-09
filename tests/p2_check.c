/* SPDX-License-Identifier: GPL-3.0-only */
/* P2 / P3 native checks: the noise registers' cycle lengths, every new kind and knob renders (bounded, not silent), and the cost of the AY's buzz against its plain voices:
 * cc -O2 -DPOOL= -DCHIPS_C='"chips.c"' -Ifirmware/src -Ifirmware/gen -Itests -o /tmp/p2c tests/p2_check.c -lm */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define FS 44100u
#include "engine_stubs.h"
#include CHIPS_C
static int32_t blk[512];
static double bench(uint32_t kind, int voices, void (*setup)(inst_t *))
{
    int c, b, it; clock_t t0; int32_t pk = 0;
    chan_n = 0;
    for (c = 0; c < voices; c++) { chan_add(kind); if (setup) setup(&chan[c].in); }
    t0 = clock();
    for (it = 0; it < 40; it++) {
        for (c = 0; c < voices; c++) chip_note(c, 40 + (c * 3) % 40, 110);
        for (b = 0; b < 40; b++) { int i; memset(blk, 0, sizeof blk); chip_render(blk, 256); for (i = 0; i < 512; i++) if (abs(blk[i]) > pk) pk = abs(blk[i]); }
    }
    return (double)(clock() - t0) / CLOCKS_PER_SEC;
}
static void buzz3(inst_t *in) { in->wc = 3; in->wt = 1; in->wave = 0; }
static void buzz5b(inst_t *in) { in->pan = 3; in->wc = 1; }
static uint32_t period(uint32_t kind, uint32_t duty, uint32_t cap)
{
    uint32_t n = 0, r0;
    chan_n = 0; chan_add(kind);
    osc[0].duty = (uint8_t)duty; osc[0].inc2 = 65536u; osc[0].ph2 = 0; osc[0].rng = 0;
    for (n = 0; n < 300000u; n++) nz_step(0, kind);                     /* past any run-in */
    n = 0; r0 = osc[0].rng;
    do { nz_step(0, kind); n++; } while (osc[0].rng != r0 && n < cap);
    return n;
}
int main(void)
{
    uint32_t k, t;
    printf("NES long %u short %u | GB 15 bit %u 7 bit %u | PCE %u\n", period(KD_NES_NOI, 0, 1u << 20), period(KD_NES_NOI, 1, 1u << 20), period(KD_GB_NOI, 0, 1u << 20), period(KD_GB_NOI, 1, 1u << 20), period(KD_PCE_NOI, 1, 1u << 20));
    for (t = 0; t < 8; t++) printf("VB tap %u cycle %u\n", t, period(KD_VB_NOI, t, 1u << 20));
    for (k = KD_NES_NOI; k < KINDS; k++) {
        int c, it, b, i, pk = 0; long sum = 0;
        srand(5u + k); chan_n = 0;
        for (c = 0; c < 16; c++) chan_add(k);
        for (it = 0; it < 24; it++) {
            for (c = 0; c < 16; c++) {
                inst_t *in = &chan[c].in;
                in->duty = (uint8_t)rand(); in->wf = rand() & 15; in->vol = 6 + rand() % 10; in->pan = rand() % 3;
                chip_note(c, rand() % 128, 1 + rand() % 127);
            }
            for (b = 0; b < 20; b++) { memset(blk, 0, sizeof blk); chip_render(blk, 256); for (i = 0; i < 512; i++) { if (abs(blk[i]) > pk) pk = abs(blk[i]); sum += abs(blk[i]); } }
        }
        printf("kind %u (%s): peak %d mean %ld %s\n", k, KIND_NAME[k], pk, sum / (24L * 20 * 512), pk < 32768 && sum > 0 ? "ok" : "CHECK");
    }
    printf("AY 16 plain %.3f s, 16 buzz %.3f s, 5B 16 plain %.3f s, 5B 16 buzz %.3f s (40 x 40 blocks)\n", bench(KD_AY, 16, 0), bench(KD_AY, 16, buzz3), bench(KD_S5B, 16, 0), bench(KD_S5B, 16, buzz5b));
    return 0;
}
