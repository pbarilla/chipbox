/* SPDX-License-Identifier: GPL-3.0-only */
/* FDS fuzz: every knob (also sweep outside +-16, duty/pan outside their nibbles), notes 0..127, 16 voices, retriggers. Build with -fsanitize=address,undefined. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include "chips.c"
static int32_t blk[512];
int main(int argc, char **argv)
{
    uint32_t seed = (uint32_t)atoi(argv[1]); long pk = 0; int it, c, b, i, nv = 16;
    srand(seed); chan_n = 0;
    for (c = 0; c < nv; c++) chan_add(KD_FDS);
    for (it = 0; it < 150; it++) {
        for (c = 0; c < nv; c++) {
            inst_t *in = &chan[c].in; int m = rand() % 4;
            in->wave = rand() & 15; in->duty = (uint8_t)(m == 0 ? rand() : (rand() & 15) | (it & 1 ? 0xF0 : (rand() & 0xF0))); in->wf = rand() & 15; in->wc = rand() & 15; in->wt = rand() & 15;
            in->atk = rand() & 15; in->dec = rand() & 15; in->sus = rand() & 15; in->rel = rand() & 15; in->vol = (uint8_t)(m == 1 ? rand() : 15);
            in->vib_spd = rand() & 15; in->vib_dep = rand() & 15; in->sweep = (int8_t)(m == 2 ? rand() : rand() % 33 - 16); in->pan = (uint8_t)(m == 3 ? rand() : (it % 3 ? 15 : rand() & 15));
            in->arp = rand() % 9; in->arp_rate = rand() & 3; in->arp_dir = rand() % 3;
            chip_note(c, (it & 2) ? 120 + rand() % 8 : rand() % 128, 127);
        }
        for (b = 0; b < 25; b++) {
            if (b == 12 && (it & 1)) chip_note(rand() % nv, rand() % 128, 100);
            if (b == 18) for (c = 0; c < nv; c++) if (rand() & 1) chip_off(c);
            memset(blk, 0, sizeof blk); chip_render(blk, 256);
            for (i = 0; i < 512; i++) { long a = blk[i] < 0 ? -(long)blk[i] : blk[i]; if (a > pk) pk = a; }
        }
    }
    printf("seed %u peak %ld\n", seed, pk); return 0;
}
