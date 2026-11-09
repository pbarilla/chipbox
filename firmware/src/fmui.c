/* SPDX-License-Identifier: GPL-3.0-only */
/* The FM chips' knobs: SEL picks what the knobs edit (the channel, then each operator), ALGORITHM turns that item's pages. Included by chipbox.c before knob_set.
 * A page is four fields; a field is an operator field (0..NOPF - 1), a channel field (FC(CF_*)) or one of the tracker's own (VOL, VIB, the arp). */
enum { FI_VOL = 100, FI_VIB, FI_ARP0, FI_ARP1, FI_ARP2, FI_NONE = 255 };
#define FC(x) (32u + (x))
#define NN FI_NONE
#define FMROW(a, b, c, d) {a, b, c, d}
#define FMARP FMROW(FI_ARP0, FI_ARP1, FI_ARP2, FI_VIB)
static const uint8_t FMCH[FAMS][4][4] = {
    [FAM_OPM] = {FMROW(FC(CF_ALG), FC(CF_FB), FC(CF_PAN), FI_VOL), FMROW(FC(CF_LFS), FC(CF_LFW), FC(CF_PMD), FC(CF_AMD)), FMROW(FC(CF_PMS), FC(CF_AMS), NN, NN), FMARP},
    [FAM_OPN] = {FMROW(FC(CF_ALG), FC(CF_FB), FC(CF_PAN), FI_VOL), FMROW(FC(CF_LFS), FC(CF_PMS), FC(CF_AMS), NN), FMARP},
    [FAM_OPN3] = {FMROW(FC(CF_ALG), FC(CF_FB), NN, FI_VOL), FMARP},
    [FAM_OPZ] = {FMROW(FC(CF_ALG), FC(CF_FB), NN, FI_VOL), FMROW(FC(CF_LFS), FC(CF_LFW), FC(CF_PMD), FC(CF_AMD)), FMROW(FC(CF_PMS), FC(CF_AMS), NN, NN), FMARP},
    [FAM_OPL3] = {FMROW(FC(CF_ALG), FC(CF_FB), FC(CF_PAN), FI_VOL), FMROW(FC(CF_DAM), FC(CF_DVB), NN, NN), FMARP},
    [FAM_OPL2] = {FMROW(FC(CF_ALG), FC(CF_FB), NN, FI_VOL), FMROW(FC(CF_DAM), FC(CF_DVB), NN, NN), FMARP},
    [FAM_OPL1] = {FMROW(FC(CF_ALG), FC(CF_FB), NN, FI_VOL), FMROW(FC(CF_DAM), FC(CF_DVB), NN, NN), FMARP},
    [FAM_OPLL] = {FMROW(FC(CF_INST), FC(CF_FB), FC(CF_PED), FI_VOL), FMARP},
};
static const uint8_t FMOP[FAMS][4][4] = {
    [FAM_OPM] = {FMROW(OF_TL, OF_MUL, OF_DT1, OF_DT2), FMROW(OF_AR, OF_D1R, OF_D1L, OF_D2R), FMROW(OF_RR, OF_KS, OF_AM, NN)},
    [FAM_OPN] = {FMROW(OF_TL, OF_MUL, OF_DT1, OF_KS), FMROW(OF_AR, OF_D1R, OF_D1L, OF_D2R), FMROW(OF_RR, OF_AM, NN, NN)},
    [FAM_OPN3] = {FMROW(OF_TL, OF_MUL, OF_DT1, OF_KS), FMROW(OF_AR, OF_D1R, OF_D1L, OF_D2R), FMROW(OF_RR, NN, NN, NN)},
    [FAM_OPZ] = {FMROW(OF_TL, OF_MUL, OF_FINE, OF_DT1), FMROW(OF_AR, OF_D1R, OF_D1L, OF_D2R), FMROW(OF_RR, OF_KS, OF_AM, OF_WAVE), FMROW(OF_DT2, OF_SHIFT, OF_FIX, NN)},
    [FAM_OPL3] = {FMROW(OF_TL, OF_MUL, OF_KSL, OF_WAVE), FMROW(OF_AR, OF_D1R, OF_D1L, OF_RR), FMROW(OF_EGT, OF_KSR, OF_AM, OF_VIB)},
    [FAM_OPL2] = {FMROW(OF_TL, OF_MUL, OF_KSL, OF_WAVE), FMROW(OF_AR, OF_D1R, OF_D1L, OF_RR), FMROW(OF_EGT, OF_KSR, OF_AM, OF_VIB)},
    [FAM_OPL1] = {FMROW(OF_TL, OF_MUL, OF_KSL, NN), FMROW(OF_AR, OF_D1R, OF_D1L, OF_RR), FMROW(OF_EGT, OF_KSR, OF_AM, OF_VIB)},
    [FAM_OPLL] = {FMROW(OF_TL, OF_KSL, OF_MUL, OF_WAVE), FMROW(OF_AR, OF_D1R, OF_D1L, OF_RR), FMROW(OF_AM, OF_VIB, OF_EGT, OF_KSR)},
};
static const uint8_t FMOPC[4] = FMROW(OF_MUL, OF_KSL, OF_WAVE, NN);      /* the OPLL's carrier: no level of its own */
static const uint8_t FMNPC[FAMS] = {0, 4, 3, 2, 4, 3, 3, 3, 2};       /* how many pages the channel and each operator have */
static const uint8_t FMNPO[FAMS] = {0, 3, 3, 3, 4, 3, 3, 3, 3};
static const uint8_t FMNONE[4] = {NN, NN, NN, NN};
static const uint8_t *fm_row(uint32_t c, uint32_t foc, uint32_t page)
{
    const uint32_t fam = fm_fam(chan[c].kind);
    if (page >= (foc ? FMNPO[fam] : FMNPC[fam])) return FMNONE;
    if (!foc) return FMCH[fam][page];
    if (fam == FAM_OPLL && foc == 2u && page == 0u) return FMOPC;
    return FMOP[fam][page];
}
static uint32_t fm_pages(uint32_t c, uint32_t foc) { const uint32_t fam = fm_fam(chan[c].kind); return foc ? FMNPO[fam] : FMNPC[fam]; }
static uint32_t fm_nfoc(uint32_t c) { return 1u + FM_NOPS(fm_fam(chan[c].kind)); }
static void fm_fix(uint32_t c)                  /* the screen's focus must exist on this channel */
{
    if (c >= chan_n || !IS_FMK(chan[c].kind)) { fm_foc = 0; return; }
    if (fm_foc >= fm_nfoc(c)) fm_foc = 0;
}
static uint8_t *fm_fld(uint32_t c, uint32_t foc, uint32_t id)
{
    chan_t *ch = &chan[c];
    if (id < NOPF) return foc && foc <= 4u ? &ch->fm.op[foc - 1u][id] : 0;
    if (id >= 32u && id < 32u + NCHF) return &ch->fm.ch[id - 32u];
    if (id == FI_VOL) return &ch->in.vol;
    if (id == FI_VIB) return &ch->in.vib_dep;
    if (id == FI_ARP0) return &ch->in.arp;
    if (id == FI_ARP1) return &ch->in.arp_rate;
    if (id == FI_ARP2) return &ch->in.arp_dir;
    return 0;
}
static uint32_t fm_fmax(uint32_t fam, uint32_t id)
{
    if (id < NOPF) return FMXO[fam][id];
    if (id >= 32u && id < 32u + NCHF) return FMXC[fam][id - 32u];
    return id == FI_ARP0 ? 8u : id == FI_ARP1 ? 3u : id == FI_ARP2 ? 2u : 15u;
}
static const char *fm_label(uint32_t fam, uint32_t id)
{
    const uint32_t opl = fam >= FAM_OPL3;
    switch (id) {
    case OF_TL: return "TL";
    case OF_MUL: return opl ? "MULT" : "MUL";
    case OF_DT1: return "DT1";
    case OF_DT2: return "DT2";
    case OF_KS: return "KS";
    case OF_AR: return "AR";
    case OF_D1R: return opl ? "DR" : "D1R";
    case OF_D1L: return opl ? "SL" : "D1L";
    case OF_D2R: return "D2R";
    case OF_RR: return "RR";
    case OF_AM: return opl ? "TREM" : "AM";
    case OF_WAVE: return "WAVE";
    case OF_FINE: return "FINE";
    case OF_SHIFT: return "SHIFT";
    case OF_FIX: return "FIX";
    case OF_KSL: return "KSL";
    case OF_EGT: return "HOLD";
    case OF_KSR: return "KSR";
    case OF_VIB: return "VIB";
    case FC(CF_ALG): return fam == FAM_OPL3 ? "CONN" : opl ? "CNT" : "ALGO";
    case FC(CF_FB): return "FB";
    case FC(CF_PAN): return "PAN";
    case FC(CF_PMS): return "PMS";
    case FC(CF_AMS): return "AMS";
    case FC(CF_LFS): return fam == FAM_OPN ? "LFO" : "SPEED";
    case FC(CF_LFW): return "WAVE";
    case FC(CF_PMD): return "PMD";
    case FC(CF_AMD): return "AMD";
    case FC(CF_DAM): return "TREMD";
    case FC(CF_DVB): return "VIBD";
    case FC(CF_INST): return "INST";
    case FC(CF_PED): return "PEDAL";
    case FI_VOL: return "VOL";
    case FI_VIB: return "VIB";
    case FI_ARP0: return "SHAPE";
    case FI_ARP1: return "RATE";
    case FI_ARP2: return "DIR";
    default: return "";
    }
}
static void fm_knob(uint32_t c, uint32_t page, uint32_t k, int32_t d)
{
    const uint32_t fam = fm_fam(chan[c].kind), id = fm_row(c, fm_foc, page)[k];
    uint8_t *v;
    int32_t mx, nv, step = 1;
    if (id == NN || !(v = fm_fld(c, fm_foc, id))) return;
    mx = (int32_t)fm_fmax(fam, id);
    if (!mx) return;
    if (mx >= 200) step = 4; else if (mx >= 100 && id != FI_VOL) step = 2;       /* the wide ones (speed, the LFO depths) take bigger steps */
    nv = (int32_t)*v + d * step;
    nv = nv < 0 ? 0 : nv > mx ? mx : nv;
    if (id == FC(CF_INST)) {                    /* the OPLL / VRC7: an instrument number copies that ROM instrument's bytes into the channel; 0 keeps what is there (USER) */
        if (nv > 0 && (uint32_t)nv != *v) fm_opll_load(&chan[c].fm, chan[c].kind, (uint32_t)nv - 1u);
        else if (!nv && *v) chan[c].fm.ch[CF_EDIT] = 1;
        *v = (uint8_t)nv;
        return;
    }
    *v = (uint8_t)nv;
    if (fam == FAM_OPLL && id != FI_VOL && id != FI_VIB && id < FI_ARP0) chan[c].fm.ch[CF_EDIT] = 1;      /* the instrument is no longer the ROM's */
}
