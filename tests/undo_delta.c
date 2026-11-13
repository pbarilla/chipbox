/* SPDX-License-Identifier: GPL-3.0-only */
/* Undo as reverse deltas (proj.c "undo:"): random edits in groups, each group closed by a quiet moment; stepping back must give the exact project bytes of every earlier
 * state, and an open group goes back to how it began.
 *   python3 tests/undo_delta.py          (builds and runs it under the sanitizers) */
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
static uint32_t fm1_ms, proj_dirty, proj_t_edit, undo_open, undo_t;
static int add_aud_c = -1;
static uint8_t proj_buf[PROJ_BANK];
#include "proj_core.h"
#include "undo_core.h"
static uint32_t fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)
static uint8_t snap[64][PROJ_BANK]; static uint32_t slen[64];
static void edited(void) { undo_open = 1; undo_t = fm1_ms; }
static void edit_once(void)
{
    uint32_t r = (uint32_t)rand() % 16u, p = (uint32_t)rand() % page_n, c = chan_n ? (uint32_t)rand() % chan_n : 0u, s = (uint32_t)rand() % 64u;
    switch (r) {
    case 0: case 1: case 2: if (chan_n) { step_t *x = STEP_W(p, c, s); x->note = (uint8_t)(30 + rand() % 60); x->vel = 100; x->gate = 5; } break;
    case 3: if (chan_n) STEP_W(p, c, s)->note = 0; blk_gc(); break;
    case 4: if (chan_n) chan[c].in.vol = (uint8_t)(rand() & 15); break;
    case 5: if (chan_n) chan[c].in.wave = (uint8_t)(rand() & 15); break;
    case 6: if (chan_n > 1u && (rand() & 3) == 0) chan_remove(c); break;
    case 7: if (chan_n < 8u) chan_add((uint32_t)rand() % 12u); break;
    case 8: song_insert_blank(song_len - 1u); break;
    case 9: seq_dup_page(p); break;
    case 10: page_sig[p] = (uint8_t)(rand() % 6); page_xb[p] = 0; break;
    case 11: seq_bpm = 60u + (uint32_t)rand() % 180u; break;
    case 12: if (song_len > 1u && (rand() & 3) == 0) song_remove(song_len - 1u); break;
    case 14: page_tmp[p] = (rand() & 1) ? (uint8_t)(40 + rand() % 201) : 0; break;
    case 15: page_gl[p] = (uint8_t)(rand() & 3); break;
    default: if (chan_n) for (s = 0; s < 40u; s++) { step_t *x = STEP_W(p, c, s); x->note = (uint8_t)(40 + s); x->vel = 90; x->gate = 5; } break;
    }
    edited();
}
int main(void)
{
    static uint8_t cur[PROJ_BANK];
    uint32_t round, g, k, depth_total = 0, steps_checked = 0, bytes_total = 0, bytes_n = 0;
    srand(21);
    for (round = 0; round < 40u; round++) {
        uint32_t n = 0, max_undo;
        seq_clear(); undo_reset();
        for (k = 0; k < 4u; k++) chan_add((uint32_t)rand() % 12u);
        for (k = 0; k < 20u; k++) STEP_W(0, (uint32_t)rand() % 4u, (uint32_t)rand() % 16u)->note = 50;
        fm1_ms = 1000; undo_tick();                              /* the first base */
        slen[0] = proj_pack(snap[0]); n = 1;
        for (g = 0; g < 30u; g++) {
            const uint32_t ops = 1u + (uint32_t)rand() % 3u;
            for (k = 0; k < ops; k++) edit_once();
            fm1_ms += 700; undo_tick();                          /* the quiet moment: the group closes */
            slen[n % 64u] = proj_pack(snap[n % 64u]);
            if (slen[n % 64u] != slen[(n - 1u) % 64u] || memcmp(snap[n % 64u], snap[(n - 1u) % 64u], slen[n % 64u])) { n++; }
        }
        bytes_total += undo_used; bytes_n += undo_cnt;
        max_undo = undo_cnt;
        CHECK(max_undo >= 1u, "round %u: some steps were kept", round);
        /* an open group goes back to the base */
        edit_once(); edit_once();
        CHECK(!undo_go(), "round %u: undo of an open group", round);
        { uint32_t l = proj_pack(cur); CHECK(l == slen[(n - 1u) % 64u] && !memcmp(cur, snap[(n - 1u) % 64u], l), "round %u: an open group came back to how it began", round); }
        for (k = 1; k <= max_undo; k++) {
            const uint32_t want = (n - 1u - k) % 64u;
            uint32_t l;
            CHECK(!undo_go(), "round %u: undo %u", round, k);
            l = proj_pack(cur);
            CHECK(l == slen[want] && !memcmp(cur, snap[want], l), "round %u: step back %u gives the earlier project (len %u vs %u)", round, k, l, slen[want]);
            steps_checked++;
        }
        depth_total += max_undo;
    }
    printf("40 rounds, %u steps back checked, average depth %u, average %u bytes a step in the ring\n", steps_checked, depth_total / 40u, bytes_n ? bytes_total / bytes_n : 0u);
    printf(fails ? "FAILED: %u\n" : "ok\n", fails);
    return fails != 0;
}
