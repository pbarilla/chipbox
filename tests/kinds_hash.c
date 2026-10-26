/* SPDX-License-Identifier: GPL-3.0-only */
/* hash of what every kind renders (16 voices, random knobs, fixed seed): cc ... -DCHIPS_C='"chips.c"' idchk.c; run old and new, diff */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include CHIPS_C
static int32_t blk[512];
int main(void)
{
    uint32_t kind;
    for (kind = 0; kind < KINDS; kind++) {
        if (getenv("ONLY") && atoi(getenv("ONLY")) != (int)kind) continue;
        uint64_t h = 1469598103934665603ull; int it, c, b, i, nv = 16;
        srand(1234u + kind); chan_n = 0;
        for (c = 0; c < nv; c++) chan_add(kind);
        for (it = 0; it < 12; it++) {
            for (c = 0; c < nv; c++) {
                inst_t *in = &chan[c].in;
                in->wave = rand() & 15; in->duty = (uint8_t)rand(); in->wf = rand() & 15; in->wc = rand() & 15; in->wt = rand() & 15;
                in->atk = rand() & 15; in->dec = rand() & 15; in->sus = rand() & 15; in->rel = rand() & 15; in->vol = rand() & 15;
                in->vib_spd = rand() & 15; in->vib_dep = rand() & 15; in->sweep = (int8_t)(rand() % 33 - 16); in->pan = (uint8_t)(rand() % 4);
                in->arp = rand() % 9; in->arp_rate = rand() & 3; in->arp_dir = rand() % 3;
                if (kind == KD_SID) { in->wf = (uint8_t)rand(); in->wc = (uint8_t)rand(); in->wt = (uint8_t)rand(); }
                if (IS_OPLL(kind)) in->wave = rand() % 15;
                chip_note(c, rand() % 128, 1 + rand() % 127);
            }
            for (b = 0; b < 30; b++) {
                if (b == 20) for (c = 0; c < nv; c++) chip_off(c);
                memset(blk, 0, sizeof blk); chip_render(blk, 256);
                for (i = 0; i < 512; i++) { h ^= (uint32_t)blk[i]; h *= 1099511628211ull; }
            }
        }
        printf("%u %016llx\n", kind, (unsigned long long)h);
    }
    return 0;
}
