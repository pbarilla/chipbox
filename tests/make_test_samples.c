/* SPDX-License-Identifier: GPL-3.0-only */
/* The TEST projects (site/projects/samples/test-*.chipbox): made with the firmware's own code (chips.c, proj_pack), so every new field is stored exactly as the device stores it.
 * A channel = one patch that differs from its reference in ONE thing. A page = the reference and up to three variants one after the other (four steps each, the same note),
 * so a page is an A / B listening. Page 1 of every project is the reference alone.
 *   python3 tools/make_test_samples.py        (builds this, writes the .chipbox files, samples.json and tests/test_samples.txt) */
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

static char leg[MAXCH][48];                     /* what each channel is */
static char pgs[64][160];                       /* what each page plays */
static FILE *lf;
static int ref_of[MAXCH];
static int fail;
static uint8_t slotused[64][4];                  /* a slot that has a note on its page (the check wants it to sound) */

static void begin(const char *title, uint32_t bpm)
{
    seq_clear(); seq_bpm = bpm; memset(slotused, 0, sizeof slotused); memset(leg, 0, sizeof leg); memset(pgs, 0, sizeof pgs); memset(ref_of, 0, sizeof ref_of);
    fprintf(lf, "\n%s\n", title);
}
static int add(uint32_t kind, const char *lab)
{
    int c = chan_add(kind);
    if (c < 0) { printf("TOO MANY CHANNELS at %s\n", lab); exit(1); }
    snprintf(leg[c], sizeof leg[c], "%s", lab);
    return c;
}
#define FO(c, j, f, v) (chan[c].fm.op[j][f] = (uint8_t)(v))
#define FC(c, f, v) (chan[c].fm.ch[f] = (uint8_t)(v))
#define IN(c) (chan[c].in)
static void put(uint32_t p, uint32_t c, uint32_t s, uint32_t note, uint32_t gate)
{
    step_t *x = STEP_W(p, c, s);
    x->note = (uint8_t)note; x->vel = 110; x->gate = (uint8_t)gate;
}
static uint32_t npage;
static uint32_t newpage(void)
{
    uint32_t p = npage++;
    if (p) page_n = npage;
    song_len = npage; song[p] = (uint8_t)p;
    return p;
}
static void drumbar(uint32_t p, uint32_t c)                      /* a plain kit bar: kick, snare, closed hats (lanes 0, 1, 7) */
{
    uint32_t s;
    for (s = 0; s < 16u; s++) {
        uint32_t hits = (s % 4u == 0u ? 1u : 0u) | (s == 4u || s == 12u ? 2u : 0u) | (s % 2u == 0u ? 128u : 0u);
        if (hits) dstep_set(STEP_W(p, c, s), hits, s == 0u ? 1u : 0u);
    }
}
/* a page: the reference and up to three variants, four steps each, one long note; note n; c = -1 leaves a slot empty */
static void pv(uint32_t note, int a, int b, int c, int d)
{
    const int ch[4] = {a, b, c, d};
    uint32_t p = newpage(), i, n = 0;
    char *o = pgs[p];
    for (i = 0; i < 4u; i++) {
        if (ch[i] < 0) continue;
        put(p, (uint32_t)ch[i], i * 4u, note, 17); slotused[p][i] = 1;
        n += (uint32_t)snprintf(o + n, sizeof pgs[0] - n, "%s%s", i ? " | " : "", leg[ch[i]]);
    }
}
static void pref(uint32_t note, int a)                           /* page 1: the reference alone, the phrase twice */
{
    uint32_t p = newpage();
    put(p, (uint32_t)a, 0, note, 17); put(p, (uint32_t)a, 4, note + 7u, 17); put(p, (uint32_t)a, 8, note, 23); put(p, (uint32_t)a, 12, note + 12u, 11);
    snprintf(pgs[p], sizeof pgs[0], "%s alone", leg[a]);
}
static void pkick(uint32_t p, int c, uint32_t every)             /* kick drum steps */
{
    uint32_t s;
    for (s = 0; s < 16u; s += every) dstep_set(STEP_W(p, c, s), 1u, s == 0u ? 1u : 0u);
}

/* ---- FM ------------------------------------------------------------------------------------------------------------------------------------------------ */
static void p_fm4a(void)                                         /* OPM: the eight algorithms, feedback and the operator registers */
{
    int r, a[8], fb, tl, mul, dt1, dt2, ar, fall, rr, i;
    begin("Test: FM 4-op A (OPM)", 110);
    r = add(KD_OPM, "OPM plain (alg 4)");
    for (i = 0; i < 8; i++) if (i != 4) { char t[48]; snprintf(t, sizeof t, "OPM alg %d", i); a[i] = add(KD_OPM, t); FC(a[i], CF_ALG, i); }
    fb = add(KD_OPM, "OPM FB 7"); FC(fb, CF_FB, 7);
    tl = add(KD_OPM, "OPM op1 TL 6 (loud modulator)"); FO(tl, 0, OF_TL, 6);
    mul = add(KD_OPM, "OPM op1 MUL 5"); FO(mul, 0, OF_MUL, 5);
    dt1 = add(KD_OPM, "OPM op4 DT1 6"); FO(dt1, 3, OF_DT1, 6);
    dt2 = add(KD_OPM, "OPM op4 DT2 2"); FO(dt2, 3, OF_DT2, 2);
    ar = add(KD_OPM, "OPM op4 AR 11 (slow attack)"); FO(ar, 3, OF_AR, 11); FO(ar, 1, OF_AR, 11);
    fall = add(KD_OPM, "OPM op2+4 D1R 24 D1L 8 D2R 9"); FO(fall, 1, OF_D1R, 24); FO(fall, 1, OF_D1L, 8); FO(fall, 1, OF_D2R, 9); FO(fall, 3, OF_D1R, 24); FO(fall, 3, OF_D1L, 8); FO(fall, 3, OF_D2R, 9);
    rr = add(KD_OPM, "OPM op2+4 RR 2 (long release)"); FO(rr, 1, OF_RR, 2); FO(rr, 3, OF_RR, 2);
    pref(60, r);
    pv(60, r, a[0], a[1], a[2]);
    pv(60, r, a[3], a[5], a[6]);
    pv(60, r, a[7], fb, tl);
    pv(60, r, mul, dt1, dt2);
    pv(60, r, ar, fall, rr);
}
static void p_fm4b(void)                                         /* OPZ, OPM (LFO, key scale), Mega Drive, YM2203, PC-98, Neo Geo */
{
    int oz, ozw, ozf, ozs, ozx, om, oks, ovib, otrem, md, mdl, mdl2, on, onks, pcl, ng;
    begin("Test: FM 4-op B (OPZ, OPM, MD, OPN, PC-98, Neo Geo)", 110);
    oz = add(KD_OPZ, "OPZ plain");
    ozw = add(KD_OPZ, "OPZ op4 WAVE 3"); FO(ozw, 3, OF_WAVE, 3);
    ozf = add(KD_OPZ, "OPZ op4 FINE 8"); FO(ozf, 3, OF_FINE, 8);
    ozs = add(KD_OPZ, "OPZ op2+4 SHIFT 3"); FO(ozs, 1, OF_SHIFT, 3); FO(ozs, 3, OF_SHIFT, 3);
    ozx = add(KD_OPZ, "OPZ op4 FIX 1"); FO(ozx, 3, OF_FIX, 1); FO(ozx, 3, OF_MUL, 5);
    om = add(KD_OPM, "OPM plain");
    oks = add(KD_OPM, "OPM op2+4 KS 1 (note 84)"); FO(oks, 1, OF_KS, 1); FO(oks, 3, OF_KS, 1);
    ovib = add(KD_OPM, "OPM LFO PMS 5 PMD 100 LFS 150 tri"); FC(ovib, CF_PMS, 5); FC(ovib, CF_PMD, 100); FC(ovib, CF_LFS, 150); FC(ovib, CF_LFW, 2);
    otrem = add(KD_OPM, "OPM LFO AMS 3 AMD 127 op4 AM"); FC(otrem, CF_AMS, 3); FC(otrem, CF_AMD, 127); FC(otrem, CF_LFS, 150); FC(otrem, CF_LFW, 2); FO(otrem, 3, OF_AM, 1);
    md = add(KD_MD, "MD plain");
    mdl = add(KD_MD, "MD pan left, LFO PMS 6 LFS 4"); FC(mdl, CF_PAN, 0); FC(mdl, CF_PMS, 6); FC(mdl, CF_LFS, 4);
    mdl2 = add(KD_MD, "MD op4 AM AMS 3 LFS 4"); FC(mdl2, CF_AMS, 3); FC(mdl2, CF_LFS, 4); FO(mdl2, 3, OF_AM, 1);
    on = add(KD_OPN_FM, "YM2203 plain");
    onks = add(KD_OPN_FM, "YM2203 op2+4 KS 2, op4 DT1 6"); FO(onks, 1, OF_KS, 2); FO(onks, 3, OF_KS, 2); FO(onks, 3, OF_DT1, 6);
    pcl = add(KD_PC98_FM, "PC-98 pan right, LFO PMS 5"); FC(pcl, CF_PAN, 2); FC(pcl, CF_PMS, 5); FC(pcl, CF_LFS, 5);
    ng = add(KD_NEO_FM, "Neo Geo FM alg 5"); FC(ng, CF_ALG, 5);
    pref(60, oz);
    pv(60, oz, ozw, ozf, -1);
    pv(60, oz, ozs, ozx, -1);
    pv(60, om, ovib, otrem, -1);
    pv(84, om, oks, -1, -1);
    pv(60, md, mdl, mdl2, -1);
    pv(60, on, onks, -1, -1);
    pv(60, md, pcl, ng, -1);
}
static void p_fm2(void)                                          /* AdLib, MSX-Audio, Sound Blaster (OPL3), MSX Music (OPLL), VRC7 */
{
    int o, om, ok, oh, otr, ovb, ow, l1b, o3, o3a, o3b, o3c, l, l2, l3, v2;
    begin("Test: FM 2-op (AdLib, MSX-Audio, OPL3, OPLL, VRC7)", 110);
    o = add(KD_OPL, "AdLib plain");
    om = add(KD_OPL, "AdLib mod MULT 4"); FO(om, 0, OF_MUL, 4);
    ok = add(KD_OPL, "AdLib car KSL 2 (note 84)"); FO(ok, 1, OF_KSL, 2);
    oh = add(KD_OPL, "AdLib HOLD (EGT) both"); FO(oh, 0, OF_EGT, 1); FO(oh, 1, OF_EGT, 1);
    otr = add(KD_OPL, "AdLib car TREM, DAM 1"); FO(otr, 1, OF_AM, 1); FC(otr, CF_DAM, 1);
    ovb = add(KD_OPL, "AdLib car VIB, DVB 1"); FO(ovb, 1, OF_VIB, 1); FC(ovb, CF_DVB, 1);
    ow = add(KD_OPL, "AdLib mod WAVE 1 car WAVE 3, FB 7"); FO(ow, 0, OF_WAVE, 1); FO(ow, 1, OF_WAVE, 3); FC(ow, CF_FB, 7);
    l1b = add(KD_OPL1, "MSX-Audio mod MULT 4 FB 6"); FO(l1b, 0, OF_MUL, 4); FC(l1b, CF_FB, 6);
    o3 = add(KD_OPL3, "OPL3 plain (conn 2)");
    o3a = add(KD_OPL3, "OPL3 conn 0"); FC(o3a, CF_ALG, 0);
    o3b = add(KD_OPL3, "OPL3 conn 3"); FC(o3b, CF_ALG, 3);
    o3c = add(KD_OPL3, "OPL3 op2+4 WAVE 6, pan left"); FO(o3c, 1, OF_WAVE, 6); FO(o3c, 3, OF_WAVE, 6); FC(o3c, CF_PAN, 0);
    l = add(KD_OPLL, "OPLL inst 1 plain");
    l2 = add(KD_OPLL, "OPLL inst 8"); fm_opll_load(&chan[l2].fm, KD_OPLL, 7u);
    l3 = add(KD_OPLL, "OPLL custom: inst 1, mod TL 8, car MULT 2"); FO(l3, 0, OF_TL, 8); FO(l3, 1, OF_MUL, 2); FC(l3, CF_EDIT, 1);
    v2 = add(KD_VRC7, "VRC7 custom: inst 1, mod WAVE 1, FB 7"); FO(v2, 0, OF_WAVE, 1); FC(v2, CF_FB, 7); FC(v2, CF_EDIT, 1);
    pref(60, o);
    pv(60, o, om, oh, otr);
    pv(60, o, ovb, ow, -1);
    pv(84, o, ok, -1, -1);
    pv(60, o, l1b, -1, -1);
    pv(60, o3, o3a, o3b, o3c);
    pv(60, l, l2, l3, v2);
}

/* ---- PSG ----------------------------------------------------------------------------------------------------------------------------------------------- */
static void p_psg1(void)                                         /* AY and the SSG chips, SAA1099, POKEY */
{
    int ay, n15, np3, np12, b3, b5, b7, ss, ssb, sa, se3, se5, sn, pk, ph, pc;
    begin("Test: PSG knobs (AY, SSG, SAA, POKEY)", 110);
    ay = add(KD_AY, "AY plain tone");
    n15 = add(KD_AY, "AY NOISE 15 (follows the note)"); IN(n15).wave = 15;
    np3 = add(KD_AY, "AY NOISE 15 PERIOD 3"); IN(np3).wave = 15; IN(np3).wf = 3;
    np12 = add(KD_AY, "AY NOISE 15 PERIOD 12"); IN(np12).wave = 15; IN(np12).wf = 12;
    b3 = add(KD_AY, "AY BUZZ 3"); IN(b3).wc = 3;
    b5 = add(KD_AY, "AY BUZZ 5 BRATIO 4"); IN(b5).wc = 5; IN(b5).wt = 4;
    b7 = add(KD_AY, "AY BUZZ 7 BRATIO 12"); IN(b7).wc = 7; IN(b7).wt = 12;
    ss = add(KD_PC98_SSG, "PC-98 SSG plain"); ssb = add(KD_PC98_SSG, "PC-98 SSG BUZZ 4"); IN(ssb).wc = 4;
    sa = add(KD_SAA, "SAA plain");
    se3 = add(KD_SAA, "SAA ENV 3"); IN(se3).wf = 3;
    se5 = add(KD_SAA, "SAA ENV 6 ERES"); IN(se5).wf = 6; IN(se5).wc = 1;
    sn = add(KD_SAA, "SAA NOISE 15 NRATE 3"); IN(sn).wave = 15; IN(sn).wt = 3;
    pk = add(KD_POKEY, "POKEY plain (noisy 5)"); IN(pk).wave = 5;
    ph = add(KD_POKEY, "POKEY HPF 9"); IN(ph).wave = 5; IN(ph).wf = 9;
    pc = add(KD_POKEY, "POKEY CLOCK 1"); IN(pc).wave = 5; IN(pc).wc = 1;
    pref(60, ay);
    pv(60, ay, n15, np3, np12);
    pv(48, ay, b3, b5, b7);
    pv(60, ss, ssb, -1, -1);
    pv(60, sa, se3, se5, sn);
    pv(60, pk, ph, pc, -1);
}
static void p_psg2(void)                                         /* Lynx, SN76477, N163, VRC6, TIA */
{
    int ly, li, lt, lb, sn, sf, sr, sd, sdu, nl, n8, n64, vs, vo, ti, tt;
    begin("Test: PSG knobs 2 (Lynx, SN76477, N163, VRC6, TIA)", 110);
    ly = add(KD_LYNX, "Lynx plain");
    li = add(KD_LYNX, "Lynx INTEG"); IN(li).wf = 1;
    lt = add(KD_LYNX, "Lynx wave 4 TAPS 6"); IN(lt).wave = 4; IN(lt).wc = 6;
    lb = add(KD_LYNX, "Lynx BAL 9 (right)"); IN(lb).wt = 9;
    sn = add(KD_SN77, "SN76477 plain");
    sf = add(KD_SN77, "SN76477 FILTER 3 (noise)"); IN(sf).wave = 2; IN(sf).wf = 3;
    sr = add(KD_SN77, "SN76477 noise RATE 12"); IN(sr).wave = 2; IN(sr).wc = 12;
    sd = add(KD_SN77, "SN76477 siren DEPTH 12"); IN(sd).wave = 3; IN(sd).wt = 12; IN(sd).pan = 2;
    sdu = add(KD_SN77, "SN76477 DUTY 12"); IN(sdu).duty = 12;
    nl = add(KD_N163, "N163 saw, plain (32)"); IN(nl).wave = 5;
    n8 = add(KD_N163, "N163 saw, LENGTH 8"); IN(n8).wave = 5; IN(n8).duty = 2;
    n64 = add(KD_N163, "N163 saw, LENGTH 4"); IN(n64).wave = 5; IN(n64).duty = 1;
    vs = add(KD_VRC6_SAW, "VRC6 saw plain");
    vo = add(KD_VRC6_SAW, "VRC6 saw OVER 8"); IN(vo).duty = 8;
    ti = add(KD_TIA, "TIA TYPE 6"); IN(ti).wave = 6;
    tt = add(KD_TIA, "TIA TYPE 4"); IN(tt).wave = 4;
    pref(60, ly);
    pv(60, ly, li, lt, lb);
    pv(60, sn, sf, sr, sd);
    pv(60, sn, sdu, -1, -1);
    pv(48, nl, n8, n64, -1);
    pv(48, vs, vo, -1, -1);
    pv(60, ti, tt, -1, -1);
}
static void p_psg3(void)                                         /* 5B and SID */
{
    int fb, fp, fr, fm, fe, s, sc, sr, sh, sm, sy, rg, pw;
    begin("Test: 5B and SID", 110);
    fb = add(KD_S5B, "5B plain tone");
    fp = add(KD_S5B, "5B NOISE 15 PERIOD 4"); IN(fp).wave = 15; IN(fp).wf = 4;
    fr = add(KD_S5B, "5B BUZZ 2 RATE 3"); IN(fr).pan = 2; IN(fr).wc = 3;
    fm = add(KD_S5B, "5B BUZZ 2 RATE 3 MIXENV"); IN(fm).pan = 2; IN(fm).wc = 3; IN(fm).wt = 1;
    fe = add(KD_S5B, "5B BUZZ 6 plucks"); IN(fe).pan = 6;
    s = add(KD_SID, "SID plain (pulse)");
    sc = add(KD_SID, "SID low-pass CUT 5 RES 9"); sid_put(&IN(sc), SP_MODE, 1); sid_put(&IN(sc), SP_CUT, 5); sid_put(&IN(sc), SP_RES, 9);
    sr = add(KD_SID, "SID band-pass CUT 8 RES 12"); sid_put(&IN(sr), SP_MODE, 2); sid_put(&IN(sr), SP_CUT, 8); sid_put(&IN(sr), SP_RES, 12);
    sh = add(KD_SID, "SID high-pass CUT 6"); sid_put(&IN(sh), SP_MODE, 4); sid_put(&IN(sh), SP_CUT, 6); sid_put(&IN(sh), SP_RES, 6);
    sm = add(KD_SID, "SID LP CUT 3 FENV 12"); sid_put(&IN(sm), SP_MODE, 1); sid_put(&IN(sm), SP_CUT, 3); sid_put(&IN(sm), SP_RES, 8); sid_put(&IN(sm), SP_FENV, 12);
    sy = add(KD_SID, "SID SYNC 6"); sid_put(&IN(sy), SP_SYNC, 6);
    rg = add(KD_SID, "SID RING 5, triangle"); sid_put(&IN(rg), SP_RING, 5); sid_put(&IN(rg), SP_WAVE, 0);
    pw = add(KD_SID, "SID PW 3 PWM 9"); sid_put(&IN(pw), SP_PW, 3); sid_put(&IN(pw), SP_PWM, 9);
    pref(48, fb);
    pv(48, fb, fp, fr, fm);
    pv(48, fb, fe, -1, -1);
    pv(48, s, sc, sr, sh);
    pv(48, s, sm, sy, rg);
    pv(48, s, pw, -1, -1);
}
static void p_noise(void)                                        /* melodic noise, SMS / NGP NOISE */
{
    int nn, nm, np, gn, gw, pn, pr, vn, vt, sm, s1, s4, s8, ng, ng2;
    begin("Test: noise", 110);
    nn = add(KD_NES_NOI, "NES noise plain");
    nm = add(KD_NES_NOI, "NES noise MODE short"); IN(nm).duty = 1;
    np = add(KD_NES_NOI, "NES noise PERIOD 6"); IN(np).wf = 6;
    gn = add(KD_GB_NOI, "GB noise plain"); gw = add(KD_GB_NOI, "GB noise WIDTH 7 bit"); IN(gw).duty = 1;
    pn = add(KD_PCE_NOI, "PCE noise plain"); pr = add(KD_PCE_NOI, "PCE noise RATE 8"); IN(pr).duty = 8;
    vn = add(KD_VB_NOI, "VB noise plain"); vt = add(KD_VB_NOI, "VB noise TAP 7"); IN(vt).duty = 7;
    sm = add(KD_SMS, "SMS tone");
    s1 = add(KD_SMS, "SMS NOISE 1 white"); IN(s1).wave = 1;
    s4 = add(KD_SMS, "SMS NOISE 4 periodic"); IN(s4).wave = 4;
    s8 = add(KD_SMS, "SMS NOISE 8 fixed rate"); IN(s8).wave = 8;
    ng = add(KD_NGP, "NGP NOISE 2"); IN(ng).wave = 2;
    ng2 = add(KD_NGP, "NGP NOISE 7"); IN(ng2).wave = 7;
    pref(60, nn);
    pv(60, nn, nm, np, -1);
    pv(60, gn, gw, -1, -1);
    pv(60, pn, pr, -1, -1);
    pv(60, vn, vt, -1, -1);
    pv(60, sm, s1, s4, s8);
    pv(60, sm, ng, ng2, -1);
}
static void p_sweep(void)                                        /* sweep, vibrato speed, the console kits */
{
    int g, gs, gs2, gv1, gv2, p0, ps, v0, vs, n0, ns, np0, nps, kn, kg, ksm;
    begin("Test: sweep, vibrato, kits", 110);
    g = add(KD_GB_PU, "GB pulse plain");
    gs = add(KD_GB_PU, "GB pulse SWEEP +5"); IN(gs).sweep = 5;
    gs2 = add(KD_GB_PU, "GB pulse SWEEP -26"); IN(gs2).sweep = -26;
    gv1 = add(KD_GB_PU, "GB pulse VIB 15 VSPD 3"); IN(gv1).vib_dep = 15; IN(gv1).vib_spd = 3;
    gv2 = add(KD_GB_PU, "GB pulse VIB 15 VSPD 14"); IN(gv2).vib_dep = 15; IN(gv2).vib_spd = 14;
    p0 = add(KD_PCE, "PCE plain"); ps = add(KD_PCE, "PCE SWEEP +16"); IN(ps).sweep = 16;
    v0 = add(KD_VB_WAV, "VB plain"); vs = add(KD_VB_WAV, "VB SWEEP -10"); IN(vs).sweep = -10;
    n0 = add(KD_NGP, "NGP plain"); ns = add(KD_NGP, "NGP SWEEP +8"); IN(ns).sweep = 8;
    np0 = add(KD_NES_PU, "NES pulse plain"); nps = add(KD_NES_PU, "NES pulse SWEEP +34"); IN(nps).sweep = 34;
    kn = add(KD_DRUM0, "NES kit"); kg = add(KD_DRUM0 + 1, "GB kit"); ksm = add(KD_DRUM0 + 2, "SMS kit");
    pref(60, g);
    pv(60, g, gs, gs2, -1);
    pv(60, g, gv1, gv2, -1);
    pv(60, p0, ps, -1, -1);
    pv(60, v0, vs, -1, -1);
    pv(60, n0, ns, -1, -1);
    pv(60, np0, nps, -1, -1);
    {   uint32_t p = newpage(), i, k;                            /* the kits one after the other, four steps each: kick, hat, snare, hat */
        const int kc[3] = {kn, kg, ksm};
        for (k = 0; k < 3u; k++) for (i = 0; i < 4u; i++) dstep_set(STEP_W(p, kc[k], k * 4u + i), i == 0u ? 1u : i == 2u ? 2u : 128u, i == 0u ? 1u : 0u);
        slotused[p][0] = slotused[p][1] = slotused[p][2] = 1;
        snprintf(pgs[p], sizeof pgs[0], "%s | %s | %s", leg[kn], leg[kg], leg[ksm]);
    }
}
static void p_draw(void)                                         /* DRAW: a drawn wave on each chip with a wave RAM */
{
    static const uint32_t K[8] = {KD_GB_WAV, KD_PCE, KD_VB_WAV, KD_WS, KD_SCC, KD_N163, KD_FDS, KD_C30};
    static const char *const SH[4] = {"pulse 1/8", "two harmonics", "staircase", "noisy"};
    int ch[16], n = 0, i;
    int ref[8];
    (void)ch; (void)n;
    begin("Test: draw waves", 110);
    for (i = 0; i < 8; i++) ref[i] = -1;
    struct { int k, shape, len; } L[16] = {{0, -1, 0}, {0, 0, 0}, {0, 2, 0}, {1, -1, 0}, {1, 1, 0}, {2, -1, 0}, {2, 3, 0}, {3, -1, 0}, {3, 2, 0}, {4, -1, 0}, {4, 1, 0},
                                           {5, 1, 2}, {5, 0, 3}, {5, 3, 5}, {6, 1, 0}, {7, 0, 0}};
    int idx[16];
    for (i = 0; i < 16; i++) {
        char t[48];
        uint32_t c, j, nn, top, kind = K[L[i].k];
        static const char *const NM[8] = {"GB wave", "PCE", "VB", "WonderSwan", "SCC", "N163", "FDS", "C30"};
        if (L[i].shape < 0) snprintf(t, sizeof t, "%s generator wave", NM[L[i].k]);
        else if (L[i].k == 5) snprintf(t, sizeof t, "N163 drawn %s, LENGTH %d", SH[L[i].shape], 4 << (L[i].len - 1));
        else snprintf(t, sizeof t, "%s drawn %s", NM[L[i].k], SH[L[i].shape]);
        c = (uint32_t)add(kind, t); idx[i] = (int)c;
        if (L[i].k == 5) IN(c).duty = (uint8_t)(L[i].len);                       /* LENGTH 4 << (duty - 1): duty 2 = 8, 3 = 16, 5 = 64 */
        if (L[i].shape < 0) { if (L[i].k == 5) IN(c).duty = 0; continue; }
        nn = wd_n(c); top = wd_top(kind);
        for (j = 0; j < nn; j++) {
            double x = (double)j / (double)nn, v;
            switch (L[i].shape) {
            case 0: v = x < 0.125 ? 1.0 : 0.0; break;
            case 1: v = 0.5 + 0.5 * (sin(6.2831853 * x) + 0.5 * sin(2.0 * 6.2831853 * x + 1.0)) / 1.5; break;
            case 2: v = floor(x * 4.0) / 3.0; break;
            default: v = (double)((j * 2654435761u >> 7) & 255u) / 255.0; break;
            }
            chan[c].user[j] = (uint8_t)(v * (double)top + 0.5);
        }
        chan[c].wcus = 1;
    }
    pref(60, idx[0]);
    pv(60, idx[0], idx[1], idx[2], -1);
    pv(60, idx[3], idx[4], -1, -1);
    pv(60, idx[5], idx[6], -1, -1);
    pv(60, idx[7], idx[8], -1, -1);
    pv(60, idx[9], idx[10], -1, -1);
    pv(48, idx[11], idx[12], idx[13], -1);
    pv(60, idx[14], idx[15], -1, -1);
}
static void p_mix(void)                                          /* the rack: level, slid steps, sidechain */
{
    int kick, ref, v[3], d[3], s[3], i;
    begin("Test: rack mix (level, slide, sidechain)", 110);
    kick = add(KD_DRUM0, "NES kick");
    ref = add(KD_NES_PU, "pad plain (vol 15, duck 0)"); IN(ref).duty = 2;
    for (i = 0; i < 3; i++) {
        static const uint8_t VV[3] = {11, 7, 3}, DD[3] = {4, 9, 15};
        char t[48];
        snprintf(t, sizeof t, "pad vol %d", VV[i]); v[i] = add(KD_NES_PU, t); IN(v[i]).duty = 2; IN(v[i]).vol = VV[i];
        snprintf(t, sizeof t, "pad duck %d", DD[i]); d[i] = add(KD_NES_PU, t); IN(d[i]).duty = 2; IN(d[i]).duck = DD[i];
        snprintf(t, sizeof t, "pad slid +%d step", i + 1); s[i] = add(KD_NES_PU, t); IN(s[i]).duty = 2;
    }
    {   uint32_t p = newpage();                                  /* page 1: the pad and the kick */
        put(p, (uint32_t)ref, 0, 60, 95); pkick(p, kick, 4);
        snprintf(pgs[p], sizeof pgs[0], "pad plain + kick every beat");
    }
    {   uint32_t p = newpage();                                  /* levels: 15, 11, 7, 3 */
        int c4[4] = {ref, v[0], v[1], v[2]};
        for (i = 0; i < 4; i++) put(p, (uint32_t)c4[i], (uint32_t)i * 4u, 60, 17);
        snprintf(pgs[p], sizeof pgs[0], "level 15 | 11 | 7 | 3");
    }
    {   uint32_t p = newpage();                                  /* sidechain: duck 0, 4, 9, 15 under a kick on every other step */
        int c4[4] = {ref, d[0], d[1], d[2]};
        for (i = 0; i < 4; i++) put(p, (uint32_t)c4[i], (uint32_t)i * 4u, 60, 23);
        pkick(p, kick, 2);
        snprintf(pgs[p], sizeof pgs[0], "sidechain 0 | 4 | 9 | 15, kick every 2 steps");
    }
    {   uint32_t p = newpage();                                  /* slid: the same four-note phrase on four channels, each a step later */
        int c4[4] = {ref, s[0], s[1], s[2]}, k;
        for (i = 0; i < 4; i++) for (k = 0; k < 4; k++) put(p, (uint32_t)c4[i], (uint32_t)((k * 4 + i) % 16), (uint32_t)(60 + 4 * k), 5);
        snprintf(pgs[p], sizeof pgs[0], "same phrase on 4 channels, 0 | +1 | +2 | +3 steps (KNOB 3)");
    }
    {   uint32_t p = newpage(); drumbar(p, kick); snprintf(pgs[p], sizeof pgs[0], "NES kit: kick snare hats"); }
}

static void hit(uint32_t p, uint32_t c, uint32_t s, uint32_t lanes) { dstep_set(STEP_W(p, c, s), lanes, lanes & 1u ? 1u : 0u); }      /* a drum step: bit 0 kick, 1 snare, 7 closed hat */
static void setmeter(uint32_t p, uint32_t sig, uint32_t bars) { page_sig[p] = (uint8_t)sig; page_xb[p] = (uint8_t)(bars - 1u); }       /* sig: 0 4/4, 1 2/4, 2 3/4, 3 5/4, 4 6/8, 5 7/8 */
static void p_meters(void)                                       /* a song that changes meter: for trying the page meter and length (KNOB 3 and 4 in step entry) */
{
    int dr, ba, me;
    uint32_t p, s;
    begin("Test: meters (4/4, a waltz in 3/4, 6/8, 5/4, 7/8, 2/4 and long pages)", 120);
    dr = add(KD_DRUM0, "NES kit"); ba = add(KD_NES_TRI, "bass (triangle)"); me = add(KD_NES_PU, "melody (pulse)"); IN(me).duty = 2;
    p = newpage(); setmeter(p, 0, 1);                            /* 4/4, one bar */
    for (s = 0; s < 16u; s += 2u) hit(p, (uint32_t)dr, s, 128u | (s == 0u || s == 8u ? 1u : 0u) | (s == 4u || s == 12u ? 2u : 0u));
    put(p, (uint32_t)ba, 0, 36, 23); put(p, (uint32_t)ba, 8, 43, 23);
    put(p, (uint32_t)me, 0, 72, 17); put(p, (uint32_t)me, 4, 76, 17); put(p, (uint32_t)me, 8, 79, 17); put(p, (uint32_t)me, 12, 76, 17);
    snprintf(pgs[p], sizeof pgs[0], "4/4, 1 bar (16 steps): march");
    p = newpage(); setmeter(p, 2, 1);                            /* 3/4, one bar: the waltz */
    hit(p, (uint32_t)dr, 0, 1u | 128u); hit(p, (uint32_t)dr, 4, 128u); hit(p, (uint32_t)dr, 8, 128u);
    put(p, (uint32_t)ba, 0, 48, 11); put(p, (uint32_t)ba, 4, 55, 5); put(p, (uint32_t)ba, 8, 52, 5);
    put(p, (uint32_t)me, 0, 67, 23); put(p, (uint32_t)me, 4, 72, 11); put(p, (uint32_t)me, 8, 76, 11);
    put(p, (uint32_t)me, 14, 84, 11);                            /* past the end of a 12 step page: kept, greyed, silent */
    snprintf(pgs[p], sizeof pgs[0], "3/4, 1 bar (12 steps): waltz; a note on step 15 is past the end (greyed, silent)");
    p = newpage(); setmeter(p, 2, 2);                            /* 3/4, two bars */
    for (s = 0; s < 24u; s += 12u) { hit(p, (uint32_t)dr, s, 1u | 128u); hit(p, (uint32_t)dr, s + 4u, 128u); hit(p, (uint32_t)dr, s + 8u, 128u); }
    put(p, (uint32_t)ba, 0, 48, 11); put(p, (uint32_t)ba, 4, 55, 5); put(p, (uint32_t)ba, 8, 52, 5); put(p, (uint32_t)ba, 12, 41, 11); put(p, (uint32_t)ba, 16, 48, 5); put(p, (uint32_t)ba, 20, 45, 5);
    put(p, (uint32_t)me, 0, 76, 23); put(p, (uint32_t)me, 12, 77, 11); put(p, (uint32_t)me, 16, 76, 5); put(p, (uint32_t)me, 20, 74, 5);
    snprintf(pgs[p], sizeof pgs[0], "3/4, 2 bars (24 steps): the waltz goes on");
    p = newpage(); setmeter(p, 4, 1);                            /* 6/8 */
    for (s = 0; s < 12u; s += 2u) hit(p, (uint32_t)dr, s, 128u | (s == 0u || s == 6u ? 1u : 0u) | (s == 9u ? 2u : 0u));
    put(p, (uint32_t)ba, 0, 45, 11); put(p, (uint32_t)ba, 6, 52, 11);
    put(p, (uint32_t)me, 0, 69, 5); put(p, (uint32_t)me, 2, 72, 5); put(p, (uint32_t)me, 4, 76, 5); put(p, (uint32_t)me, 6, 74, 5); put(p, (uint32_t)me, 8, 72, 5); put(p, (uint32_t)me, 10, 69, 5);
    snprintf(pgs[p], sizeof pgs[0], "6/8, 1 bar (12 steps): two groups of three");
    p = newpage(); setmeter(p, 3, 1);                            /* 5/4 */
    for (s = 0; s < 20u; s += 4u) hit(p, (uint32_t)dr, s, 128u | (s == 0u || s == 12u ? 1u : 0u) | (s == 8u || s == 16u ? 2u : 0u));
    put(p, (uint32_t)ba, 0, 38, 23); put(p, (uint32_t)ba, 12, 38, 23);
    put(p, (uint32_t)me, 0, 74, 11); put(p, (uint32_t)me, 4, 77, 11); put(p, (uint32_t)me, 8, 81, 11); put(p, (uint32_t)me, 12, 77, 11); put(p, (uint32_t)me, 16, 74, 11);
    snprintf(pgs[p], sizeof pgs[0], "5/4, 1 bar (20 steps): two screens in step entry");
    p = newpage(); setmeter(p, 5, 1);                            /* 7/8: 2 + 2 + 3 */
    for (s = 0; s < 14u; s += 2u) hit(p, (uint32_t)dr, s, 128u | (s == 0u || s == 4u ? 1u : 0u) | (s == 8u ? 2u : 0u));
    put(p, (uint32_t)ba, 0, 40, 5); put(p, (uint32_t)ba, 4, 40, 5); put(p, (uint32_t)ba, 8, 40, 11);
    put(p, (uint32_t)me, 0, 76, 5); put(p, (uint32_t)me, 4, 79, 5); put(p, (uint32_t)me, 8, 83, 5); put(p, (uint32_t)me, 10, 81, 5);
    snprintf(pgs[p], sizeof pgs[0], "7/8, 1 bar (14 steps): 2 + 2 + 3");
    p = newpage(); setmeter(p, 1, 1);                            /* 2/4 */
    hit(p, (uint32_t)dr, 0, 1u | 128u); hit(p, (uint32_t)dr, 2, 128u); hit(p, (uint32_t)dr, 4, 2u | 128u); hit(p, (uint32_t)dr, 6, 128u);
    put(p, (uint32_t)ba, 0, 43, 11); put(p, (uint32_t)me, 0, 79, 5); put(p, (uint32_t)me, 4, 79, 5);
    snprintf(pgs[p], sizeof pgs[0], "2/4, 1 bar (8 steps)");
    p = newpage(); setmeter(p, 0, 4);                            /* 4/4, four bars: the longest page */
    for (s = 0; s < 64u; s += 2u) hit(p, (uint32_t)dr, s, 128u | (s % 8u == 0u ? 1u : 0u) | (s % 16u == 4u || s % 16u == 12u ? 2u : 0u));
    {   static const uint8_t SC[16] = {72, 74, 76, 77, 79, 81, 83, 84, 83, 81, 79, 77, 76, 74, 72, 67};
        for (s = 0; s < 64u; s += 4u) put(p, (uint32_t)me, s, SC[s / 4u], 17);
        for (s = 0; s < 64u; s += 16u) { put(p, (uint32_t)ba, s, 36, 47); put(p, (uint32_t)ba, s + 8u, 43, 23); }
    }
    snprintf(pgs[p], sizeof pgs[0], "4/4, 4 bars (64 steps): a scale up and down, four screens");
    p = newpage(); setmeter(p, 0, 2);                            /* back to 4/4, two bars, the last chord */
    for (s = 0; s < 32u; s += 4u) hit(p, (uint32_t)dr, s, 128u | (s % 8u == 0u ? 1u : 0u) | (s == 28u ? 2u : 0u));
    put(p, (uint32_t)ba, 0, 36, 47); put(p, (uint32_t)me, 0, 72, 35); put(p, (uint32_t)me, 16, 76, 23); put(p, (uint32_t)me, 24, 79, 23);
    snprintf(pgs[p], sizeof pgs[0], "4/4, 2 bars (32 steps): home again");
}

/* A showpiece song for the page meter (not a test): an original waltz that wanders through 3/4, 6/8, 5/4, 7/8, 4/4 and 2/4, with pages of several bars (the map line, the screens) */
static void p_waltz(void)
{
    static const uint8_t BASS_A[4] = {48, 43, 45, 41}, BASS_B[4] = {48, 43, 41, 43};                  /* C G Am F, C G F G */
    static const uint8_t ARP_A[4][2] = {{64, 67}, {59, 62}, {60, 64}, {57, 60}}, ARP_B[4][2] = {{64, 67}, {59, 62}, {57, 60}, {59, 62}};
    static const uint8_t MEL_A[4][3][2] = {{{72, 0}, {76, 4}, {79, 8}}, {{79, 0}, {77, 8}, {0, 0}}, {{76, 0}, {72, 4}, {69, 8}}, {{72, 0}, {0, 0}, {0, 0}}};
    static const uint8_t LEN_A[4][3] = {{4, 4, 4}, {8, 4, 0}, {4, 4, 4}, {12, 0, 0}};
    static const uint8_t MEL_B[4][4][2] = {{{79, 0}, {84, 4}, {83, 8}, {84, 10}}, {{86, 0}, {83, 4}, {79, 8}, {0, 0}}, {{81, 0}, {84, 4}, {81, 8}, {0, 0}}, {{83, 0}, {79, 8}, {0, 0}, {0, 0}}};
    static const uint8_t LEN_B[4][4] = {{4, 4, 2, 2}, {4, 4, 4, 0}, {4, 4, 4, 0}, {8, 4, 0, 0}};
    int dr, ba, me, ar;
    uint32_t p, s, b, k;
    begin("Meters waltz (an original waltz that changes meter and tempo)", 126);
    dr = add(KD_DRUM0, "NES kit"); ba = add(KD_NES_TRI, "bass (triangle)"); me = add(KD_NES_PU, "melody (pulse)"); IN(me).duty = 1; ar = add(KD_GB_PU, "accompaniment (Game Boy pulse)"); IN(ar).duty = 2; IN(ar).vol = 9;
    /* 1: 3/4 x 2, the intro: an arpeggio and a bass */
    p = newpage(); setmeter(p, 2, 2); page_tmp[p] = 92; page_gl[p] = 1;       /* a slow start that speeds up into the waltz */
    put(p, (uint32_t)ba, 0, 48, 23); put(p, (uint32_t)ba, 12, 43, 23);
    { static const uint8_t AR[2][6] = {{60, 64, 67, 72, 67, 64}, {59, 62, 67, 71, 67, 62}};
      for (b = 0; b < 2u; b++) for (k = 0; k < 6u; k++) put(p, (uint32_t)ar, b * 12u + k * 2u, AR[b][k], 11); }
    snprintf(pgs[p], sizeof pgs[0], "3/4, 2 bars (24 steps): the intro, an arpeggio over a bass");
    /* 2 and 3: the waltz, four bars of 3/4 (48 steps: three screens) */
    for (k = 0; k < 2u; k++) {
        p = newpage(); setmeter(p, 2, 4); if (k) page_tmp[p] = 138;           /* the second time round, livelier */
        for (b = 0; b < 4u; b++) {
            const uint32_t o = b * 12u;
            hit(p, (uint32_t)dr, o, 1u | 128u); hit(p, (uint32_t)dr, o + 4u, 128u); hit(p, (uint32_t)dr, o + 8u, 128u | (b == 3u ? 2u : 0u));
            put(p, (uint32_t)ba, o, k ? BASS_B[b] : BASS_A[b], 23); put(p, (uint32_t)ba, o + 6u, (k ? BASS_B[b] : BASS_A[b]) + 7u, 11);
            put(p, (uint32_t)ar, o + 4u, k ? ARP_B[b][0] : ARP_A[b][0], 11); put(p, (uint32_t)ar, o + 8u, k ? ARP_B[b][1] : ARP_A[b][1], 11);
            if (!k) { uint32_t j; for (j = 0; j < 3u; j++) if (MEL_A[b][j][0]) put(p, (uint32_t)me, o + MEL_A[b][j][1], MEL_A[b][j][0], (uint32_t)LEN_A[b][j] * 6u - 1u); }
            else { uint32_t j; for (j = 0; j < 4u; j++) if (MEL_B[b][j][0]) put(p, (uint32_t)me, o + MEL_B[b][j][1], MEL_B[b][j][0], (uint32_t)LEN_B[b][j] * 6u - 1u); }
        }
        snprintf(pgs[p], sizeof pgs[0], k ? "3/4, 4 bars (48 steps): the waltz again, the tune higher" : "3/4, 4 bars (48 steps): the waltz");
    }
    /* 4: 6/8 x 2, a jig: two groups of three */
    p = newpage(); setmeter(p, 4, 2); page_tmp[p] = 156;                 /* the jig is quick */
    for (b = 0; b < 2u; b++) {
        static const uint8_t M6[2][6][2] = {{{69, 0}, {72, 2}, {76, 4}, {81, 6}, {76, 10}, {0, 0}}, {{80, 0}, {76, 2}, {71, 4}, {76, 6}, {80, 10}, {0, 0}}};
        static const uint8_t L6[2][6] = {{2, 2, 2, 4, 2, 0}, {2, 2, 2, 4, 2, 0}};
        const uint32_t o = b * 12u;
        for (s = 0; s < 12u; s += 2u) hit(p, (uint32_t)dr, o + s, 128u | (s == 0u || s == 6u ? 1u : 0u) | (s == 9u ? 2u : 0u));
        put(p, (uint32_t)ba, o, b ? 40 : 45, 23); put(p, (uint32_t)ba, o + 6u, b ? 47 : 52, 23);
        put(p, (uint32_t)ar, o + 2u, b ? 56 : 60, 5); put(p, (uint32_t)ar, o + 4u, b ? 59 : 64, 5); put(p, (uint32_t)ar, o + 8u, b ? 56 : 60, 5); put(p, (uint32_t)ar, o + 10u, b ? 59 : 64, 5);
        for (k = 0; k < 6u; k++) if (M6[b][k][0]) put(p, (uint32_t)me, o + M6[b][k][1], M6[b][k][0], (uint32_t)L6[b][k] * 6u - 1u);
    }
    snprintf(pgs[p], sizeof pgs[0], "6/8, 2 bars (24 steps): a jig, two groups of three");
    /* 5: 5/4 x 2, a bridge */
    p = newpage(); setmeter(p, 3, 2); page_tmp[p] = 118;                 /* the bridge lets go a little */
    for (b = 0; b < 2u; b++) {
        static const uint8_t M5[2][5] = {{74, 77, 81, 77, 74}, {79, 83, 86, 83, 79}};
        const uint32_t o = b * 20u;
        for (s = 0; s < 20u; s += 4u) hit(p, (uint32_t)dr, o + s, 128u | (s == 0u || s == 12u ? 1u : 0u) | (s == 8u || s == 16u ? 2u : 0u));
        put(p, (uint32_t)ba, o, b ? 43 : 38, 23); put(p, (uint32_t)ba, o + 12u, b ? 43 : 38, 23);
        put(p, (uint32_t)ar, o + 4u, b ? 59 : 53, 11); put(p, (uint32_t)ar, o + 8u, b ? 62 : 57, 11); put(p, (uint32_t)ar, o + 16u, b ? 59 : 53, 11);
        for (k = 0; k < 5u; k++) put(p, (uint32_t)me, o + k * 4u, M5[b][k], 17);
    }
    snprintf(pgs[p], sizeof pgs[0], "5/4, 2 bars (40 steps): a bridge in five");
    /* 6: 7/8 x 2, a dance: 2 + 2 + 3 */
    p = newpage(); setmeter(p, 5, 2); page_tmp[p] = 144;
    for (b = 0; b < 2u; b++) {
        static const uint8_t M7[2][7] = {{76, 79, 83, 79, 76, 74, 76}, {72, 76, 81, 76, 72, 71, 69}};
        const uint32_t o = b * 14u;
        for (s = 0; s < 14u; s += 2u) hit(p, (uint32_t)dr, o + s, 128u | (s == 0u || s == 4u ? 1u : 0u) | (s == 8u ? 2u : 0u));
        put(p, (uint32_t)ba, o, b ? 45 : 40, 11); put(p, (uint32_t)ba, o + 4u, b ? 45 : 40, 11); put(p, (uint32_t)ba, o + 8u, b ? 45 : 40, 17);
        put(p, (uint32_t)ar, o + 2u, b ? 60 : 55, 5); put(p, (uint32_t)ar, o + 6u, b ? 64 : 59, 5); put(p, (uint32_t)ar, o + 10u, b ? 60 : 55, 5);
        for (k = 0; k < 7u; k++) put(p, (uint32_t)me, o + k * 2u, M7[b][k], 11);
    }
    snprintf(pgs[p], sizeof pgs[0], "7/8, 2 bars (28 steps): a dance in 2 + 2 + 3");
    /* 7: 4/4 x 4, the long page: a scale up and down (four screens) */
    p = newpage(); setmeter(p, 0, 4); page_tmp[p] = 132; page_gl[p] = 2;   /* the long page speeds up into the stutter, late (ease in) */
    for (s = 0; s < 64u; s += 2u) hit(p, (uint32_t)dr, s, 128u | (s % 8u == 0u ? 1u : 0u) | (s % 16u == 4u || s % 16u == 12u ? 2u : 0u));
    {   static const uint8_t SC[16] = {72, 74, 76, 77, 79, 81, 83, 84, 83, 81, 79, 77, 76, 74, 72, 67}, RT[4] = {48, 43, 41, 48};
        static const uint8_t CH[4][4] = {{64, 67, 72, 67}, {59, 62, 67, 62}, {57, 60, 65, 60}, {64, 67, 72, 67}};
        for (s = 0; s < 64u; s += 4u) put(p, (uint32_t)me, s, SC[s / 4u], 17);
        for (b = 0; b < 4u; b++) { put(p, (uint32_t)ba, b * 16u, RT[b], 47); put(p, (uint32_t)ba, b * 16u + 8u, RT[b] + 7u, 23); for (k = 0; k < 8u; k++) put(p, (uint32_t)ar, b * 16u + k * 2u, CH[b][k & 3u], 5); }
    }
    snprintf(pgs[p], sizeof pgs[0], "4/4, 4 bars (64 steps): a scale up and down, four screens");
    /* 8: 2/4 x 2, a stutter */
    p = newpage(); setmeter(p, 1, 2); page_tmp[p] = 160;               /* the stutter is fast */
    for (b = 0; b < 2u; b++) {
        const uint32_t o = b * 8u;
        hit(p, (uint32_t)dr, o, 1u | 128u); hit(p, (uint32_t)dr, o + 2u, 128u); hit(p, (uint32_t)dr, o + 4u, 2u | 128u); hit(p, (uint32_t)dr, o + 6u, 128u);
        put(p, (uint32_t)ba, o, 43, 11); put(p, (uint32_t)ba, o + 4u, 43, 11);
    }
    put(p, (uint32_t)me, 0, 79, 5); put(p, (uint32_t)me, 2, 79, 5); put(p, (uint32_t)me, 4, 79, 11); put(p, (uint32_t)me, 8, 77, 5); put(p, (uint32_t)me, 10, 76, 5); put(p, (uint32_t)me, 12, 74, 5); put(p, (uint32_t)me, 14, 72, 5);
    snprintf(pgs[p], sizeof pgs[0], "2/4, 2 bars (16 steps): a stutter");
    /* 9: the waltz once more, slowing into the end */
    p = newpage(); setmeter(p, 2, 4); page_gl[p] = 2;                    /* slowing into the end, the slowdown coming late (ease in) */
    for (b = 0; b < 4u; b++) {
        const uint32_t o = b * 12u;
        uint32_t j;
        hit(p, (uint32_t)dr, o, 1u | 128u); hit(p, (uint32_t)dr, o + 4u, 128u); hit(p, (uint32_t)dr, o + 8u, 128u | (b == 3u ? 2u : 0u));
        put(p, (uint32_t)ba, o, BASS_A[b], 23); put(p, (uint32_t)ba, o + 6u, BASS_A[b] + 7u, 11);
        put(p, (uint32_t)ar, o + 4u, ARP_A[b][0], 11); put(p, (uint32_t)ar, o + 8u, ARP_A[b][1], 11);
        for (j = 0; j < 3u; j++) if (MEL_A[b][j][0]) put(p, (uint32_t)me, o + MEL_A[b][j][1], MEL_A[b][j][0], (uint32_t)LEN_A[b][j] * 6u - 1u);
    }
    snprintf(pgs[p], sizeof pgs[0], "3/4, 4 bars (48 steps): the waltz, reprise");
    /* 10: 4/4 x 2, the end */
    p = newpage(); setmeter(p, 0, 2); page_tmp[p] = 78;
    for (s = 0; s < 32u; s += 4u) hit(p, (uint32_t)dr, s, 128u | (s % 8u == 0u ? 1u : 0u) | (s == 28u ? 2u : 0u));
    put(p, (uint32_t)ba, 0, 36, 95); put(p, (uint32_t)ba, 16, 43, 95);
    put(p, (uint32_t)me, 0, 72, 95); put(p, (uint32_t)me, 16, 76, 47); put(p, (uint32_t)me, 24, 79, 47);
    put(p, (uint32_t)ar, 0, 64, 5); put(p, (uint32_t)ar, 4, 67, 5); put(p, (uint32_t)ar, 8, 72, 5); put(p, (uint32_t)ar, 12, 67, 5); put(p, (uint32_t)ar, 16, 64, 5); put(p, (uint32_t)ar, 20, 67, 5); put(p, (uint32_t)ar, 24, 72, 5); put(p, (uint32_t)ar, 28, 76, 5);
    snprintf(pgs[p], sizeof pgs[0], "4/4, 2 bars (32 steps): the end");
}
static void waltz_level(void)                                    /* the song through the engine: the loudest sample, and clipping */
{
    static int32_t blk[512];
    uint32_t p, steps = 0, i, n, bi, clip = 0;
    double pk = 0;
    for (p = 0; p < page_n; p++) steps += page_steps(p);
    n = 0;
    for (p = 0; p < page_n; p++) n += (uint32_t)((double)page_steps(p) * 6.0 * (double)FS * 2.5 / (double)page_bpm(p) / 256.0);
    seq_play(0, 0);
    for (bi = 0; bi < n; bi++) { memset(blk, 0, sizeof blk); chip_render(blk, 256); for (i = 0; i < 512; i++) { double v = (double)blk[i] / 32768.0; if (v < 0) v = -v; if (v > pk) pk = v; if (blk[i] >= 32767 || blk[i] <= -32768) clip++; } }
    seq_stop();
    printf("meters-waltz: %u steps, about %.1f s, peak %.1f dBFS, clip %u\n", steps, (double)n * 256.0 / (double)FS, 20.0 * log10(pk + 1e-9), clip);
    if (clip) fail = 1;
}

/* ---- the check: load the bytes back, play the song, level per page ----------------------------------------------------------------------------------- */
static void check(const char *file, const uint8_t *b, uint32_t len)
{
    static int32_t blk[512];
    static double sum[256], mx[256], dif[256], ab[256], prev;
    static uint32_t clip[256];
    uint32_t pages, p, sl, bi, i, per, blocks;
    if (proj_check(b, len) || proj_unpack(b, len)) { printf("%s: DOES NOT LOAD\n", file); fail = 1; return; }
    pages = page_n;
    memset(sum, 0, sizeof sum); memset(mx, 0, sizeof mx); memset(dif, 0, sizeof dif); memset(ab, 0, sizeof ab); prev = 0; memset(clip, 0, sizeof clip);
    seq_play(0, 0);
    per = (uint32_t)((double)FS * 60.0 / (double)seq_bpm / 4.0 * 16.0);       /* samples a page lasts */
    blocks = per * pages / 256u;
    for (bi = 0; bi < blocks; bi++) {
        sl = (uint32_t)((uint64_t)bi * 256u * 4u / per);                       /* the quarter page (four steps) this block is in */
        if (sl >= pages * 4u) sl = pages * 4u - 1u;
        memset(blk, 0, sizeof blk); chip_render(blk, 256);
        for (i = 0; i < 512; i++) { double v = (double)blk[i] / 32768.0; if (fabs(v) > mx[sl]) mx[sl] = fabs(v); sum[sl] += v * v; if (!(i & 1)) { dif[sl] += fabs(v - prev); ab[sl] += fabs(v); prev = v; } if (blk[i] >= 32767 || blk[i] <= -32768) clip[sl]++; }
    }
    seq_stop();
    for (p = 0; p < pages; p++) {
        uint32_t any = slotused[p][0] | slotused[p][1] | slotused[p][2] | slotused[p][3], bad = 0, cl = 0;
        double lo = 0, pk = -200, rms_all = 0;
        for (sl = 0; sl < 4u; sl++) {
            const double rms = 10.0 * log10(sum[p * 4u + sl] / (512.0 * (double)(per / 4u / 256u)) + 1e-12), pp = 20.0 * log10(mx[p * 4u + sl] > 1e-9 ? mx[p * 4u + sl] : 1e-9);
            if (!any || slotused[p][sl]) { if (rms < -50.0) bad |= 1u << sl; if (sl == 0u || rms < lo) lo = rms; }
            if (pp > pk) pk = pp;
            rms_all += rms; cl += clip[p * 4u + sl];
        }
        (void)rms_all;
        for (sl = 1; sl < 4u; sl++) if (slotused[p][0] && slotused[p][sl]) {          /* a variant that sounds the same as its reference: the knob changed nothing you can hear here */
            const double r0 = sum[p * 4u], r1 = sum[p * 4u + sl], c0 = dif[p * 4u] / (ab[p * 4u] + 1e-9), c1 = dif[p * 4u + sl] / (ab[p * 4u + sl] + 1e-9);
            if (fabs(10.0 * log10((r1 + 1e-12) / (r0 + 1e-12))) < 0.4 && fabs(c1 - c0) / (c0 + 1e-9) < 0.04) printf("      SAME as the reference: page %u slot %u\n", p + 1u, sl + 1u);
        }
        if (bad) { for (sl = 0; sl < 4u; sl++) printf("      slot %u rms %6.1f\n", sl + 1u, 10.0 * log10(sum[p * 4u + sl] / (512.0 * (double)(per / 4u / 256u)) + 1e-12)); }
        printf("%-22s page %2u peak %6.1f dBFS, quietest slot %6.1f, clip %u%s\n", file, p + 1u, pk, lo, cl, bad ? "  QUIET SLOT" : cl ? "  CLIPS" : "");
        if (bad || cl) fail = 1;
    }
}
static int no_level;                                             /* the level check assumes 16 step pages */
static void finish(const char *dir, const char *fn)
{
    static uint8_t a[PROJ_BANK];
    char path[256];
    uint32_t len, c, p;
    FILE *f;
    len = proj_pack(a);
    if (!len) { printf("%s: does not fit a bank\n", fn); exit(1); }
    snprintf(path, sizeof path, "%s/%s.bin", dir, fn);
    f = fopen(path, "wb"); fwrite(a, 1, len, f); fclose(f);
    fprintf(lf, "  %u bytes, %u channels, %u pages\n  channels:\n", len, (unsigned)chan_n, (unsigned)page_n);
    for (c = 0; c < chan_n; c++) fprintf(lf, "    %2u  %s\n", c + 1u, leg[c]);
    fprintf(lf, "  pages (steps 1-4 | 5-8 | 9-12 | 13-16):\n");
    for (p = 0; p < page_n; p++) fprintf(lf, "    %2u  %s\n", p + 1u, pgs[p]);
    if (no_level) { if (proj_check(a, len) || proj_unpack(a, len)) { printf("%s: DOES NOT LOAD\n", fn); fail = 1; } else printf("%-22s %u bytes, %u pages, loads\n", fn, len, (unsigned)page_n); }
    else check(fn, a, len);
}
int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[256];
    snprintf(path, sizeof path, "%s/legend.txt", dir);
    lf = fopen(path, "w");
    fprintf(lf, "The test projects (site/projects/samples/test-*.chipbox). A channel differs from the plain one in one thing; a page plays the plain channel and up to three variants, four steps each.\n");
    npage = 0; p_fm4a(); finish(dir, "test-fm-4op-a");
    npage = 0; p_fm4b(); finish(dir, "test-fm-4op-b");
    npage = 0; p_fm2(); finish(dir, "test-fm-2op");
    npage = 0; p_psg1(); finish(dir, "test-psg-knobs");
    npage = 0; p_noise(); finish(dir, "test-noise");
    npage = 0; p_sweep(); finish(dir, "test-sweep");
    npage = 0; p_draw(); finish(dir, "test-draw-waves");
    npage = 0; p_mix(); finish(dir, "test-rack-mix");
    npage = 0; p_psg2(); finish(dir, "test-psg-knobs-2");
    npage = 0; p_psg3(); finish(dir, "test-5b-sid");
    npage = 0; no_level = 1; p_meters(); finish(dir, "test-meters");
    npage = 0; no_level = 1; p_waltz(); finish(dir, "meters-waltz"); waltz_level();
    fclose(lf);
    return fail;
}
