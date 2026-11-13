/* SPDX-License-Identifier: GPL-3.0-only */
/* A meter and a length for each page: the block store, the clock's wrap at the page's length, the project format's meter byte (version 6).
 *   python3 tests/meter_seq.py          (builds and runs it under the sanitizers) */
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
static uint32_t fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)
static void clear_all(void) { seq_clear(); chan_add(KD_NES_PU); chan_add(KD_NES_TRI); chan_add(KD_DRUM0); }
int main(void)
{
    static uint8_t a[PROJ_BANK], b[PROJ_BANK];
    uint32_t i, p, t, s, la, lb;
    /* 1: the meters and their lengths */
    clear_all();
    CHECK(page_steps(0) == 16 && page_bars(0) == 1 && page_sig[0] == 0, "a new page is 4/4 x 1 bar");
    for (i = 0; i < 6u; i++) { page_sig[0] = (uint8_t)i; page_xb[0] = 0; CHECK(page_steps(0) == METER_STEPS[i], "meter %u: %u steps", i, page_steps(0)); CHECK(page_max_bars(0) * METER_STEPS[i] <= 64u, "max bars of meter %u", i); }
    page_sig[0] = 2; page_xb[0] = 4; CHECK(page_steps(0) == 60, "3/4 x 5 bars = 60, got %u", page_steps(0));
    page_sig[0] = 0; page_xb[0] = 0;
    /* 2: blocks: read where nothing is, write, read back, collect, count */
    CHECK(blk_free_n() == SEQ_NBLK, "all blocks free at the start");
    CHECK(STEP_R(3, 1, 40)->note == 0, "an empty step reads zero");
    CHECK(blk_free_n() == SEQ_NBLK, "reading takes no block");
    for (s = 0; s < SEQ_MAXLEN; s += 7u) { step_t *x = STEP_W(3, 1, s); x->note = (uint8_t)(40 + s); x->vel = 100; x->gate = 5; }
    for (s = 0; s < SEQ_MAXLEN; s += 7u) CHECK(STEP_R(3, 1, s)->note == 40 + s, "step %u reads back", s);
    CHECK(blk_free_n() == SEQ_NBLK - 4u, "64 steps are 4 blocks, got %u used", SEQ_NBLK - blk_free_n());
    STEP_W(5, 0, 2)->note = 0;                  /* a block made and left empty */
    blk_gc();
    CHECK(blk_free_n() == SEQ_NBLK - 4u, "the empty block was collected");
    CHECK(!page_empty(3) && page_empty(5), "page_empty sees the notes");
    /* 3: a page copy takes the blocks, the meter and the length */
    page_sig[3] = 3; page_xb[3] = 1;
    page_n = 4;
    { int d = seq_dup_page(3); CHECK(d == 4 && page_n == 5, "dup page"); CHECK(page_sig[d] == 3 && page_xb[d] == 1, "dup keeps the meter");
      for (s = 0; s < SEQ_MAXLEN; s += 7u) CHECK(STEP_R((uint32_t)d, 1, s)->note == 40 + s, "dup step %u", s);
      STEP_W((uint32_t)d, 1, 0)->note = 99; CHECK(STEP_R(3, 1, 0)->note == 40, "the copy is its own"); }
    /* 4: a channel removed: its blocks go, the ones above move down */
    STEP_W(0, 2, 20)->note = 77; STEP_W(0, 1, 33)->note = 66;
    { const uint32_t before = blk_free_n(); chan_remove(1); CHECK(blk_free_n() > before, "removing a channel frees its blocks");
      CHECK(STEP_R(0, 1, 20)->note == 77, "the track above moved down"); CHECK(STEP_R(0, 2, 20)->note == 0, "and the last one is empty"); }
    /* 5: no blocks left: writes are lost, nothing breaks */
    clear_all();
    for (p = 0; p < SEQ_PAGES; p++) for (t = 0; t < MAXCH; t++) for (s = 0; s < SEQ_MAXLEN; s += SEQ_LEN) STEP_W(p, t, s)->note = 1;
    CHECK(blk_free_n() == 0, "the store fills up to its budget (%u free)", blk_free_n());
    STEP_W(63, 15, 63)->note = 9;
    CHECK(STEP_R(63, 15, 63)->note == 0 || STEP_R(63, 15, 63)->note == 9, "a write with no block left does not crash");
    clear_all();
    /* 6: the clock wraps at the page's length: 3/4 (12), then 7/8 x 2 (28), then round again */
    page_n = 2; song_len = 2; song[0] = 0; song[1] = 1;
    page_sig[0] = 2; page_xb[0] = 0; page_sig[1] = 5; page_xb[1] = 1;
    seq_play(0, 0);
    {   uint32_t lens[4] = {0, 0, 0, 0}, pass = 0, cur = seq_step, run = 1, tk;
        uint8_t pgs[4] = {0, 0, 0, 0};
        pgs[0] = seq_pat;
        for (tk = 0; tk < 200u * SEQ_PPS && pass < 3u; tk++) {
            const uint32_t pp = seq_pat, ps = seq_step;
            seq_clock();
            if (seq_step != ps || seq_pat != pp) { if (seq_step == 0) { lens[pass] = ps + 1u; pgs[pass] = (uint8_t)pp; pass++; } }
        }
        (void)cur; (void)run;
        CHECK(lens[0] == 12 && pgs[0] == 0, "pass 1: page 0 is 12 steps (got %u on page %u)", lens[0], pgs[0]);
        CHECK(lens[1] == 28 && pgs[1] == 1, "pass 2: page 1 is 28 steps (got %u on page %u)", lens[1], pgs[1]);
        CHECK(lens[2] == 12 && pgs[2] == 0, "pass 3: page 0 again, 12 steps (got %u)", lens[2]);
    }
    /* 7: a change to the page that plays waits for the next wrap */
    seq_view = 0; page_sig[0] = 0; page_xb[0] = 0;            /* page 0, 16 steps, round and round */
    seq_play(0, 1);
    { uint32_t tk, steps = 0, first = 0, second = 0, passes = 0;
      for (tk = 0; tk < 100u * SEQ_PPS && passes < 2u; tk++) {
          const uint32_t ps = seq_step;
          if (tk == 5u * SEQ_PPS) { page_sig[0] = 1; page_xb[0] = 0; }          /* 2/4: 8 steps, turned mid-pass */
          seq_clock();
          if (seq_sub == 0) steps++;
          if (seq_step == 0 && ps != 0) { if (!passes) first = ps + 1u; else second = ps + 1u; passes++; }
      }
      CHECK(first == 16, "the pass that was playing keeps its 16 steps (got %u)", first);
      CHECK(second == 8, "the next pass is the new 8 (got %u)", second); (void)steps; }
    /* 8: the project format */
    clear_all();
    STEP_W(0, 0, 3)->note = 60; STEP_W(0, 0, 3)->vel = 100; STEP_W(0, 0, 3)->gate = 5;
    la = proj_pack(a);
    CHECK(la && a[0] == 5, "a project of 4/4 x 1 bar packs as version 5 (got %u)", a[0]);
    page_n = 3; song_len = 3; song[0] = 0; song[1] = 1; song[2] = 2;
    page_sig[1] = 2; page_xb[1] = 0; page_sig[2] = 5; page_xb[2] = 1;
    STEP_W(1, 1, 40)->note = 50; STEP_W(1, 1, 40)->vel = 100;           /* past a 3/4 page's end: kept */
    STEP_W(2, 0, 27)->note = 61; STEP_W(2, 0, 27)->vel = 100;
    la = proj_pack(a);
    CHECK(la && a[0] == 6, "a project with a meter packs as version 6 (got %u)", a[0]);
    CHECK(!proj_check(a, la), "and checks");
    CHECK(!proj_unpack(a, la), "and loads");
    CHECK(page_sig[1] == 2 && page_xb[1] == 0 && page_sig[2] == 5 && page_xb[2] == 1 && page_sig[0] == 0, "the meters came back");
    CHECK(STEP_R(1, 1, 40)->note == 50 && STEP_R(2, 0, 27)->note == 61, "the steps past 16 came back");
    lb = proj_pack(b);
    CHECK(la == lb && !memcmp(a, b, la), "the round trip packs the same bytes");
    { uint8_t bad[PROJ_BANK]; memcpy(bad, a, la); bad[la - 1] = 0x0F; CHECK(proj_check(bad, la) != 0, "a meter index past the list is refused");
      memcpy(bad, a, la); bad[la - 1] = (uint8_t)(5u | 7u << 4); CHECK(proj_check(bad, la) != 0, "more bars than 64 steps allow (8 bars of 7/8) is refused");
      memcpy(bad, a, la); CHECK(proj_check(bad, la - 1u) != 0, "a cut meter trailer is refused");
      memcpy(bad, a, la); bad[0] = 5; CHECK(!proj_check(bad, la) && !proj_unpack(bad, la) && page_sig[1] == 0, "version 5 ignores the trailer"); }
    /* 8b: a page's own tempo, and the glide into the next page's */
    clear_all();
    page_n = 3; song_len = 3; song[0] = 0; song[1] = 1; song[2] = 2; seq_bpm = 120;
    page_tmp[0] = 90; page_tmp[1] = 0; page_tmp[2] = 150; page_gl[1] = 1;
    seq_play(0, 0);
    {   uint32_t seen[3][64], cnt[3] = {0, 0, 0}, tk, pass = 0;
        uint32_t prev_sub = 0, prev_pat = 0;
        memset(seen, 0, sizeof seen);
        for (tk = 0; tk < 60u * SEQ_PPS && pass < 3u; tk++) {
            prev_sub = seq_sub; prev_pat = seq_pat;
            seq_clock();
            if (prev_sub == 0u && cnt[prev_pat] < 64u && prev_pat < 3u) seen[prev_pat][cnt[prev_pat]++] = seq_bpm_play;
            if (seq_pat != prev_pat) pass++;
        }
        CHECK(cnt[0] == 16 && seen[0][0] == 90 && seen[0][15] == 90, "page 0 plays at its own 90 (%u steps, %u .. %u)", cnt[0], seen[0][0], seen[0][15]);
        CHECK(cnt[1] == 16 && seen[1][0] == 120, "page 1 starts at the song's 120 (%u)", seen[1][0]);
        for (i = 0; i < 16u; i++) CHECK(seen[1][i] == 120u + 30u * i / 16u, "page 1 step %u glides: %u (want %u)", i, seen[1][i], 120u + 30u * i / 16u);
        CHECK(cnt[2] >= 1 && seen[2][0] == 150, "page 2 plays at 150 (%u)", seen[2][0]);
    }
    for (i = 1; i <= 3u; i++) {                              /* the shapes: straight, ease in (late), ease out (early) */
        uint32_t seen2[16], c2 = 0, tk2, pp;
        seq_bpm = 120; page_tmp[0] = 90; page_tmp[1] = 0; page_tmp[2] = 150; page_gl[1] = (uint8_t)i;
        seq_play(0, 0);
        for (tk2 = 0; tk2 < 80u * SEQ_PPS && c2 < 16u; tk2++) { const uint32_t was = seq_sub; pp = seq_pat; seq_clock(); if (was == 0u && pp == 1u) seen2[c2++] = seq_bpm_play; }
        CHECK(c2 == 16, "shape %u: 16 steps seen", i);
        for (tk2 = 0; tk2 < 16u; tk2++) { const uint32_t f = i == 2u ? (tk2 * 16u) * (tk2 * 16u) / 256u : i == 3u ? 256u - (256u - tk2 * 16u) * (256u - tk2 * 16u) / 256u : tk2 * 16u; CHECK(seen2[tk2] == 120u + 30u * f / 256u, "shape %u step %u: %u (want %u)", i, tk2, seen2[tk2], 120u + 30u * f / 256u); }
        if (i == 2u) CHECK(seen2[8] < 120u + 30u / 2u, "ease in is still near the start half way (%u)", seen2[8]);
        if (i == 3u) CHECK(seen2[8] > 120u + 30u / 2u, "ease out is past half way by half way (%u)", seen2[8]);
    }
    page_gl[1] = 1;
    seq_bpm = 100; page_gl[1] = 1;                         /* a page played round and round does not glide */
    seq_view = 1; seq_play(1, 1);
    { uint32_t tk, bad = 0; for (tk = 0; tk < 16u * SEQ_PPS; tk++) { const uint32_t was = seq_sub; seq_clock(); if (!was && seq_bpm_play != 100u) bad++; } CHECK(!bad, "a looped page keeps its tempo"); }
    clk_mode = 2; seq_bpm = 111; seq_play(0, 0);
    { uint32_t tk, bad = 0; for (tk = 0; tk < 40u * SEQ_PPS; tk++) { const uint32_t was = seq_sub; seq_clock(); if (!was && seq_bpm_play != 111u) bad++; } CHECK(!bad, "with the outside clock following, the page tempos are ignored"); }
    clk_mode = 0; seq_stop(); seq_bpm = 120;
    page_tmp[0] = 0; seq_view = 0;
    page_tempo_step(0, 5); CHECK(page_tmp[0] == 125, "KNOB 1 on a page gives it a tempo of its own (%u)", page_tmp[0]);
    page_tempo_step(0, -5); CHECK(page_tmp[0] == 0, "back at the song's tempo, it lets go (%u)", page_tmp[0]);
    seq_bpm = 238; page_tempo_step(0, 9); CHECK(page_tmp[0] == 240, "the tempo stops at 240 (%u)", page_tmp[0]); seq_bpm = 120; page_tmp[0] = 0;
    tempo_knob(3, 0); CHECK(seq_bpm == 123 && page_tmp[0] == 0, "KNOB 1 outside step entry moves the song's tempo"); page_tmp[0] = 100; tempo_knob(-4, 0); CHECK(page_tmp[0] == 96 && seq_bpm == 123, "... or the page's own if it has one"); page_tmp[0] = 0; seq_bpm = 120;
    page_tmp[2] = 140; page_gl[2] = 1; song_insert_blank(2); CHECK(page_tmp[page_n - 1u] == 140 && page_gl[page_n - 1u] == 0, "a new page keeps the tempo of the one before, not its glide");
    { int d = seq_dup_page(2); CHECK(d >= 0 && page_tmp[d] == 140 && page_gl[d] == 1, "a page copy keeps both"); }
    /* the format: a tempo makes it version 7, a glide alone version 6 */
    clear_all(); page_n = 2; song_len = 2; song[0] = 0; song[1] = 1; STEP_W(0, 0, 0)->note = 60; STEP_W(0, 0, 0)->vel = 100;
    page_gl[0] = 1;
    la = proj_pack(a); CHECK(la && a[0] == 6 && !proj_check(a, la), "a glide alone is version 6 (got %u)", a[0]);
    CHECK(!proj_unpack(a, la) && page_gl[0] == 1 && page_gl[1] == 0 && page_tmp[0] == 0, "and comes back");
    for (i = 0; i < 4u; i++) {                               /* every shape comes back; bit 7 alone (what version 6 wrote) is a straight glide */
        page_gl[0] = (uint8_t)i; page_tmp[1] = 0; la = proj_pack(a);
        CHECK(la && !proj_check(a, la) && !proj_unpack(a, la) && page_gl[0] == i, "glide shape %u round trips", i);
        if (i == 1u && la) CHECK(a[la - 2] == 0x80, "a straight glide is bit 7 (0x%02x)", a[la - 2]);
    }
    page_gl[0] = 1;
    page_tmp[1] = 77; la = proj_pack(a); CHECK(la && a[0] == 7 && !proj_check(a, la), "a page tempo is version 7 (got %u)", a[0]);
    CHECK(!proj_unpack(a, la) && page_tmp[1] == 77 && page_tmp[0] == 0 && page_gl[0] == 1, "and comes back");
    lb = proj_pack(b); CHECK(la == lb && !memcmp(a, b, la), "the version 7 round trip packs the same bytes");
    { uint8_t bad[PROJ_BANK]; memcpy(bad, a, la); bad[la - 1] = 30; CHECK(proj_check(bad, la) != 0, "a tempo of 30 is refused");
      memcpy(bad, a, la); bad[la - 1] = 250; CHECK(proj_check(bad, la) != 0, "a tempo of 250 is refused");
      memcpy(bad, a, la); CHECK(proj_check(bad, la - 1u) != 0, "a cut tempo block is refused"); }
    clear_all(); seq_bpm = 120;
    /* 9: a project that needs more blocks than there are is refused; one that just fits loads */
    for (i = 0; i < 2u; i++) {
        static uint8_t big[PROJ_BANK];
        const uint32_t pairs = i ? 300u : 250u;      /* 4 blocks each: 1200 (too many) or 1000 */
        uint32_t n = 0, ev = 0, q, st, evat;
        big[n++] = 5; big[n++] = 120; big[n++] = 0; big[n++] = 64; big[n++] = 16;
        for (q = 0; q < 16u; q++) { big[n++] = KD_NES_PU; memset(big + n, 0, PROJ_INST); n += PROJ_INST; big[n++] = 0; }
        evat = n; n += 2;
        for (q = 0; q < pairs; q++) for (st = 0; st < 64u; st += SEQ_LEN) { big[n++] = (uint8_t)(q / 16u); big[n++] = (uint8_t)(q % 16u); big[n++] = (uint8_t)st; big[n++] = 60; big[n++] = 100; big[n++] = 5; ev++; }
        big[evat] = (uint8_t)ev; big[evat + 1u] = (uint8_t)(ev >> 8);
        big[n++] = 1; big[n++] = 0;
        CHECK((proj_check(big, n) != 0) == (i == 1u), "%u blocks: %s", pairs * 4u, i ? "refused" : "accepted");
        if (!i) { CHECK(!proj_unpack(big, n), "1000 blocks load"); CHECK(blk_free_n() == SEQ_NBLK - 1000u, "and use 1000 blocks (%u free)", blk_free_n()); }
    }
    /* 10: random bytes in the trailer never crash a load */
    srand(7);
    for (i = 0; i < 3000u; i++) {
        uint8_t z[PROJ_BANK]; uint32_t k;
        memcpy(z, a, la);
        for (k = 0; k < 4u; k++) z[la - 1u - (uint32_t)(rand() % 3)] = (uint8_t)rand();
        if (!proj_check(z, la)) proj_unpack(z, la);
    }
    printf(fails ? "FAILED: %u\n" : "ok\n", fails);
    return fails != 0;
}
