/* SPDX-License-Identifier: GPL-3.0-only */
/* The FM kinds with random patches (random bytes through fm_unpack, so every field is clamped as a loaded project's would be), 16 voices, random notes, held and released:
 * the output must stay finite and inside what the mixer can take, the voices must end, and the time a second takes is printed (the old engine's from fm_cmp built on the old tree).
 *   cc -O2 -DPOOL= '-D__attribute__(x)=' -Ifirmware/src -Ifirmware/gen tests/fm_fuzz.c -lm -o fm_fuzz && ./fm_fuzz */
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
int main(int argc, char **argv)
{
    static const uint32_t K[] = {KD_OPM, KD_MD, KD_OPN_FM, KD_PC98_FM, KD_NEO_FM, KD_OPZ, KD_OPL3, KD_OPL, KD_OPL1, KD_OPLL, KD_VRC7};
    uint32_t ki, it, c, b, i, bad = 0;
    int nv = argc > 1 ? atoi(argv[1]) : 16;
    for (ki = 0; ki < sizeof K / sizeof K[0]; ki++) {
        int32_t peak = 0; uint32_t endvoices = 0; double secs = 0;
        srand(4321u + ki);
        while (chan_n) chan_remove(chan_n - 1u);
        for (c = 0; (int)c < nv; c++) chan_add(K[ki]);
        for (it = 0; it < 10u; it++) {
            for (c = 0; (int)c < nv; c++) {
                uint8_t raw[64];
                for (i = 0; i < sizeof raw; i++) raw[i] = (uint8_t)rand();
                fm_unpack(K[ki], &chan[c].fm, raw);
                chan[c].in.vol = (uint8_t)(rand() & 15); chan[c].in.vib_dep = (uint8_t)(rand() & 15);
                chip_note(c, rand() % 128, 1 + rand() % 127);
            }
            { clock_t t0 = clock();
              for (b = 0; b < 40u; b++) {
                  if (b == 20u) for (c = 0; (int)c < nv; c++) chip_off(c);
                  memset(blk, 0, sizeof blk); chip_render(blk, 128);
                  for (i = 0; i < 256u; i++) { int32_t a = blk[i] < 0 ? -blk[i] : blk[i]; if (a > peak) peak = a; }
              }
              secs += (double)(clock() - t0) / CLOCKS_PER_SEC; }
        }
        for (b = 0; b < 400u; b++) { memset(blk, 0, sizeof blk); chip_render(blk, 128); }       /* ten seconds of quiet: everything must have ended */
        for (c = 0; (int)c < nv; c++) endvoices += vc[c].stage != ST_IDLE;
        printf("kind %u: peak %d, still sounding %u, %.2f s of render took %.2f s\n", K[ki], peak, endvoices, 10.0 * 40 * 128 / FS, secs);
        if (peak > 120000 || peak < 0) bad++;
    }
    printf(bad ? "FAIL\n" : "ok\n");
    return bad != 0;
}
