/* SPDX-License-Identifier: GPL-3.0-only */
/* Old projects against new engine: for every kind, 16 voices with only the knob values the OLD build could reach (INST_DEF plus its old knobs), hashed. Build once against the
 * pre-change tree and once against this one (-DKMAX=82 -DCHIPS_C='"chips.c"'), run both, diff. Where NZ_SKIP is set the AY-family NOISE 2 and the SAA
 * NOISE >= 5 (both fixed on purpose) are left out. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include "engine_stubs.h"
#include CHIPS_C
static int32_t blk[512];
static int r(int n) { return rand() % n; }
int main(void)
{
    uint32_t kind;
    for (kind = 0; kind < KMAX; kind++) {
        uint64_t h = 1469598103934665603ull; int it, c, b, i, nv = 16;
        if (getenv("ONLY") && atoi(getenv("ONLY")) != (int)kind) continue;
        srand(777u + kind); chan_n = 0;
        for (c = 0; c < nv; c++) chan_add(kind);
        for (it = 0; it < 12; it++) {
            for (c = 0; c < nv; c++) {
                inst_t *in = &chan[c].in;
                const uint32_t k = kind;
                const int drum = IS_DRUM(k), fm = IS_OPLL(k) || k == KD_MD || k == KD_OPM || k == KD_OPL || k == KD_OPL1 || k == KD_OPL3 || k == KD_OPZ || k == KD_PC98_FM || k == KD_NEO_FM || k == KD_OPN_FM;
                if (fm) continue;                               /* the FM kinds have their own comparison (tests/fm_cmp.c) */
                in->vol = (uint8_t)r(16); in->vib_dep = (uint8_t)r(16);
                in->atk = (uint8_t)r(16); in->dec = (uint8_t)r(16); in->sus = (uint8_t)r(16); in->rel = (uint8_t)r(16);
                in->arp = (uint8_t)r(9); in->arp_rate = (uint8_t)r(4); in->arp_dir = (uint8_t)r(3);
                if (drum) { chip_note(c, r(128), 1 + r(127)); continue; }
                if (k == KD_NES_PU || k == KD_NES_TRI || k == KD_SMS || k == KD_VRC6_PU || k == KD_VRC6_SAW || k == KD_TIA || k == KD_AY || k == KD_POKEY || k == KD_MMC5 || k == KD_PC98_SSG || k == KD_NEO_SSG || k == KD_OPN_SSG || k == KD_VIC || k == KD_TED || k == KD_SPK || k == KD_ZX || k == KD_FDS)
                    in->sweep = (int8_t)(r(33) - 16);
                if (k == KD_GB_WAV || k == KD_PCE || k == KD_VB_WAV || k == KD_WSG || k == KD_N163 || k == KD_FDS || k == KD_SCC || k == KD_WS || k == KD_C30) {
                    in->wave = (uint8_t)(r(16)); in->wf = (uint8_t)r(16); in->wc = (uint8_t)r(16); in->wt = (uint8_t)r(16);
                }
                if (k == KD_TIA) in->wave = (uint8_t)r(11);
                if (k == KD_AY || k == KD_S5B || k == KD_SAA || k == KD_PC98_SSG || k == KD_NEO_SSG || k == KD_OPN_SSG) {
                    in->wave = (uint8_t)r(16);
                    if (getenv("NZ_SKIP")) { if (in->wave == 2 && k != KD_SAA) in->wave = 3; if (k == KD_SAA && in->wave > 4) in->wave = (uint8_t)(in->wave & 3); }
                }
                if (k == KD_S5B) in->wf = (uint8_t)r(16);
                if (k == KD_TED || k == KD_VIC) in->wave = (uint8_t)r(2);
                if (k == KD_MSM) in->wave = (uint8_t)(1 + r(15));
                if (k == KD_SN77) in->wave = (uint8_t)r(4);
                if (k == KD_LYNX || k == KD_POKEY) in->wave = (uint8_t)r(8);
                if (k == KD_NES_PU || k == KD_GB_PU || k == KD_MMC5) in->duty = (uint8_t)r(4);
                if (k == KD_VRC6_PU || k == KD_ZX) in->duty = (uint8_t)r(8);
                if (k == KD_FDS) in->duty = (uint8_t)r(256);
                if (k == KD_GB_PU || k == KD_GB_WAV || k == KD_PCE || k == KD_VB_WAV || k == KD_WS || k == KD_SAA || k == KD_LYNX || k == KD_NGP || k == KD_C30) in->pan = (uint8_t)r(3);
                if (k == KD_S5B) in->pan = (uint8_t)r(8);
                if (k == KD_FDS) in->pan = (uint8_t)r(16);
                if (k == KD_MSM || k == KD_SN77) in->pan = (uint8_t)r(4);
                if (k == KD_SID) { in->wave = (uint8_t)r(256); in->duty = (uint8_t)r(256); in->wf = (uint8_t)r(256); in->wc = (uint8_t)r(256); in->wt = (uint8_t)r(256); }
                chip_note(c, r(128), 1 + r(127));
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
