/* SPDX-License-Identifier: GPL-3.0-only */
/* The sequencer, included by chips.c: a page is 16 steps of 1/16 note for every channel, played by a clock in the audio ISR.
 *   a song is a list of slots, each one a page (1 1 1 1 2 3 1 1 ...); PLAY goes down the list and round again, a held PLAY loops one page
 *   a track is a channel (the same number); a step holds a note, its velocity and its length in clock ticks
 *   the clock ticks 24 times a beat (6 a step) and each tick lands on its own sample, not on the 60 Hz tick
 *   an arp is a channel's setting (chips.c), not a step's
 * The main loop edits the arrays and calls seq_play / seq_stop; one writer on each side, plain bytes. */
#define SEQ_LEN 16u                             /* steps in a block, and on a screen */
#define SEQ_MAXLEN 64u                          /* the most steps in a page */
#define SEQ_PAGES 64u
#define SEQ_PPS 6u                              /* clock ticks per step */
#define SEQ_NBLK 1024u                          /* blocks: the room the old 64 pages x 16 tracks had */

typedef struct { uint8_t note, vel, gate, spare; } step_t;      /* note 0: nothing; gate in ticks */
#ifndef POOL
#define POOL __attribute__((section(".pool")))        /* big arrays live in the pool (a native test build defines POOL as nothing) */
#endif
/* The steps live in blocks of 16: the block of (page, track, step / 16) is there only when it has something in it. STEP_R reads (an empty step where there is no block),
 * STEP_W writes (it makes the block; with none left it hands back a scratch step, the write is lost). The audio ISR only reads; blk_gc (main loop) lets empty blocks go. */
static step_t blk[SEQ_NBLK][SEQ_LEN] POOL;
static uint16_t bmap[SEQ_PAGES][MAXCH][SEQ_MAXLEN / SEQ_LEN];      /* block + 1; 0: none */
static uint8_t blk_used[SEQ_NBLK / 8u];
static step_t step_zero, step_junk;
static uint32_t blk_hint;
static const step_t *step_r(uint32_t p, uint32_t t, uint32_t s)
{
    const uint32_t b = bmap[p][t][(s / SEQ_LEN) % (SEQ_MAXLEN / SEQ_LEN)];
    return b ? &blk[b - 1u][s % SEQ_LEN] : &step_zero;
}
static uint32_t blk_new(void)                   /* a free block, cleared: its number + 1, or 0 */
{
    uint32_t i, n;
    for (n = 0; n < SEQ_NBLK; n++) {
        i = (blk_hint + n) % SEQ_NBLK;
        if (!(blk_used[i >> 3] >> (i & 7u) & 1u)) { memset(blk[i], 0, sizeof blk[i]); blk_used[i >> 3] |= (uint8_t)(1u << (i & 7u)); blk_hint = i + 1u; return i + 1u; }
    }
    return 0;
}
static step_t *step_w(uint32_t p, uint32_t t, uint32_t s)
{
    uint16_t *m = &bmap[p][t][(s / SEQ_LEN) % (SEQ_MAXLEN / SEQ_LEN)];
    if (!*m) { const uint32_t b = blk_new(); if (!b) { memset(&step_junk, 0, sizeof step_junk); return &step_junk; } *m = (uint16_t)b; }
    return &blk[*m - 1u][s % SEQ_LEN];
}
#define STEP_R(p, t, s) step_r((p), (t), (s))
#define STEP_W(p, t, s) step_w((p), (t), (s))
static void blk_free(uint32_t p, uint32_t t, uint32_t b)
{
    const uint32_t i = bmap[p][t][b] - 1u;
    bmap[p][t][b] = 0;
    blk_used[i >> 3] &= (uint8_t)~(1u << (i & 7u));
}
static uint32_t blk_empty(uint32_t i)
{
    uint32_t k;
    for (k = 0; k < SEQ_LEN; k++) if (blk[i][k].note || blk[i][k].vel || blk[i][k].gate) return 0;
    return 1;
}
static void blk_gc(void)                        /* blocks with nothing in them go back (main loop only) */
{
    uint32_t p, t, b;
    for (p = 0; p < SEQ_PAGES; p++) for (t = 0; t < MAXCH; t++) for (b = 0; b < SEQ_MAXLEN / SEQ_LEN; b++)
        if (bmap[p][t][b] && blk_empty(bmap[p][t][b] - 1u)) blk_free(p, t, b);
}
static uint32_t blk_free_n(void) { uint32_t i, n = 0; for (i = 0; i < SEQ_NBLK; i++) n += !(blk_used[i >> 3] >> (i & 7u) & 1u); return n; }
/* a page's meter and length: the signature (index in METER, 0 = 4/4, so zero is the default) and the bars less one; steps = a bar's steps x bars, at most SEQ_MAXLEN */
static const uint8_t METER_STEPS[6] = {16, 8, 12, 20, 12, 14};
static const char *const METER_NAME[6] = {"4/4", "2/4", "3/4", "5/4", "6/8", "7/8"};
static uint8_t page_sig[SEQ_PAGES], page_xb[SEQ_PAGES];
static uint32_t page_bar_steps(uint32_t p) { return METER_STEPS[page_sig[p] % 6u]; }
static uint32_t page_bars(uint32_t p) { return 1u + page_xb[p]; }
static uint32_t page_steps(uint32_t p) { const uint32_t n = page_bar_steps(p) * page_bars(p); return n > SEQ_MAXLEN ? SEQ_MAXLEN : n; }
static uint32_t page_max_bars(uint32_t p) { return SEQ_MAXLEN / page_bar_steps(p); }
static const uint32_t METER_GRP[6] = {0x1111u, 0x11u, 0x111u, 69905u /* 5/4: 1 5 9 13 17 */, 0x41u, 0x111u};       /* the steps of a bar where a beat group starts: 4/4 1 5 9 13, 6/8 1 7, 7/8 2 + 2 + 3 eighths */
static const uint8_t METER_UI[6] = {1, 2, 0, 3, 4, 5};      /* the knob's order, 2/4 3/4 4/4 5/4 6/8 7/8, as indexes in METER */
static uint8_t page_tmp[SEQ_PAGES], page_gl[SEQ_PAGES];     /* a page's own tempo (40..240, 0: the song's), and its glide into the next page's tempo: 0 none, 1 straight, 2 ease in (the change comes late), 3 ease out (it comes early) */
static uint32_t glide_f(uint32_t shape, uint32_t step, uint32_t len)     /* how far along a glide, 0..256, at a step */
{
    const uint32_t x = step * 256u / (len ? len : 1u);
    return shape == 2u ? x * x / 256u : shape == 3u ? 256u - (256u - x) * (256u - x) / 256u : x;
}
static uint32_t vwin;                           /* the screen of 16 steps the rack and the step view show of a longer page */
static void page_meter_step(uint32_t p, int32_t d)         /* KNOB 3: the next meter in the knob's order, stopping at the ends */
{
    int32_t u = 0, k;
    for (k = 0; k < 6; k++) if (METER_UI[k] == page_sig[p]) u = k;
    u += d; u = u < 0 ? 0 : u > 5 ? 5 : u;
    page_sig[p] = METER_UI[u];
    if (page_xb[p] >= page_max_bars(p)) page_xb[p] = (uint8_t)(page_max_bars(p) - 1u);
}
static void page_bars_step(uint32_t p, int32_t d)          /* KNOB 4: the bars, one to as many as 64 steps allow */
{
    int32_t v = (int32_t)page_xb[p] + d, mx = (int32_t)page_max_bars(p) - 1;
    page_xb[p] = (uint8_t)(v < 0 ? 0 : v > mx ? mx : v);
}
static uint32_t page_n = 1;                     /* pages in use */
static volatile uint32_t seq_bpm = 120;       /* the song's tempo */
static volatile uint32_t seq_bpm_play = 120;  /* the tempo the clock runs at: the playing page's own, or part way through a glide */
static uint32_t page_bpm(uint32_t p) { return page_tmp[p] ? page_tmp[p] : seq_bpm; }
#define SONG_MAX 128
static uint8_t song[SONG_MAX];                  /* the page of each slot */
static uint32_t song_len = 1;
static volatile int32_t fx_speed = 256;        /* the tape pad (fx.c): 256 normal, 512 double, 128 half, 0 stopped; pitch and level follow */
static volatile uint8_t fx_rpt;                 /* beat repeat (a punch-in effect): ticks between repeats, 0 off */
static uint8_t rpt_step, rpt_page, rpt_cnt;     /* ... of this step, on this page */
static volatile int8_t seq_rate;                /* the tape dial: -3..3, 0 pause; 1 half speed, 2 normal, 3 double; negative runs backwards */
static volatile uint8_t seq_hold, seq_seeked;   /* seq_hold: paused (the clock stands still); seq_seeked: the playhead was moved, PLAY goes on from there */
static volatile uint32_t seq_ticks;             /* clock ticks since play: a recorded note's length */
static volatile uint8_t seq_run, seq_pat, seq_next = 255, seq_step, seq_sub, seq_slot, seq_loop, seq_follow, seq_len = 16;     /* seq_loop: one page round and round; seq_follow: the rack shows the page that plays; seq_len: the steps of the page that plays, read again at each wrap (a change to a playing page waits for the next one) */
static uint8_t seq_view;                        /* the page the screen shows and the keys write; the player follows it at the end of the loop */
static uint32_t seq_left, seq_frac;             /* samples to the next tick, and the fraction a tick keeps */
/* MIDI clock (Menu > MIDI Clock; the settings byte `unlocked`): 0 off, 1 send, 2 follow. Our ticks are the MIDI clock's own, 24 a quarter note and 6 a step.
 * Send: each tick in the audio ISR is a clock byte; play and stop send start and stop. Follow: the main loop counts the outside ticks (ext_in) and the audio
 * ISR plays one tick for each, so the tempo is theirs; no tick for 0.4 s and our own clock is back. */
static volatile uint8_t clk_mode, ext_on, ext_reset;
static uint8_t bt_side, bt_patsync, bt_fxsync, bt_key, bt_scale, bt_last = 0xFFu;      /* Bluetooth sync, as a follower: the sidechain; following the leader's pattern (menu > Bluetooth) */
static volatile uint32_t ext_in;
static uint32_t ext_seen, ext_ms, ext_n, ext_t0;
static struct { uint8_t on, gate; } tr[MAXCH];

static uint32_t seq_tempo_at(uint32_t p, uint32_t slot, uint32_t step, uint32_t len)     /* the tempo at a step of a page: its own, gliding into the next slot's page's when it is set to */
{
    uint32_t a = page_bpm(p), b;
    if (clk_mode == 2u || brothers_mode == 2u) return seq_bpm;                    /* an outside clock decides */
    if (!page_gl[p] || seq_loop || slot + 1u >= song_len) return a;               /* (a page played round and round, or the last slot, does not glide) */
    b = page_bpm(song[slot + 1u] < page_n ? song[slot + 1u] : 0u);
    return (uint32_t)((int32_t)a + ((int32_t)b - (int32_t)a) * (int32_t)glide_f(page_gl[p], step, len) / 256);
}
static int32_t glide_delta(uint32_t p)          /* what the glide of a page adds by its end, for the arrow: 0 when it has none to go to */
{
    uint32_t i, at = song_len;
    if (!page_gl[p]) return 0;
    if (seq_run && seq_pat == p && !seq_loop) at = seq_slot;
    else for (i = 0; i < song_len; i++) if (song[i] == p) { at = i; break; }
    if (at + 1u >= song_len) return 0;
    return (int32_t)page_bpm(song[at + 1u] < page_n ? song[at + 1u] : 0u) - (int32_t)page_bpm(p);
}
static uint32_t tempo_shown(void)               /* the tempo for the top bar: live while the page on show plays, else what the page would play at */
{
    return seq_run && !seq_hold && seq_pat == seq_view ? seq_bpm_play : page_bpm(seq_view);
}
static void page_tempo_step(uint32_t p, int32_t d)         /* KNOB 1 on a page: its own tempo; back at the song's it lets go */
{
    int32_t v = (int32_t)page_bpm(p) + d;
    v = v < 40 ? 40 : v > 240 ? 240 : v;
    page_tmp[p] = (uint32_t)v == seq_bpm ? 0u : (uint8_t)v;
}
static void tempo_knob(int32_t d, uint32_t force_page)     /* KNOB 1: the tempo you see (a page's own if it has one, else the song's); in step entry always the page's */
{
    if (force_page || page_tmp[seq_view]) page_tempo_step(seq_view, d);
    else { int32_t v = (int32_t)seq_bpm + d; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); }
}
static uint32_t seq_period(void)                /* samples in one tick at the tempo, the remainder carried so the tempo is exact */
{
    uint32_t bpm = seq_bpm_play < 40u ? 40u : seq_bpm_play > 240u ? 240u : seq_bpm_play;
    uint32_t q = ((FS * 5u / 2u) << 8) / bpm + seq_frac;
    seq_frac = q & 255u;
    q >>= 8;
    return seq_rate == 1 || seq_rate == -1 ? q * 2u : seq_rate == 3 || seq_rate == -3 ? (q > 1u ? q / 2u : 1u) : q;     /* half speed, normal, double */
}
/* a drum channel's step holds 11 drums and an accent for each: note = hits 0..7, gate = accents 0..7, vel = hits 8..10 (bits 0..2) and accents 8..10 (bits 3..5) */
static uint32_t dstep_hits(const step_t *s) { return s->note | (uint32_t)(s->vel & 7u) << 8; }
static uint32_t dstep_acc(const step_t *s) { return s->gate | (uint32_t)(s->vel >> 3 & 7u) << 8; }
static void dstep_set(step_t *s, uint32_t hits, uint32_t acc)
{
    s->note = (uint8_t)hits; s->gate = (uint8_t)acc; s->vel = (uint8_t)((hits >> 8 & 7u) | (acc >> 8 & 7u) << 3);
}
static void seq_drums_fire(uint32_t t, const step_t *s)     /* a drum channel's step: every drum in it, at once; an accent is louder */
{
    uint32_t l, h = dstep_hits(s), a = dstep_acc(s);
    if (h & 1u) duck_lvl = 256u;                /* a kick: the sidechain */
    for (l = 0; l < 11u; l++) if (h >> l & 1u) ev_do(EV_ON, t, DR_LANE_TYPE[l], a >> l & 1u ? 127u : 88u);
}
static void seq_clock(void)                     /* one tick, in the audio ISR */
{
    uint32_t t;
    seq_ticks++;
    for (t = 0; t < chan_n; t++)                /* what is already sounding: its length */
        if (tr[t].on && tr[t].gate && !--tr[t].gate) { ev_do(EV_OFF, t, 0, 0); tr[t].on = 0; }
    if (fx_rpt) {                               /* beat repeat: the step that was held sounds again and again */
        if (!rpt_cnt) for (t = 0; t < chan_n; t++) {
            const step_t *s = STEP_R(rpt_page, t, rpt_step);
            if (IS_DRUM(chan[t].kind)) { if (dstep_hits(s)) seq_drums_fire(t, s); continue; }
            if (!s->note) continue;
            tr[t].on = 1; tr[t].gate = s->gate < fx_rpt ? s->gate : (uint8_t)(fx_rpt > 1u ? fx_rpt - 1u : 1u);
            ev_do(EV_ON, t, bt_scale ? brothers_snap(s->note, bt_scale) : s->note, s->vel);
        }
        if (++rpt_cnt >= fx_rpt) rpt_cnt = 0;
    }
    if (!seq_sub) seq_bpm_play = seq_tempo_at(seq_pat, seq_slot, seq_step, seq_len);     /* the tempo this step plays at */
    if (!seq_sub && !fx_rpt) {                  /* a new step */
        for (t = 0; t < chan_n; t++) {
            const step_t *s = STEP_R(seq_pat, t, seq_step);
            if (IS_DRUM(chan[t].kind)) { if (dstep_hits(s)) seq_drums_fire(t, s); continue; }
            if (!s->note) continue;
            tr[t].on = 1; tr[t].gate = s->gate;
            ev_do(EV_ON, t, bt_scale ? brothers_snap(s->note, bt_scale) : s->note, s->vel);
        }
    }
    if (++seq_sub >= SEQ_PPS) {
        seq_sub = 0;
        if (++seq_step >= seq_len) {
            seq_step = 0;
            if (seq_loop == 2u) {                   /* a page played once (the song screen's preview): stop, let the notes ring out */
                seq_run = 0;
                for (t = 0; t < chan_n; t++) { ev_do(EV_OFF, t, 0, 0); tr[t].on = 0; }
            }
            else if (seq_loop) { if (seq_next < page_n) { seq_pat = seq_next; seq_next = 255; } }
            else {                                  /* the next slot of the song, and round again at the end */
                if (++seq_slot >= song_len) seq_slot = 0;
                seq_pat = song[seq_slot] < page_n ? song[seq_slot] : 0;
                if (seq_follow) seq_view = seq_pat;
            }
            seq_len = (uint8_t)page_steps(seq_pat);         /* the page that comes (or the same one again): its length now */
        }
    }
}

static void seq_back(void)                      /* one tick backwards: crossing into a step from its end plays it */
{
    uint32_t t;
    seq_ticks++;
    for (t = 0; t < chan_n; t++)
        if (tr[t].on && tr[t].gate && !--tr[t].gate) { ev_do(EV_OFF, t, 0, 0); tr[t].on = 0; }
    if (seq_sub) { seq_sub--; return; }
    seq_sub = SEQ_PPS - 1u;
    if (seq_step) seq_step--;
    else {
        if (!seq_loop) {                        /* the song: the slot before */
            seq_slot = seq_slot ? seq_slot - 1u : (uint8_t)(song_len - 1u);
            seq_pat = song[seq_slot] < page_n ? song[seq_slot] : 0;
            if (seq_follow) seq_view = seq_pat;
        }
        seq_len = (uint8_t)page_steps(seq_pat);
        seq_step = (uint8_t)(seq_len - 1u);
    }
    seq_bpm_play = seq_tempo_at(seq_pat, seq_slot, seq_step, seq_len);
    if (fx_rpt) return;
    for (t = 0; t < chan_n; t++) {
        const step_t *s = STEP_R(seq_pat, t, seq_step);
        if (!s->note) continue;
        tr[t].on = 1; tr[t].gate = s->gate;
        ev_do(EV_ON, t, s->note, s->vel);
    }
}
static uint32_t seq_off;                         /* the sample of this render the tick is on (chips.c sets it): the brothers' leader timestamps by it */
static void seq_tick(void) { if (clk_mode == 1u && seq_rate > 0) midi_isr_send(0xF8u, 0, 0); if (brothers_mode == 1u && seq_rate > 0) brothers_lead_tick(seq_off, 60000000u / (seq_bpm_play < 40u ? 40u : seq_bpm_play > 240u ? 240u : seq_bpm_play) * (seq_rate == 1 ? 2u : 1u) / (seq_rate == 3 ? 2u : 1u)); if (seq_rate < 0) seq_back(); else seq_clock(); }

/* ---- the main loop's side */
static void seq_play(uint32_t slot, uint32_t loop)      /* the song from a slot; loop 1: the page on show round and round; loop 2: that page once */
{
    memset(tr, 0, sizeof tr);
    if (!(seq_seeked && loop == 1u)) seq_step = seq_sub = 0;       /* a page you seeked in goes on from where you left the playhead */
    seq_seeked = 0; seq_left = seq_frac = 0; seq_rate = 2; seq_hold = 0;
    seq_next = 255; seq_loop = (uint8_t)loop;
    if (loop) { seq_pat = seq_view; seq_follow = 0; }
    else {
        seq_slot = (uint8_t)(slot < song_len ? slot : 0u);
        seq_pat = song[seq_slot] < page_n ? song[seq_slot] : 0;
        seq_view = seq_pat; seq_follow = 1;
    }
    seq_len = (uint8_t)page_steps(seq_pat);
    seq_bpm_play = seq_tempo_at(seq_pat, seq_slot, seq_step, seq_len);
    ext_reset = 1;                                  /* an outside clock: the next tick is the first step */
    if (clk_mode == 1u) midi_send(0xFAu, 0, 0);
    if (brothers_mode == 1u) brothers_lead_run(1);
    seq_run = 1;
}
static void seq_stop(void)
{
    uint32_t t, was = seq_run;
    seq_run = 0; seq_rate = 0; seq_hold = 0;
    if (was && clk_mode == 1u) midi_send(0xFCu, 0, 0);
    if (was && brothers_mode == 1u) brothers_lead_run(0);
    for (t = 0; t < chan_n; t++) ev_push(EV_OFF, t, 0, 0);
}
static void seq_shuttle(int32_t d)              /* SELECT turned: the tape dial moves, from a stop it starts the tape */
{
    int32_t r = (int32_t)seq_rate + d;
    r = r < -3 ? -3 : r > 3 ? 3 : r;
    seq_seeked = 1;
    if (r == seq_rate) return;
    if (!seq_run) {                             /* from a stop: the page on show, from where the playhead is */
        memset(tr, 0, sizeof tr);
        seq_left = seq_frac = 0; seq_next = 255; seq_loop = 1; seq_pat = seq_view; seq_follow = 0; seq_len = (uint8_t)page_steps(seq_pat); seq_bpm_play = seq_tempo_at(seq_pat, seq_slot, seq_step, seq_len);
        seq_hold = 1; seq_rate = 0;
        if (clk_mode == 1u) midi_send(0xFAu, 0, 0);
        if (brothers_mode == 1u) brothers_lead_run(1);
        seq_run = 1;
    }
    seq_rate = (int8_t)r;
    if (r == 0) { uint32_t t; seq_hold = 1; for (t = 0; t < chan_n; t++) ev_push(EV_OFF, t, 0, 0); }       /* paused: the notes let go */
    else seq_hold = 0;
}
static uint32_t page_empty(uint32_t p)
{
    uint32_t t, s;
    for (t = 0; t < MAXCH; t++)
        for (s = 0; s < SEQ_MAXLEN; s++)
            if (STEP_R(p, t, s)->note) return 0;
    return 1;
}
static void seq_page(int32_t d)                 /* flip the page on show through the pages there are */
{
    seq_view = (uint8_t)(d < 0 ? (seq_view ? seq_view - 1u : page_n - 1u) : (seq_view + 1u < page_n ? seq_view + 1u : 0u));
    seq_follow = 0;
    if (seq_run && seq_loop == 1u) seq_next = seq_view;
}
static uint32_t page_used(uint32_t p)
{
    uint32_t i;
    for (i = 0; i < song_len; i++) if (song[i] == p) return 1;
    return 0;
}
static void pages_trim(void)                    /* an empty last page that no slot holds goes */
{
    while (page_n > 1u && !page_used(page_n - 1u) && page_empty(page_n - 1u)) page_n--;
    if (seq_view >= page_n) seq_view = (uint8_t)(page_n - 1u);
}
static void song_insert_blank(uint32_t at)      /* a new slot after slot `at` holding a new empty page; the cursor's new place, or `at` when full */
{
    uint32_t pos = at < song_len ? at + 1u : song_len;
    if (page_n >= SEQ_PAGES || song_len >= SONG_MAX) return;
    memmove(&song[pos + 1u], &song[pos], song_len - pos);
    page_sig[page_n] = page_sig[song[at < song_len ? at : song_len - 1u]]; page_xb[page_n] = page_xb[song[at < song_len ? at : song_len - 1u]];       /* a new page keeps the meter and length of the one before */
    page_tmp[page_n] = page_tmp[song[at < song_len ? at : song_len - 1u]]; page_gl[page_n] = 0;       /* and its own tempo, if it has one (not the glide) */
    song[pos] = (uint8_t)page_n++;
    song_len++;
    seq_view = song[pos];
}
static void song_remove(uint32_t at)
{
    if (song_len <= 1u || at >= song_len) return;
    memmove(&song[at], &song[at + 1u], song_len - at - 1u);
    song_len--;
    pages_trim();
}
static void song_cycle(uint32_t at, int32_t d)  /* the slot's page: the next one there is, or the one before */
{
    if (at >= song_len) return;
    song[at] = (uint8_t)(d < 0 ? (song[at] ? song[at] - 1u : page_n - 1u) : (song[at] + 1u < page_n ? song[at] + 1u : 0u));
    seq_view = song[at];
    pages_trim();
}
static int seq_dup_page(uint32_t p)              /* a copy of page p as a new last page: its number, or -1 when full */
{
    uint32_t t, b, need = 0;
    if (page_n >= SEQ_PAGES || p >= page_n) return -1;
    for (t = 0; t < MAXCH; t++) for (b = 0; b < SEQ_MAXLEN / SEQ_LEN; b++) need += bmap[p][t][b] != 0;
    if (need > blk_free_n()) return -1;
    for (t = 0; t < MAXCH; t++) for (b = 0; b < SEQ_MAXLEN / SEQ_LEN; b++)
        if (bmap[p][t][b]) { const uint32_t n = blk_new(); memcpy(blk[n - 1u], blk[bmap[p][t][b] - 1u], sizeof blk[0]); bmap[page_n][t][b] = (uint16_t)n; }
    page_sig[page_n] = page_sig[p]; page_xb[page_n] = page_xb[p]; page_tmp[page_n] = page_tmp[p]; page_gl[page_n] = page_gl[p];
    return (int)page_n++;
}
static void seq_toggle(uint32_t track, uint32_t step, uint32_t note)    /* a key on a step: set that note, or clear the step if it already has it */
{
    step_t *x = STEP_W(seq_view, track, step);
    if (x->note == note) { x->note = 0; return; }
    if (!x->note) { x->vel = 100; x->gate = 5; }
    x->note = (uint8_t)note;
}
static void seq_track_clear(uint32_t p, uint32_t t)      /* a track of a page: nothing */
{
    uint32_t b;
    for (b = 0; b < SEQ_MAXLEN / SEQ_LEN; b++) if (bmap[p][t][b]) blk_free(p, t, b);
}
static void seq_track_copy(uint32_t p, uint32_t dst, uint32_t src)       /* a track of a page, copied to another track */
{
    uint32_t b;
    seq_track_clear(p, dst);
    for (b = 0; b < SEQ_MAXLEN / SEQ_LEN; b++)
        if (bmap[p][src][b]) { const uint32_t n = blk_new(); if (!n) return; memcpy(blk[n - 1u], blk[bmap[p][src][b] - 1u], sizeof blk[0]); bmap[p][dst][b] = (uint16_t)n; }
}
static void seq_slide(uint32_t p, uint32_t t, int32_t by)    /* a track's steps along the page's length, round the ends */
{
    step_t tmp[SEQ_MAXLEN];
    const int32_t len = (int32_t)page_steps(p);
    int32_t j, sh = by % len;
    if (sh < 0) sh += len;
    for (j = 0; j < len; j++) tmp[j] = *STEP_R(p, t, (uint32_t)j);
    for (j = 0; j < len; j++) *STEP_W(p, t, (uint32_t)((j + sh) % len)) = tmp[j];
}
static void seq_chan_removed(uint32_t c)        /* a channel is gone: the tracks above it move down */
{
    uint32_t p;
    memset(tr, 0, sizeof tr);
    for (p = 0; p < SEQ_PAGES; p++) {
        uint32_t b;
        for (b = 0; b < SEQ_MAXLEN / SEQ_LEN; b++) if (bmap[p][c][b]) blk_free(p, c, b);
        memmove(&bmap[p][c], &bmap[p][c + 1u], (MAXCH - 1u - c) * sizeof bmap[p][0]);
        memset(&bmap[p][MAXCH - 1u], 0, sizeof bmap[p][0]);
    }
}
static void seq_clear(void)                     /* an empty project: no channels, one page */
{
    chan_n = 0;
    memset(bmap, 0, sizeof bmap); memset(blk_used, 0, sizeof blk_used); blk_hint = 0;
    memset(page_sig, 0, sizeof page_sig); memset(page_xb, 0, sizeof page_xb); memset(page_tmp, 0, sizeof page_tmp); memset(page_gl, 0, sizeof page_gl); seq_len = 16;
    memset(tr, 0, sizeof tr);
    page_n = 1; seq_view = 0; seq_pat = 0; seq_next = 255; seq_step = seq_sub = 0; seq_slot = 0; seq_loop = 0; seq_follow = 0;
    song_len = 1; memset(song, 0, sizeof song);
    seq_bpm = 120; seq_bpm_play = 120;
}

static void seq_put(uint32_t p, uint32_t t, uint32_t s, uint32_t note, uint32_t vel, uint32_t gate)
{
    step_t *x = STEP_W(p, t, s);
    x->note = (uint8_t)note; x->vel = (uint8_t)vel; x->gate = (uint8_t)gate;
}
static int chan_add(uint32_t kind);
static void fm_set(uint32_t c, uint32_t alg, uint32_t ratio, uint32_t index, uint32_t fb, uint32_t decay)      /* an FM channel's operators */
{
    chan[c].in.duty = (uint8_t)alg; chan[c].in.wave = (uint8_t)ratio; chan[c].in.wf = (uint8_t)index; chan[c].in.wc = (uint8_t)fb; chan[c].in.wt = (uint8_t)decay;
    fm_from_classic(chan[c].kind, &chan[c].in, &chan[c].fm);       /* (the old knobs, as an operator patch) */
}

/* THE BLUETOOTH DEMO: a locked project to play with all three brothers (D minor, 124 BPM): page 1 the groove, page 2 the lift. Drum kicks on the first lane, so the sidechain pumps. */
static void seq_bt_demo(void)
{
    static const uint8_t BASS[2][16] = {{38, 0, 0, 38, 0, 0, 41, 0, 38, 0, 0, 43, 0, 0, 41, 0}, {46, 0, 0, 46, 0, 0, 45, 0, 43, 0, 0, 41, 0, 41, 43, 0}};
    static const uint8_t LEAD[2][16] = {{62, 0, 65, 0, 69, 0, 65, 0, 62, 0, 65, 0, 67, 0, 65, 0}, {70, 0, 74, 0, 77, 0, 74, 0, 69, 0, 72, 0, 76, 0, 72, 0}};
    uint32_t p, s;
    seq_clear();
    chan_add(KD_DRUM0); chan_add(KD_NES_TRI); chan_add(KD_NES_PU);
    chan[2].in.duty = 1; chan[2].in.vol = 8;
    seq_bpm = 124; page_n = 2; song_len = 1; song[0] = 0;
    for (p = 0; p < 2u; p++)
        for (s = 0; s < 16u; s++) {
            uint32_t hit = (s % 4u == 0u ? 1u : 0u) | (s == 4u || s == 12u ? 2u : 0u) | (s % 4u == 2u ? 128u : 0u);          /* a drum step: bit 0 kick, 1 snare, 7 closed hat */
            if (hit) seq_put(p, 0, s, hit, 0, s == 0u ? 1u : 0u);
            if (BASS[p][s]) seq_put(p, 1, s, BASS[p][s], 127, 9);
            if (LEAD[p][s]) seq_put(p, 2, s, LEAD[p][s], 100, 9);
        }
}

/* Korobeiniki (the Russian folk song, public domain; the Tetris tune): eight bars, one page each, in A minor. The first project of a new device */
static void seq_tetris(void)
{
    static const uint8_t N[8][6] = {{76, 71, 72, 74, 72, 71}, {69, 69, 72, 76, 74, 72}, {71, 72, 74, 76, 0, 0}, {72, 69, 69, 0, 0, 0},
                                    {74, 77, 81, 79, 77, 0}, {76, 72, 76, 74, 72, 0}, {71, 71, 72, 74, 76, 0}, {72, 69, 69, 0, 0, 0}};
    static const uint8_t L[8][6] = {{4, 2, 2, 4, 2, 2}, {4, 2, 2, 4, 2, 2}, {6, 2, 4, 4, 0, 0}, {4, 4, 8, 0, 0, 0},
                                    {6, 2, 4, 2, 2, 0}, {6, 2, 4, 2, 2, 0}, {4, 2, 2, 4, 4, 0}, {4, 4, 8, 0, 0, 0}};
    static const uint8_t BASS[8] = {40, 45, 40, 45, 50, 48, 40, 45};                      /* the chord root of each bar: E A E A D C E A */
    static const uint8_t STAB[8][3] = {{56, 59, 64}, {57, 60, 64}, {56, 59, 64}, {57, 60, 64}, {57, 62, 65}, {55, 60, 64}, {56, 59, 64}, {57, 60, 64}};
    uint32_t p, i, s, k;
    seq_clear();
    chan_add(KD_NES_PU); chan_add(KD_NES_TRI); chan_add(KD_GB_PU); chan_add(KD_SMS);
    chan[2].in.duty = 1; chan[2].in.vol = 9;                                    /* the stabs: thin pulse */
    chan[3].in.vol = 5;                                                         /* the lead doubled an octave down, quiet */
    seq_bpm = 144; page_n = 8; song_len = 8;
    for (p = 0; p < 8u; p++) {
        song[p] = (uint8_t)p;
        for (i = 0, s = 0; i < 6u && L[p][i]; s += L[p][i], i++) {
            uint32_t g = L[p][i] * 6u - 3u;
            seq_put(p, 0, s, N[p][i], 110, g);
            seq_put(p, 3, s, N[p][i] - 12u, 100, g);
        }
        for (k = 0; k < 8u; k++) seq_put(p, 1, k * 2u, BASS[p] + (k & 1u ? 12u : 0u), 127, 9);       /* bass: root, octave, root, octave */
        for (k = 0; k < 4u; k++) seq_put(p, 2, 2u + k * 4u, STAB[p][1u + (k & 1u)], 100, 4);          /* off-beat chord tones */
    }
}
