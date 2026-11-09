/* SPDX-License-Identifier: GPL-3.0-only */
/* (fm_ab: a hash of what the FM kinds render with random patches, run before and after an engine edit: they must match) The FM kinds with random patches (random bytes through fm_unpack, so every field is clamped as a loaded project's would be), 16 voices, random notes, held and released:
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
    uint32_t ki, it, c, b, i;
    int nv = argc > 1 ? atoi(argv[1]) : 16;
    FILE *dump = argc > 2 ? fopen(argv[2], "wb") : 0;      /* raw int32 of every block: tests/fm_ab.py compares two dumps (level and error in dB) */
    for (ki = 0; ki < sizeof K / sizeof K[0]; ki++) {
        uint32_t h = 2166136261u;
        srand(777u + ki);
        while (chan_n) chan_remove(chan_n - 1u);
        for (c = 0; (int)c < nv; c++) chan_add(K[ki]);
        for (it = 0; it < 24u; it++) {
            for (c = 0; (int)c < nv; c++) {
                uint8_t raw[64];
                for (i = 0; i < sizeof raw; i++) raw[i] = (uint8_t)rand();
                if (!(argc > 3)) fm_unpack(K[ki], &chan[c].fm, raw);      /* a 3rd argument: the default patches (a plain pluck) */
                chan[c].in.vol = (uint8_t)(rand() & 15); chan[c].in.vib_dep = (uint8_t)(rand() & 15);
                chip_note(c, rand() % 128, 1 + rand() % 127);
            }
            for (b = 0; b < 60u; b++) {
                if (b == 30u) for (c = 0; (int)c < nv; c++) chip_off(c);
                memset(blk, 0, sizeof blk); chip_render(blk, 128);
                for (i = 0; i < 256u; i++) h = (h ^ (uint32_t)blk[i]) * 16777619u;
                if (dump) fwrite(blk, 4, 256, dump);
            }
        }
        printf("%u %08x\n", K[ki], h);
        if (dump) { int32_t mark[256]; memset(mark, 0x7f, sizeof mark); fwrite(mark, 4, 256, dump); }
    }
    return 0;
}
