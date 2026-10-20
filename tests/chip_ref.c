/* SPDX-License-Identifier: GPL-3.0-only */
/* Renders one steady note from chips.c to a WAV, no screen, no tick-driven changes: for comparing against
 * reference emulators (tests/ref_compare.py).
 *   cc -O2 -Ifirmware/src -Ifirmware/gen -o build/host/chip_ref tests/chip_ref.c
 *   build/host/chip_ref CHIP CH NOTE SECONDS OUT.wav [WAVE_OR_DUTY [PAN [ANALOG]]]      CHIP: 0 NES 1 GB 2 SMS (CH 0, 1 pulse or tone, 2 triangle / wave); ANALOG: 0 clean 1 hardware 2 alt */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include "chips.c"

int main(int argc, char **argv)
{
    uint32_t chip, ch, note, frames, done = 0, par = 0, pan = 1, i;
    static int32_t blk[512];
    int16_t *pcm;
    FILE *f;
    uint32_t hdr[11];
    if (argc < 6) { fprintf(stderr, "usage: chip_ref CHIP CH NOTE SECONDS OUT.wav [WAVE_OR_DUTY [PAN]]\n"); return 2; }
    chip = (uint32_t)atoi(argv[1]); ch = (uint32_t)atoi(argv[2]); note = (uint32_t)atoi(argv[3]);
    frames = (uint32_t)(atof(argv[4]) * FS);
    if (argc > 6) par = (uint32_t)atoi(argv[6]);
    if (argc > 7) pan = (uint32_t)atoi(argv[7]);
    if (argc > 8) analog_mode[chip] = (uint32_t)atoi(argv[8]);
    { static const uint8_t KIND_OF[CHIPS][3] = {{KD_NES_PU, KD_NES_PU, KD_NES_TRI}, {KD_GB_PU, KD_GB_PU, KD_GB_WAV}, {KD_SMS, KD_SMS, KD_SMS}};
      inst_t *in;
      if (chip >= CHIPS || ch >= 3u) { fprintf(stderr, "bad chip or channel\n"); return 2; }
      chan_add(KIND_OF[chip][ch]);
      in = &chan[0].in;
      in->atk = 0; in->dec = 0; in->sus = 15; in->rel = 0; in->vib_spd = in->vib_dep = 0; in->sweep = 0; in->pan = (uint8_t)pan;
      in->wave = (uint8_t)par; in->duty = (uint8_t)par; }
    memset(blk, 0, sizeof blk);
    chip_render(blk, 256);                      /* the first block starts the engine */
    chip_note(0, note, 127);
    pcm = malloc(frames * 4u);
    while (done < frames) {
        uint32_t n = frames - done < 256u ? frames - done : 256u;
        memset(blk, 0, sizeof blk);
        chip_render(blk, n);
        for (i = 0; i < 2u * n; i++) pcm[2u * done + i] = (int16_t)(blk[i] > 32767 ? 32767 : blk[i] < -32767 ? -32767 : blk[i]);
        done += n;
    }
    f = fopen(argv[5], "wb");
    if (!f) { perror(argv[5]); return 1; }
    hdr[0] = 0x46464952u; hdr[1] = 36u + frames * 4u; hdr[2] = 0x45564157u; hdr[3] = 0x20746d66u; hdr[4] = 16u;
    hdr[5] = 0x00020001u; hdr[6] = FS; hdr[7] = FS * 4u; hdr[8] = 0x00100004u; hdr[9] = 0x61746164u; hdr[10] = frames * 4u;
    fwrite(hdr, 4, 11, f); fwrite(pcm, 4, frames, f); fclose(f);
    return 0;
}
