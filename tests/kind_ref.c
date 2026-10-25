/* SPDX-License-Identifier: GPL-3.0-only */
/* One steady note of one kind through chips.c, raw int32 stereo, for tests/arcade_compare.py.
 *   cc -O2 -DPOOL= '-D__attribute__(x)=' -Ifirmware/src -Ifirmware/gen -o build/host/kind_ref tests/kind_ref.c -lm
 *   kind_ref KIND NOTE SECONDS OUT.raw [wave=N duty=N wf= wc= wt= pan= vol= vel= atk= dec= sus= rel= sweep= off=SECONDS analog=N]   (DUMPW=1 prints the wave table, DUMPI=1 the phase steps) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#include "chips.c"
int main(int argc, char **argv)
{
    uint32_t kind = atoi(argv[1]), note = atoi(argv[2]), frames = (uint32_t)(atof(argv[3]) * FS), done = 0, vel = 127, rel_after = 0;
    static int32_t blk[512];
    FILE *f;
    int i;
    chan_add(kind);
    inst_t *in = &chan[0].in;
    in->atk = 0; in->dec = 0; in->sus = 15; in->rel = 0; in->vib_spd = in->vib_dep = 0; in->sweep = 0; in->vol = 15; in->arp = 0;
    for (i = 5; i < argc; i++) {
        char *e = strchr(argv[i], '='); int v;
        if (!e) continue;
        *e = 0; v = atoi(e + 1);
        if (!strcmp(argv[i], "wave")) in->wave = v; else if (!strcmp(argv[i], "duty")) in->duty = v; else if (!strcmp(argv[i], "wf")) in->wf = v;
        else if (!strcmp(argv[i], "wc")) in->wc = v; else if (!strcmp(argv[i], "wt")) in->wt = v; else if (!strcmp(argv[i], "pan")) in->pan = v;
        else if (!strcmp(argv[i], "vol")) in->vol = v; else if (!strcmp(argv[i], "vel")) vel = v; else if (!strcmp(argv[i], "atk")) in->atk = v;
        else if (!strcmp(argv[i], "dec")) in->dec = v; else if (!strcmp(argv[i], "sus")) in->sus = v; else if (!strcmp(argv[i], "rel")) in->rel = v;
        else if (!strcmp(argv[i], "sweep")) in->sweep = v; else if (!strcmp(argv[i], "off")) rel_after = (uint32_t)(atof(e + 1) * FS);
        else if (!strcmp(argv[i], "analog")) analog_mode[KIND_CHIP[kind]] = v;
    }
    memset(blk, 0, sizeof blk); chip_render(blk, 256);
    chip_note(0, note, vel);
    f = fopen(argv[4], "wb");
    while (done < frames) {
        uint32_t n = frames - done < 256u ? frames - done : 256u;
        if (rel_after && done <= rel_after && done + n > rel_after) chip_off(0);
        memset(blk, 0, sizeof blk); chip_render(blk, n);
        fwrite(blk, 8, n, f); done += n;
    }
    if (getenv("DUMPI")) fprintf(stderr, "inc %u inc2 %u rate_kHz %.1f\n", osc[0].inc, osc[0].inc2, osc[0].inc2 / 4294967296.0 * 176400 / 1000);
    if (getenv("DUMPW")) { int k; for (k = 0; k < 32; k++) fprintf(stderr, "%d ", wtab[0][k]); fprintf(stderr, "\n"); }
    fclose(f); return 0;
}
