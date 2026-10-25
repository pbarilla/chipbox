/* SPDX-License-Identifier: GPL-3.0-only */
/* ours: render one steady note of a kind from chips.c -> f32 stereo raw @44100.
   ours KIND NOTE SECS OUT.f32 wave duty pan vol wf wc wt analog [note2 kind2 ...no] */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include "chips.c"
int main(int argc, char **argv)
{
    uint32_t kind, note, frames, done = 0, i; static int32_t blk[512]; float *pcm; FILE *f;
    if (argc < 5) return 2;
    kind = atoi(argv[1]); note = atoi(argv[2]); frames = (uint32_t)(atof(argv[3]) * FS);
    chan_add(kind);
    { inst_t *in = &chan[0].in; uint32_t chip = KIND_CHIP[kind];
      in->atk = 0; in->dec = 0; in->sus = 15; in->rel = 0; in->vib_spd = in->vib_dep = 0; in->sweep = 0; in->vol = 15;
      if (argc > 5) in->wave = atoi(argv[5]);
      if (argc > 6) in->duty = atoi(argv[6]);
      if (argc > 7) in->pan = atoi(argv[7]);
      if (argc > 8) in->vol = atoi(argv[8]);
      if (argc > 9) in->wf = atoi(argv[9]);
      if (argc > 10) in->wc = atoi(argv[10]);
      if (argc > 11) in->wt = atoi(argv[11]);
      analog_mode[chip] = argc > 12 ? atoi(argv[12]) : 0; }
    memset(blk, 0, sizeof blk); chip_render(blk, 256);
    chip_note(0, note, 127);
    pcm = malloc(frames * 8u);
    while (done < frames) {
        uint32_t n = frames - done < 256u ? frames - done : 256u;
        memset(blk, 0, sizeof blk); chip_render(blk, n);
        for (i = 0; i < 2u * n; i++) pcm[2u * done + i] = (float)blk[i];
        done += n;
    }
    if (getenv("DUMPW")) { for (i = 0; i < 32; i++) fprintf(stderr, "%u ", wtab[0][i]); fprintf(stderr, "\n"); }
    f = fopen(argv[4], "wb"); fwrite(pcm, 8, frames, f); fclose(f);
    return 0;
}
