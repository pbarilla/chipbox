/* SPDX-License-Identifier: GPL-3.0-only */
/* ref DEV CORE SECS OUT.f32 < script   (DEV: gb sms ngp pce vsu ws mikey; CORE: 0 default, or 4 letters)
   script lines: SAMPLE_44100 CHIPIDX ADDR DATA   (numbers decimal or 0x..); output f32 stereo @44100, area-weighted from the core's native rate */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stdtype.h"
#include "emu/EmuStructs.h"
#include "emu/SoundEmu.h"
#include "emu/SoundDevs.h"
#include "emu/EmuCores.h"
#include "emu/cores/sn764intf.h"
typedef struct { long t; int chip, a, d; } ev_t;
static DEV_INFO inf[2]; static DEVFUNC_WRITE_A8D8 wr[2]; static DEVFUNC_WRITE_A16D8 wr16, wrmem; static int is16; static int nchip = 1;
static double rate; static double acc[2], pos; /* decimation state */
static float *out; static long nout;
static void feed(DEV_SMPL *l, DEV_SMPL *r, long n)   /* ZOH native samples -> mean over each 1/44100 interval */
{
    static double nextb = 0; /* end of the current output interval, in native samples */
    static long oi = 0; static double a0 = 0, a1 = 0, filled = 0; long i;
    double step = rate / 44100.0;
    if (nextb == 0) nextb = step;
    for (i = 0; i < n; i++) {
        double s0 = pos, s1 = pos + 1.0;
        while (s0 < s1) {
            double e = s1 < nextb ? s1 : nextb;
            a0 += l[i] * (e - s0); a1 += r[i] * (e - s0); filled += e - s0; s0 = e;
            if (e >= nextb - 1e-9) { if (oi < nout) { out[2 * oi] = (float)(a0 / filled); out[2 * oi + 1] = (float)(a1 / filled); } oi++; a0 = a1 = filled = 0; nextb += step; }
        }
        pos += 1.0;
    }
}
int main(int argc, char **argv)
{
    DEV_GEN_CFG g; const char *dev; ev_t *ev = 0; long nev = 0, cap = 0, i, done = 0; double secs; DEV_ID id; int retv;
    SN76496_CFG sn; void *cfg = &g;
    if (argc < 5) return 2;
    dev = argv[1]; secs = atof(argv[3]);
    memset(&g, 0, sizeof g);
    if (argv[2][0] >= 'A') g.emuCore = ((UINT32)argv[2][0] << 24) | ((UINT32)argv[2][1] << 16) | ((UINT32)argv[2][2] << 8) | (UINT32)argv[2][3];
    g.srMode = DEVRI_SRMODE_NATIVE; g.smplRate = 44100;
    if (!strcmp(dev, "gb")) { id = DEVID_GB_DMG; g.clock = 4194304; }
    else if (!strcmp(dev, "pce")) { id = DEVID_C6280; g.clock = 3579545; }
    else if (!strcmp(dev, "vsu")) { id = DEVID_VBOY_VSU; g.clock = 5000000; is16 = 1; }
    else if (!strcmp(dev, "ws")) { id = DEVID_WSWAN; g.clock = 3072000; }
    else if (!strcmp(dev, "mikey")) { id = DEVID_MIKEY; g.clock = 16000000; }
    else if (!strcmp(dev, "sms") || !strcmp(dev, "ngp")) {
        id = DEVID_SN76496; memset(&sn, 0, sizeof sn); sn._genCfg = g; sn.shiftRegWidth = 16; sn.noiseTaps = 0x09; sn.negate = 1; sn.clkDiv = 8; sn.segaPSG = 1; sn.stereo = 0;
        sn._genCfg.clock = !strcmp(dev, "sms") ? 3579545 : 3072000; if (!strcmp(dev, "ngp")) { sn._genCfg.flags = 1; sn.segaPSG = 0; nchip = 2; }
        cfg = &sn;
    } else return 2;
    { char line[256]; while (fgets(line, sizeof line, stdin)) { long t; int c, a, d;
        if (sscanf(line, "%ld %i %i %i", &t, &c, &a, &d) == 4) { if (nev == cap) { cap = cap ? cap * 2 : 256; ev = realloc(ev, cap * sizeof *ev); } ev[nev].t = t; ev[nev].chip = c; ev[nev].a = a; ev[nev].d = d; nev++; } } }
    for (i = 0; i < nchip; i++) {
        if (id == DEVID_SN76496 && nchip == 2) {
            if (i == 1) sn.t6w28_tone = inf[0].dataPtr;       /* the noise half is made second */
            if (i == 1) sn._genCfg.emuCore = inf[0].devDef->coreID;
            sn._genCfg.flags = 1;
        }
        retv = SndEmu_Start(id, (DEV_GEN_CFG *)cfg, &inf[i]);
        if (retv) { fprintf(stderr, "start failed %d\n", retv); return 1; }
        inf[i].devDef->Reset(inf[i].dataPtr);
        if (is16) SndEmu_GetDeviceFunc(inf[i].devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A16D8, 0, (void **)&wr16); else SndEmu_GetDeviceFunc(inf[i].devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void **)&wr[i]); SndEmu_GetDeviceFunc(inf[i].devDef, RWF_MEMORY | RWF_WRITE, DEVRW_A16D8, 0, (void **)&wrmem);
    }
    rate = inf[0].sampleRate;
    fprintf(stderr, "core %s native rate %.0f\n", inf[0].devDef->name, rate);
    nout = (long)(secs * 44100); out = calloc(nout * 2, sizeof(float));
    {
        long total = (long)(secs * rate) + 2, maxb = 4096; DEV_SMPL *bl = malloc(maxb * 4), *br = malloc(maxb * 4), *bl2 = malloc(maxb * 4), *br2 = malloc(maxb * 4);
        long k = 0;
        while (done < total) {
            long target = k < nev ? (long)llround((double)ev[k].t * rate / 44100.0) : total;
            if (target > total) target = total;
            while (done < target) {
                long n = target - done < maxb ? target - done : maxb; DEV_SMPL *o[2]; DEV_SMPL *o2[2]; long j;
                o[0] = bl; o[1] = br; o2[0] = bl2; o2[1] = br2;
                inf[0].devDef->Update(inf[0].dataPtr, (UINT32)n, o);
                if (nchip == 2) { inf[1].devDef->Update(inf[1].dataPtr, (UINT32)n, o2); }
                if (nchip == 2) for (j = 0; j < n; j++) { bl[j] += bl2[j]; br[j] += br2[j]; }
                feed(bl, br, n); done += n;
            }
            while (k < nev && (long)llround((double)ev[k].t * rate / 44100.0) <= done) { if (ev[k].a >= 0x10000) { if (wrmem) wrmem(inf[0].dataPtr, (UINT16)(ev[k].a - 0x10000), (UINT8)ev[k].d); } else if (is16) wr16(inf[0].dataPtr, (UINT16)ev[k].a, (UINT8)ev[k].d); else wr[ev[k].chip](inf[ev[k].chip].dataPtr, (UINT8)ev[k].a, (UINT8)ev[k].d); k++; }
            if (k >= nev && done >= total) break;
        }
    }
    { FILE *f = fopen(argv[4], "wb"); fwrite(out, 8, nout, f); fclose(f); }
    return 0;
}
