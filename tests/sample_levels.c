/* SPDX-License-Identifier: GPL-3.0-only */
/* Plays project files (the raw payload bytes: `python3 tests/sample_levels.py` unpacks the site's samples and the demos) through the engine for 30 s each: peak, RMS, clipped samples, seconds of silence.
 *   cc -O2 -DPOOL= '-D__attribute__(x)=' -Itests -Ifirmware/src -Ifirmware/gen tests/sample_levels.c -lm -o sample_levels && ./sample_levels FILE.bin ... */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define FS 44100u
#define PROJ_INST 17u
#define PROJ_EV 6u
#define PROJ_BANK 0x3000u
#define PROJ_HDR 16u
#include "engine_stubs.h"
#include "chips.c"
static uint32_t fm1_ms;
#include "proj_core.h"
int main(int argc, char **argv)
{
    static uint8_t buf[PROJ_BANK * 2];
    int a;
    for (a = 1; a < argc; a++) {
        FILE *f = fopen(argv[a], "rb");
        uint32_t len, blocks, b, sec_blocks = FS / 256u, quiet = 0, clip = 0, i;
        double sum = 0, mx = 0, secsum = 0;
        int32_t blk[512];
        if (!f) { printf("%s: no file\n", argv[a]); continue; }
        len = (uint32_t)fread(buf, 1, sizeof buf, f); fclose(f);
        if (proj_check(buf, len) || proj_unpack(buf, len)) { printf("%-40s DOES NOT LOAD\n", argv[a]); continue; }
        seq_play(0, 0);
        blocks = 30u * sec_blocks;
        for (b = 0; b < blocks; b++) {
            memset(blk, 0, sizeof blk); chip_render(blk, 256);
            for (i = 0; i < 512; i++) {
                double v = (double)blk[i] / 32768.0;
                if (fabs(v) > mx) mx = fabs(v);
                if (blk[i] >= 32767 || blk[i] <= -32768) clip++;
                sum += v * v; secsum += v * v;
            }
            if ((b + 1u) % sec_blocks == 0u) { if (secsum / (512.0 * sec_blocks) < 1e-6) quiet++; secsum = 0; }
        }
        printf("%-40s peak %5.1f dBFS  rms %6.1f dBFS  clipped %u  silent seconds %u/30\n", argv[a], 20 * log10(mx > 1e-9 ? mx : 1e-9), 10 * log10(sum / (512.0 * blocks) + 1e-12), clip, quiet);
    }
    return 0;
}
