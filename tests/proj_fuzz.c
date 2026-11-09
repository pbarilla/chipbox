/* SPDX-License-Identifier: GPL-3.0-only */
/* The project format with FM patches: every FM kind with random patches packs and unpacks to the same bytes; old projects (the demo, version 3) load and become patches; random bytes never crash
 * proj_check / proj_unpack (run under the sanitizers).
 *   python3 tests/proj_fuzz.py          (builds and runs it) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS 44100u
#define PROJ_INST 17u
#define PROJ_EV 6u
#define PROJ_BANK 0x3000u
#define PROJ_HDR 16u
#include "engine_stubs.h"
#include "chips.c"
#include "demo2.h"
static uint32_t fm1_ms;
#include "proj_core.h"
int main(void)
{
    static const uint32_t K[] = {KD_OPM, KD_MD, KD_OPN_FM, KD_PC98_FM, KD_NEO_FM, KD_OPZ, KD_OPL3, KD_OPL, KD_OPL1, KD_OPLL, KD_VRC7};
    static uint8_t a[PROJ_BANK], b[PROJ_BANK], fz[400];
    uint32_t it, i, c, la, lb, fails = 0, ok_loads = 0;
    srand(99);
    /* 1: a project of random FM patches round-trips */
    for (it = 0; it < 200u; it++) {
        seq_clear();
        for (c = 0; c < 11u; c++) {
            uint8_t raw[64];
            chan_add(K[c]);
            for (i = 0; i < sizeof raw; i++) raw[i] = (uint8_t)rand();
            fm_unpack(K[c], &chan[c].fm, raw);
            chan[c].in.duck = (uint8_t)(rand() & 15); chan[c].in.vol = (uint8_t)(rand() & 15);
        }
        la = proj_pack(a);
        if (!la || proj_check(a, la)) { printf("pack/check failed\n"); fails++; continue; }
        if (proj_unpack(a, la)) { printf("unpack failed\n"); fails++; continue; }
        lb = proj_pack(b);
        if (la != lb || memcmp(a, b, la)) { printf("round trip differs (iteration %u)\n", it); fails++; }
    }
    /* 1b: every kind, every sound field (the new knobs live in wf wc wt duty pan sweep vib_spd): the values come back as they went in */
    for (it = 0; it < 400u; it++) {
        inst_t keep[16];
        seq_clear();
        for (c = 0; c < 16u; c++) {
            const uint32_t k = (uint32_t)rand() % KINDS;
            inst_t *x;
            chan_add(k);
            x = &chan[c].in;
            if (IS_FMK(k)) { memset(&keep[c], 0, sizeof keep[c]); continue; }
            x->wave = (uint8_t)(rand() & 15); x->duty = (uint8_t)rand(); x->wf = (uint8_t)(rand() & 15); x->wc = (uint8_t)(rand() & 15); x->wt = (uint8_t)(rand() & 15);
            if (k == KD_SID) { x->wave = (uint8_t)rand(); x->wf = (uint8_t)rand(); x->wc = (uint8_t)rand(); x->wt = (uint8_t)rand(); }
            x->atk = (uint8_t)(rand() & 15); x->dec = (uint8_t)(rand() & 15); x->sus = (uint8_t)(rand() & 15); x->rel = (uint8_t)(rand() & 15);
            x->vib_spd = (uint8_t)(rand() & 15); x->vib_dep = (uint8_t)(rand() & 15); x->sweep = (int8_t)(rand() % 129 - 64); x->pan = (uint8_t)(rand() & 15); x->vol = (uint8_t)(rand() & 15);
            x->arp = (uint8_t)(rand() % 9); x->arp_rate = (uint8_t)(rand() & 3); x->arp_dir = (uint8_t)(rand() % 3); x->duck = (uint8_t)(rand() & 15);
            keep[c] = *x;
        }
        la = proj_pack(a);
        if (!la || proj_check(a, la) || proj_unpack(a, la)) { printf("all kinds: pack / load failed (iteration %u)\n", it); fails++; continue; }
        for (c = 0; c < 16u; c++) if (!IS_FMK(chan[c].kind) && memcmp(&keep[c], &chan[c].in, sizeof keep[c])) { printf("all kinds: kind %u sound changed in the round trip (iteration %u)\n", chan[c].kind, it); fails++; break; }
    }
    /* 1c: drawn waves (DRAW) come back sample for sample, and a channel without one stays without */
    for (it = 0; it < 400u; it++) {
        uint8_t keepw[16][64], keepc[16], keepn[16];
        seq_clear();
        for (c = 0; c < 16u; c++) {
            static const uint32_t W[9] = {KD_GB_WAV, KD_PCE, KD_VB_WAV, KD_WS, KD_SCC, KD_N163, KD_FDS, KD_C30, KD_WSG};
            const uint32_t k = rand() & 1u ? W[rand() % 9u] : (uint32_t)rand() % KINDS;
            chan_add(k);
            if (wd_ok(k)) chan[c].in.duty = (uint8_t)(rand() % 7u);
            keepc[c] = 0; keepn[c] = 0;
            if (wd_ok(k) && (rand() & 1u)) {
                uint32_t i;
                keepn[c] = (uint8_t)wd_n(c); keepc[c] = 1;
                for (i = 0; i < 64u; i++) chan[c].user[i] = (uint8_t)(rand() % (wd_top(k) + 1u));
                memcpy(keepw[c], chan[c].user, 64); chan[c].wcus = 1;
            }
        }
        la = proj_pack(a);
        if (!la || proj_check(a, la) || proj_unpack(a, la)) { printf("drawn waves: pack / load failed (iteration %u)\n", it); fails++; continue; }
        for (c = 0; c < 16u; c++) {
            if (chan[c].wcus != keepc[c] || (keepc[c] && memcmp(keepw[c], chan[c].user, keepn[c]))) { printf("drawn waves: kind %u differs (iteration %u)\n", chan[c].kind, it); fails++; break; }
        }
    }
    /* 2: the demo (version 3, no patches) loads, and every FM channel got a patch inside its chip's limits */
    if (proj_unpack(DEMO2, sizeof DEMO2)) { printf("demo failed to load\n"); fails++; }
    else {
        uint32_t nfm = 0;
        for (c = 0; c < chan_n; c++) if (IS_FMK(chan[c].kind)) {
            const uint32_t fam = fm_fam(chan[c].kind), nop = FM_NOPS(fam);
            uint32_t j, f;
            nfm++;
            for (j = 0; j < nop; j++) for (f = 0; f < NOPF; f++) if (chan[c].fm.op[j][f] > FMXO[fam][f]) { printf("demo: kind %u op %u field %u = %u (max %u)\n", chan[c].kind, j, f, chan[c].fm.op[j][f], FMXO[fam][f]); fails++; }
            for (f = 0; f < NCHF; f++) if (chan[c].fm.ch[f] > FMXC[fam][f]) { printf("demo: channel field out of range\n"); fails++; }
        }
        printf("demo: %u channels, %u FM\n", chan_n, nfm);
    }
    /* 3: random bytes behind a plausible header */
    for (it = 0; it < 200000u; it++) {
        uint32_t n = 7u + (uint32_t)(rand() % 380);
        for (i = 0; i < n; i++) fz[i] = (uint8_t)(rand() % (it & 1u ? 256 : 4));
        fz[0] = (uint8_t)(2 + rand() % 4); fz[3] = (uint8_t)(1 + rand() % 3); fz[4] = (uint8_t)(rand() % 4);
        if (!proj_check(fz, n)) { ok_loads++; if (proj_unpack(fz, n)) { printf("check passed, unpack failed\n"); fails++; } else if (chan_n > MAXCH) fails++; }
    }
    printf("fuzz: %u random byte strings passed the check and loaded\n", ok_loads);
    printf(fails ? "FAIL (%u)\n" : "ok\n", fails);
    return fails != 0;
}
