/* SPDX-License-Identifier: GPL-3.0-only */
/* drum KIND LANE VEL SECS OUT.f32 : one hit of a kit lane (0..10) -> f32 stereo */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include "chips.c"
int main(int argc, char **argv)
{
    uint32_t kind = atoi(argv[1]), lane = atoi(argv[2]), vel = atoi(argv[3]), frames = (uint32_t)(atof(argv[4]) * FS), done = 0, i;
    static int32_t blk[512]; float *pcm; FILE *f;
    chan_add(kind);
    chan[0].in.vol = 15;
    memset(blk, 0, sizeof blk); chip_render(blk, 256);
    chip_note(0, DR_LANE_TYPE[lane], vel);
    pcm = malloc(frames * 8u);
    while (done < frames) {
        uint32_t n = frames - done < 256u ? frames - done : 256u;
        memset(blk, 0, sizeof blk); chip_render(blk, n);
        for (i = 0; i < 2u * n; i++) pcm[2u * done + i] = (float)blk[i];
        done += n;
    }
    f = fopen(argv[5], "wb"); fwrite(pcm, 8, frames, f); fclose(f);
    return 0;
}
