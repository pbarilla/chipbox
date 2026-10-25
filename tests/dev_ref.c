/* SPDX-License-Identifier: GPL-3.0-only */
/* Register script -> raw int32 stereo through a libvgm core (AY, MSM5232, SCC), for tests/arcade_compare.py. Measurement only.
 *   cc -O2 -std=gnu99 -D VGM_LITTLE_ENDIAN -D HAVE_STDINT_H -D EC_AY8910_EMU2149 -D EC_AY8910_MAME -D SNDDEV_AY8910 -D SNDDEV_MSM5232 -D SNDDEV_K051649 -DSNDDEV_SELECT -I$LIBVGM -o build/host/dev_ref tests/dev_ref.c $LIBVGM/build/bin/libvgm-emu.a $LIBVGM/build/bin/libvgm-utils.a -lm -lz */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdtype.h"
#include "emu/EmuStructs.h"
#include "emu/SoundEmu.h"
#include "emu/SoundDevs.h"
#include "emu/EmuCores.h"
#include "emu/cores/ayintf.h"
#include "emu/cores/msm5232.h"
#include "emu/cores/k051649.h"
int main(int argc, char **argv)
{
    DEV_GEN_CFG g; DEV_INFO inf; DEVFUNC_WRITE_A8D8 wr; AY8910_CFG ay; MSM5232_CFG ms;
    UINT32 clock, rate, total, done = 0; double secs; int isay = !strcmp(argv[1], "ay"), isscc = !strcmp(argv[1], "scc");
    static struct { double t; unsigned r, v; } ev[100000]; int ne = 0, ie = 0;
    FILE *f;
    if (argc < 9) return 2;
    clock = atoi(argv[4]); rate = atoi(argv[5]); secs = atof(argv[8]);
    memset(&g, 0, sizeof g);
    g.emuCore = !strcmp(argv[2], "mame") ? FCC_MAME : !strcmp(argv[2], "emu") ? FCC_EMU_ : 0;
    g.srMode = rate ? DEVRI_SRMODE_CUSTOM : DEVRI_SRMODE_NATIVE; g.flags = 0; g.clock = clock; g.smplRate = rate;
    if (isay) { ay._genCfg = g; ay.chipType = atoi(argv[3]); ay.chipFlags = 0; if (SndEmu_Start(DEVID_AY8910, (DEV_GEN_CFG *)&ay, &inf)) { fprintf(stderr, "start fail\n"); return 1; } }
    else if (isscc) { if (SndEmu_Start(DEVID_K051649, &g, &inf)) { fprintf(stderr, "start fail\n"); return 1; } }
    else { int i; ms._genCfg = g; for (i = 0; i < 8; i++) ms.capacitors[i] = atof(argv[3]); if (SndEmu_Start(DEVID_MSM5232, (DEV_GEN_CFG *)&ms, &inf)) { fprintf(stderr, "start fail\n"); return 1; } }
    inf.devDef->Reset(inf.dataPtr);
    SndEmu_GetDeviceFunc(inf.devDef, RWF_REGISTER | (isay ? RWF_QUICKWRITE : RWF_WRITE), DEVRW_A8D8, 0, (void **)&wr);
    f = fopen(argv[6], "r");
    { char ln[200]; while (fgets(ln, sizeof ln, f)) { double t; char a[32], b[32]; if (sscanf(ln, "%lf %31s %31s", &t, a, b) == 3) { ev[ne].t = t; ev[ne].r = strtoul(a, 0, 0); ev[ne].v = strtoul(b, 0, 0); ne++; } } }
    fclose(f);
    fprintf(stderr, "rate %u\n", inf.sampleRate);
    total = (UINT32)(secs * inf.sampleRate);
    f = fopen(argv[7], "wb");
    while (done < total) {
        UINT32 n = 64; DEV_SMPL *o[2]; static DEV_SMPL L[64], R[64]; UINT32 i;
        double tnow = (double)done / inf.sampleRate * 1000.0;
        while (ie < ne && ev[ie].t <= tnow) { wr(inf.dataPtr, ev[ie].r, ev[ie].v); ie++; }
        o[0] = L; o[1] = R; memset(L, 0, sizeof L); memset(R, 0, sizeof R);
        inf.devDef->Update(inf.dataPtr, n, o);
        for (i = 0; i < n; i++) { int32_t s[2] = {L[i], R[i]}; fwrite(s, 8, 1, f); }
        done += n;
    }
    fclose(f); SndEmu_Stop(&inf); return 0;
}
