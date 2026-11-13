/* SPDX-License-Identifier: GPL-3.0-only */
/* Loads each payload file given, packs it again, prints a line per file: name, status, pack length, hash of the pack, hash of every step (page, channel, step, note, vel, gate). Also the built-in Korobeiniki.
 * Build against two firmware trees and diff the output: an old project must come out the same. */
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
static uint32_t fm1_ms;
#include "proj_core.h"
static uint64_t fnv(const uint8_t *p, uint32_t n) { uint64_t h = 1469598103934665603ull; while (n--) { h ^= *p++; h *= 1099511628211ull; } return h; }
static void report(const char *name, const uint8_t *buf, uint32_t len)
{
    static uint8_t out[PROJ_BANK], dump[1 << 20];
    uint32_t p, c, s, n = 0, lp;
    if (proj_check(buf, len)) { printf("%s REJECT\n", name); return; }
    if (proj_unpack(buf, len)) { printf("%s UNPACK-FAIL\n", name); return; }
    lp = proj_pack(out);
    for (p = 0; p < page_n; p++) for (c = 0; c < chan_n; c++) for (s = 0; s < 16u; s++) {
        const step_t *x = STEP_R(p, c, s);
        if (x->note || x->vel || x->gate) { dump[n++] = (uint8_t)p; dump[n++] = (uint8_t)c; dump[n++] = (uint8_t)s; dump[n++] = x->note; dump[n++] = x->vel; dump[n++] = x->gate; }
    }
    printf("%s ok pack %u %016llx steps %016llx song %u pages %u chans %u bpm %u\n", name, lp, (unsigned long long)fnv(out, lp), (unsigned long long)fnv(dump, n), (unsigned)song_len, (unsigned)page_n, (unsigned)chan_n, (unsigned)seq_bpm);
}
int main(int argc, char **argv)
{
    static uint8_t buf[1 << 16], out[PROJ_BANK];
    int i;
    seq_tetris(); { uint32_t l = proj_pack(out); report("tetris", out, l); }
    for (i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "rb");
        size_t n;
        if (!f) continue;
        n = fread(buf, 1, sizeof buf, f); fclose(f);
        { const char *b = strrchr(argv[i], '/'); report(b ? b + 1 : argv[i], buf, (uint32_t)n); }
    }
    return 0;
}
