/* SPDX-License-Identifier: GPL-3.0-only */
/* The sequencer, included by chips.c: a page is 16 steps of 1/16 note for every channel, played by a clock in the audio ISR.
 *   a song is a list of slots, each one a page (1 1 1 1 2 3 1 1 ...); PLAY goes down the list and round again, a held PLAY loops one page
 *   a track is a channel (the same number); a step holds a note, its velocity and its length in clock ticks
 *   the clock ticks 24 times a beat (6 a step) and each tick lands on its own sample, not on the 60 Hz tick
 *   an arp is a channel's setting (chips.c), not a step's
 * The main loop edits the arrays and calls seq_play / seq_stop; one writer on each side, plain bytes. */
#define SEQ_LEN 16u                             /* steps in a page */
#define SEQ_PAGES 64u
#define SEQ_PPS 6u                              /* clock ticks per step */

typedef struct { uint8_t note, vel, gate, spare; } step_t;      /* note 0: nothing; gate in ticks */
#ifndef POOL
#define POOL __attribute__((section(".pool")))        /* big arrays live in the pool (a native test build defines POOL as nothing) */
#endif
static step_t pat[SEQ_PAGES][MAXCH][SEQ_LEN] POOL;
static uint32_t page_n = 1;                     /* pages in use */
static volatile uint32_t seq_bpm = 120;
#define SONG_MAX 128
static uint8_t song[SONG_MAX];                  /* the page of each slot */
static uint32_t song_len = 1;
static volatile int32_t fx_speed = 256;        /* the tape pad (fx.c): 256 normal, 512 double, 128 half, 0 stopped; pitch and level follow */
static volatile uint8_t fx_rpt;                 /* beat repeat (a punch-in effect): ticks between repeats, 0 off */
static uint8_t rpt_step, rpt_page, rpt_cnt;     /* ... of this step, on this page */
static volatile int8_t seq_rate;                /* the tape dial: -3..3, 0 pause; 1 half speed, 2 normal, 3 double; negative runs backwards */
static volatile uint8_t seq_hold, seq_seeked;   /* seq_hold: paused (the clock stands still); seq_seeked: the playhead was moved, PLAY goes on from there */
static volatile uint32_t seq_ticks;             /* clock ticks since play: a recorded note's length */
static volatile uint8_t seq_run, seq_pat, seq_next = 255, seq_step, seq_sub, seq_slot, seq_loop, seq_follow;     /* seq_loop: one page round and round; seq_follow: the rack shows the page that plays */
static uint8_t seq_view;                        /* the page the screen shows and the keys write; the player follows it at the end of the loop */
static uint32_t seq_left, seq_frac;             /* samples to the next tick, and the fraction a tick keeps */
static struct { uint8_t on, gate; } tr[MAXCH];

static uint32_t seq_period(void)                /* samples in one tick at the tempo, the remainder carried so the tempo is exact */
{
    uint32_t bpm = seq_bpm < 40u ? 40u : seq_bpm > 240u ? 240u : seq_bpm;
    uint32_t q = ((FS * 5u / 2u) << 8) / bpm + seq_frac;
    seq_frac = q & 255u;
    q >>= 8;
    return seq_rate == 1 || seq_rate == -1 ? q * 2u : seq_rate == 3 || seq_rate == -3 ? (q > 1u ? q / 2u : 1u) : q;     /* half speed, normal, double */
}
static void seq_clock(void)                     /* one tick, in the audio ISR */
{
    uint32_t t;
    seq_ticks++;
    for (t = 0; t < chan_n; t++)                /* what is already sounding: its length */
        if (tr[t].on && tr[t].gate && !--tr[t].gate) { ev_do(EV_OFF, t, 0, 0); tr[t].on = 0; }
    if (fx_rpt) {                               /* beat repeat: the step that was held sounds again and again */
        if (!rpt_cnt) for (t = 0; t < chan_n; t++) {
            const step_t *s = &pat[rpt_page][t][rpt_step];
            if (!s->note) continue;
            tr[t].on = 1; tr[t].gate = s->gate < fx_rpt ? s->gate : (uint8_t)(fx_rpt > 1u ? fx_rpt - 1u : 1u);
            ev_do(EV_ON, t, s->note, s->vel);
        }
        if (++rpt_cnt >= fx_rpt) rpt_cnt = 0;
    }
    if (!seq_sub && !fx_rpt) {                  /* a new step */
        for (t = 0; t < chan_n; t++) {
            const step_t *s = &pat[seq_pat][t][seq_step];
            if (!s->note) continue;
            tr[t].on = 1; tr[t].gate = s->gate;
            ev_do(EV_ON, t, s->note, s->vel);
        }
    }
    if (++seq_sub >= SEQ_PPS) {
        seq_sub = 0;
        if (++seq_step >= SEQ_LEN) {
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
        seq_step = SEQ_LEN - 1u;
        if (!seq_loop) {                        /* the song: the slot before */
            seq_slot = seq_slot ? seq_slot - 1u : (uint8_t)(song_len - 1u);
            seq_pat = song[seq_slot] < page_n ? song[seq_slot] : 0;
            if (seq_follow) seq_view = seq_pat;
        }
    }
    if (fx_rpt) return;
    for (t = 0; t < chan_n; t++) {
        const step_t *s = &pat[seq_pat][t][seq_step];
        if (!s->note) continue;
        tr[t].on = 1; tr[t].gate = s->gate;
        ev_do(EV_ON, t, s->note, s->vel);
    }
}
static void seq_tick(void) { if (seq_rate < 0) seq_back(); else seq_clock(); }

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
    seq_run = 1;
}
static void seq_stop(void)
{
    uint32_t t;
    seq_run = 0; seq_rate = 0; seq_hold = 0;
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
        seq_left = seq_frac = 0; seq_next = 255; seq_loop = 1; seq_pat = seq_view; seq_follow = 0;
        seq_hold = 1; seq_rate = 0; seq_run = 1;
    }
    seq_rate = (int8_t)r;
    if (r == 0) { uint32_t t; seq_hold = 1; for (t = 0; t < chan_n; t++) ev_push(EV_OFF, t, 0, 0); }       /* paused: the notes let go */
    else seq_hold = 0;
}
static uint32_t page_empty(uint32_t p)
{
    uint32_t t, s;
    for (t = 0; t < MAXCH; t++)
        for (s = 0; s < SEQ_LEN; s++)
            if (pat[p][t][s].note) return 0;
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
    if (page_n >= SEQ_PAGES || p >= page_n) return -1;
    memcpy(pat[page_n], pat[p], sizeof pat[p]);
    return (int)page_n++;
}
static void seq_toggle(uint32_t track, uint32_t step, uint32_t note)    /* a key on a step: set that note, or clear the step if it already has it */
{
    step_t *x = &pat[seq_view][track][step];
    if (x->note == note) { x->note = 0; return; }
    if (!x->note) { x->vel = 100; x->gate = 5; }
    x->note = (uint8_t)note;
}
static void seq_chan_removed(uint32_t c)        /* a channel is gone: the tracks above it move down */
{
    uint32_t p;
    memset(tr, 0, sizeof tr);
    for (p = 0; p < SEQ_PAGES; p++) {
        memmove(&pat[p][c], &pat[p][c + 1u], (MAXCH - 1u - c) * sizeof pat[p][0]);
        memset(&pat[p][MAXCH - 1u], 0, sizeof pat[p][0]);
    }
}
static void seq_clear(void)                     /* an empty project: no channels, one page */
{
    chan_n = 0;
    memset(pat, 0, sizeof pat);
    memset(tr, 0, sizeof tr);
    page_n = 1; seq_view = 0; seq_pat = 0; seq_next = 255; seq_step = seq_sub = 0; seq_slot = 0; seq_loop = 0; seq_follow = 0;
    song_len = 1; memset(song, 0, sizeof song);
    seq_bpm = 120;
}

static void seq_put(uint32_t p, uint32_t t, uint32_t s, uint32_t note, uint32_t vel, uint32_t gate)
{
    step_t *x = &pat[p][t][s];
    x->note = (uint8_t)note; x->vel = (uint8_t)vel; x->gate = (uint8_t)gate;
}
static int chan_add(uint32_t kind);

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

/* In the Hall of the Mountain King (Grieg, 1875, public domain), in B minor. It speeds up by packing the notes tighter: slow, eighths, then sixteenths, and the channels come in one by one. 8 pages. */
static void ph_put(uint32_t p, uint32_t t, uint32_t at, const uint8_t *n, uint32_t from, uint32_t cnt, uint32_t d, uint32_t shift, uint32_t vel)
{
    uint32_t k;
    for (k = 0; k < cnt; k++) seq_put(p, t, at + k * d, n[from + k] + shift, vel, d * 6u - 2u);
}
static void seq_mountain(void)
{
    static const uint8_t P1[7] = {59, 61, 62, 64, 66, 62, 66}, P2[7] = {64, 66, 67, 69, 71, 67, 71};
    uint32_t p, s;
    seq_clear();
    chan_add(KD_NES_PU); chan_add(KD_NES_TRI); chan_add(KD_GB_PU); chan_add(KD_SMS); chan_add(KD_GB_WAV);
    chan[2].in.vol = 7; chan[3].in.vol = 7;
    chan[4].in.wave = 10; chan[4].in.wf = 5; chan[4].in.wt = 6; chan[4].in.vol = 8;           /* the drone: a folded, twisted triangle */
    seq_bpm = 132; page_n = 8; song_len = 8;
    for (p = 0; p < 8u; p++) song[p] = (uint8_t)p;
    ph_put(0, 0, 0, P1, 0, 4, 4, 0, 90); seq_put(0, 1, 0, 47, 120, 22); seq_put(0, 1, 8, 47, 120, 22);                /* bars 1 and 2: the tune, one note in four steps */
    ph_put(1, 0, 0, P1, 4, 3, 4, 0, 90); seq_put(1, 1, 0, 47, 120, 22); seq_put(1, 1, 8, 47, 120, 22);
    ph_put(2, 0, 0, P1, 0, 7, 2, 0, 100);                                                                           /* then in eighths */
    ph_put(3, 0, 0, P2, 0, 7, 2, 0, 100);
    for (s = 0; s < 16u; s += 4u) { seq_put(2, 1, s, 47, 120, 10); seq_put(3, 1, s, 40, 120, 10); }
    for (p = 4; p < 8u; p++) {                                                                                      /* then in sixteenths, twice a bar */
        const uint8_t *n = (p & 1u) ? P2 : P1;
        uint32_t root = (p & 1u) ? 40u : 47u;
        ph_put(p, 0, 0, n, 0, 7, 1, 0, 110); ph_put(p, 0, 8, n, 0, 7, 1, 0, 110);
        for (s = 0; s < 16u; s += 2u) seq_put(p, 1, s, root + ((s & 2u) ? 12u : 0u), 120, 8);
        ph_put(p, 2, 0, n, 0, 7, 1, 12, 80); ph_put(p, 2, 8, n, 0, 7, 1, 12, 80);                                    /* an octave up joins */
        if (p >= 5u) for (s = 2; s < 16u; s += 4u) seq_put(p, 3, s, root + 19u, 90, 4);                             /* the off-beat stabs */
        if (p >= 6u) seq_put(p, 4, 0, 59, 100, 90);                                                                  /* the drone */
    }
}

/* Fur Elise (Beethoven, 1810, public domain): the main theme, A minor. A rest is note 0. */
static void seq_elise(void)
{
    static const uint8_t N[] = {76, 75,  76, 75, 76, 71, 74, 72,  69, 0, 60, 64, 69,  71, 0, 64, 68, 71,  72, 0, 64, 76, 75,
                                76, 75, 76, 71, 74, 72,  69, 0, 60, 64, 69,  71, 0, 64, 72, 71,  69};
    static const uint8_t L[] = { 1, 1,   1, 1, 1, 1, 1, 1,   2, 1, 1, 1, 1,   2, 1, 1, 1, 1,   2, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1,   2, 1, 1, 1, 1,   2, 1, 1, 1, 1,   8};
    static const uint8_t LH[6][4] = {{8, 45, 52, 57}, {14, 40, 52, 56}, {20, 45, 52, 57}, {32, 45, 52, 57}, {38, 40, 52, 56}, {44, 45, 52, 57}};
    uint32_t i, at = 0, k;
    seq_clear();
    chan_add(KD_NES_PU); chan_add(KD_NES_TRI); chan_add(KD_GB_PU);
    chan[0].in.vib_dep = 2; chan[2].in.duty = 1; chan[2].in.vol = 6;
    seq_bpm = 66; page_n = 4; song_len = 4;
    for (i = 0; i < 4u; i++) song[i] = (uint8_t)i;
    for (i = 0; i < sizeof L; i++) {
        if (N[i]) {
            uint32_t g = L[i] * 6u - 1u;
            seq_put(at / 16u, 0, at % 16u, N[i], 100, g > 90u ? 90u : g);
            seq_put(at / 16u, 2, at % 16u, N[i] - 12u, 70, g > 90u ? 90u : g);               /* a quiet double an octave down */
        }
        at += L[i];
    }
    for (i = 0; i < 6u; i++)                                                                  /* the left hand: root, fifth, octave */
        for (k = 0; k < 3u; k++) { uint32_t st = LH[i][0] + k * 2u; seq_put(st / 16u, 1, st % 16u, LH[i][1u + k], 110, 10); }
}

/* Toccata and Fugue in D minor (Bach, c. 1704, public domain): the opening, in octaves, then chords. */
static void seq_toccata(void)
{
    uint32_t i;
    seq_clear();
    chan_add(KD_NES_PU); chan_add(KD_NES_TRI); chan_add(KD_GB_PU); chan_add(KD_SMS);
    chan[0].in.vib_dep = 3; chan[2].in.arp = 3; chan[2].in.arp_rate = 3; chan[2].in.arp_dir = 2; chan[2].in.vol = 8; chan[3].in.vol = 7;
    seq_bpm = 84; page_n = 4; song_len = 4;
    for (i = 0; i < 4u; i++) song[i] = (uint8_t)i;
    for (i = 0; i < 2u; i++) {                                                     /* the motif high, then an octave down */
        uint32_t o = i * 32u, sh = i * 12u;
        seq_put((o) / 16u, 0, o % 16u, 81 - sh, 120, 4); seq_put((o + 1u) / 16u, 0, (o + 1u) % 16u, 79 - sh, 120, 4); seq_put((o + 2u) / 16u, 0, (o + 2u) % 16u, 81 - sh, 120, 60);
        seq_put((o + 16u) / 16u, 0, 0, 79 - sh, 120, 4); seq_put((o + 16u) / 16u, 0, 1, 77 - sh, 120, 4); seq_put((o + 16u) / 16u, 0, 2, 76 - sh, 120, 4);
        seq_put((o + 16u) / 16u, 0, 3, 74 - sh, 120, 4); seq_put((o + 16u) / 16u, 0, 4, 73 - sh, 120, 4); seq_put((o + 16u) / 16u, 0, 5, 74 - sh, 120, 90);
        seq_put((o) / 16u, 3, o % 16u, 69 - sh + (i ? 0u : 0u), 90, 4);          /* the organ's second voice, an octave below */
        seq_put((o) / 16u, 3, (o + 1u) % 16u, 67 - sh, 90, 4); seq_put((o) / 16u, 3, (o + 2u) % 16u, 69 - sh, 90, 60);
        seq_put((o + 16u) / 16u, 3, 0, 67 - sh, 90, 4); seq_put((o + 16u) / 16u, 3, 1, 65 - sh, 90, 4); seq_put((o + 16u) / 16u, 3, 2, 64 - sh, 90, 4);
        seq_put((o + 16u) / 16u, 3, 3, 62 - sh, 90, 4); seq_put((o + 16u) / 16u, 3, 4, 61 - sh, 90, 4); seq_put((o + 16u) / 16u, 3, 5, 62 - sh, 90, 90);
    }
    seq_put(2, 1, 0, 38, 127, 90); seq_put(3, 1, 0, 33, 127, 40); seq_put(3, 1, 8, 38, 127, 40);                        /* the pedal */
    seq_put(2, 2, 0, 62, 100, 90); seq_put(3, 2, 0, 57, 100, 50); seq_put(3, 2, 8, 62, 100, 50);                        /* the chords, arped */
}
