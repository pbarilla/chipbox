/* SPDX-License-Identifier: GPL-3.0-only */
/* How long 16 voices of an FM kind take to render, native: build against the old tree and this one and compare the ratios.
 *   cc -O2 -DPOOL= '-D__attribute__(x)=' -I OLD/firmware/src -I OLD/firmware/gen tests/fm_bench.c -lm -o bench_old   (and the same with this tree's directories) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define FS 44100u
#define PROJ_INST 17u
#define PROJ_EV 6u
#include "engine_stubs.h"
#include "chips.c"
static int32_t blk[512];
int main(void)
{
    static const uint32_t K[] = {KD_OPM, KD_MD, KD_OPN_FM, KD_OPZ, KD_OPL3, KD_OPL, KD_OPLL};
    uint32_t ki, c, b, rep;
    for (ki = 0; ki < sizeof K / sizeof K[0]; ki++) {
        double best = 1e9;
        for (rep = 0; rep < 5u; rep++) {
            clock_t t0;
            while (chan_n) chan_remove(chan_n - 1u);
            for (c = 0; c < 16u; c++) {
                chan_add(K[ki]);
#ifdef IS_FMK                                           /* the new engine: operators that do not fall, so every voice rings to the end */
                { uint32_t j; for (j = 0; j < 4u; j++) { chan[c].fm.op[j][OF_D1R] = 0; chan[c].fm.op[j][OF_D2R] = 0; chan[c].fm.op[j][OF_EGT] = 1; } }
#endif
                chip_note(c, 48 + c * 3, 110);
            }
            t0 = clock();
            for (b = 0; b < 6000u; b++) { memset(blk, 0, sizeof blk); chip_render(blk, 128); }
            { double t = (double)(clock() - t0) / CLOCKS_PER_SEC; if (t < best) best = t; }
        }
        printf("kind %u: %.3f s for %.1f s of audio\n", K[ki], best, 6000.0 * 128 / FS);
    }
    return 0;
}
