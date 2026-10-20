/* SPDX-License-Identifier: GPL-3.0-only */
/* The sequencer, included by chips.c: patterns of steps per track, played by a clock in the audio ISR.
 *   a track is a chip channel (chip * 4 + channel: 12 tracks); a pattern is up to 64 steps of 1/16 note; 4 patterns, A..D
 *   the clock ticks 24 times a beat (6 a step) and each tick lands on its own sample, not on the 60 Hz tick
 *   a step holds a note, its length in clock ticks and an optional arp: a chord run by the one voice
 * The main loop edits the arrays and calls seq_play / seq_stop; one writer on each side, plain bytes. */
#define SEQ_TRACKS 12u
#define SEQ_PATS 4u
#define SEQ_MAXLEN 64u
#define SEQ_PPS 6u                              /* clock ticks per step */

typedef struct { uint8_t note, vel, gate, arp; } step_t;       /* note 0: nothing; gate in ticks; arp: bit 7 on, 6..5 rate, 4..3 direction, 2..0 shape */
enum { ARP_UP, ARP_DOWN, ARP_UPDOWN };
static const uint8_t ARP_RATE[4] = {6, 3, 2, 1};                /* ticks per arp note */
static const uint8_t ARP_N[8] = {2, 4, 4, 4, 4, 4, 4, 4};
static const int8_t ARP_IV[8][4] = {                            /* octave, major, minor, maj7, min7, sus4, power, dim */
    {0, 12, 0, 12}, {0, 4, 7, 12}, {0, 3, 7, 12}, {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 12}, {0, 7, 12, 19}, {0, 3, 6, 9},
};
#define ARP(shape, dir, rate) ((uint8_t)(0x80u | (rate) << 5 | (dir) << 3 | (shape)))

static step_t pat[SEQ_PATS][SEQ_TRACKS][SEQ_MAXLEN];
static uint8_t pat_len[SEQ_PATS] = {16, 16, 16, 16};
static uint8_t song[16] = {0, 1, 2, 3}, song_len = 4;
static volatile uint32_t seq_bpm = 120;
static volatile uint8_t seq_run, seq_song_on, seq_pat, seq_next = 255, seq_pos, seq_step, seq_sub;
static uint8_t seq_view;                        /* the page the screen shows and the keys edit; the player follows it at the end of the loop */
static volatile uint16_t seq_mute;              /* bit per track */
static uint32_t seq_left, seq_frac;             /* samples to the next tick, and the fraction a tick keeps */
static struct { uint8_t on, root, gate, arp, since, k; } tr[SEQ_TRACKS];

static uint32_t arp_note(uint32_t root, uint32_t arp, uint32_t k)
{
    uint32_t n = ARP_N[arp & 7u], i;
    if ((arp >> 3 & 3u) == ARP_UPDOWN) { k %= 2u * n - 2u; i = k < n ? k : 2u * n - 2u - k; }
    else { i = k % n; if ((arp >> 3 & 3u) == ARP_DOWN) i = n - 1u - i; }
    i = root + (uint32_t)ARP_IV[arp & 7u][i];
    return i > 127u ? 127u : i;
}
static uint32_t seq_period(void)                /* samples in one tick at the tempo, the remainder carried so the tempo is exact */
{
    uint32_t bpm = seq_bpm < 40u ? 40u : seq_bpm > 240u ? 240u : seq_bpm;
    uint32_t q = ((FS * 5u / 2u) << 8) / bpm + seq_frac;
    seq_frac = q & 255u;
    return q >> 8;
}
static void seq_clock(void)                     /* one tick, in the audio ISR */
{
    uint32_t t;
    for (t = 0; t < SEQ_TRACKS; t++) {          /* what is already sounding: its length, its arp */
        if (!tr[t].on) continue;
        tr[t].since++;
        if (tr[t].gate && !--tr[t].gate) { ev_do(EV_OFF, t, 0, 0); tr[t].on = 0; continue; }
        if ((tr[t].arp & 0x80u) && tr[t].since % ARP_RATE[tr[t].arp >> 5 & 3u] == 0u)
            ev_do(EV_PITCH, t, arp_note(tr[t].root, tr[t].arp, ++tr[t].k), 0);
    }
    if (!seq_sub) {                             /* a new step */
        for (t = 0; t < SEQ_TRACKS; t++) {
            const step_t *s = &pat[seq_pat][t][seq_step];
            if (!s->note || (seq_mute >> t & 1u)) continue;
            tr[t].on = 1; tr[t].root = s->note; tr[t].gate = s->gate; tr[t].arp = s->arp; tr[t].since = tr[t].k = 0;
            ev_do(EV_ON, t, (s->arp & 0x80u) ? arp_note(s->note, s->arp, 0) : s->note, s->vel);
        }
    }
    if (++seq_sub >= SEQ_PPS) {
        seq_sub = 0;
        if (++seq_step >= pat_len[seq_pat]) {
            seq_step = 0;
            if (seq_song_on && song_len) { seq_pos = (uint8_t)((seq_pos + 1u) % song_len); seq_pat = (uint8_t)(song[seq_pos] % SEQ_PATS); }
            else if (seq_next < SEQ_PATS) { seq_pat = seq_next; seq_next = 255; }
        }
    }
}

/* ---- the main loop's side */
static void seq_play(void)
{
    memset(tr, 0, sizeof tr);
    seq_step = seq_sub = 0; seq_left = seq_frac = 0;
    seq_pat = seq_view; seq_next = 255;
    if (seq_song_on) { seq_pos = 0; seq_pat = (uint8_t)(song[0] % SEQ_PATS); }
    seq_run = 1;
}
static void seq_stop(void)
{
    uint32_t t;
    seq_run = 0;
    for (t = 0; t < SEQ_TRACKS; t++) ev_push(EV_OFF, t, 0, 0);
}

static void seq_page(int32_t d)                 /* flip the page */
{
    seq_view = (uint8_t)((seq_view + (d < 0 ? SEQ_PATS - 1u : 1u)) % SEQ_PATS);
    if (seq_run && !seq_song_on) seq_next = seq_view;
}
static void seq_toggle(uint32_t track, uint32_t step, uint32_t note)    /* a key on a step: set that note, or clear the step if it already has it */
{
    step_t *x = &pat[seq_view][track][step];
    if (x->note == note) { x->note = 0; return; }
    if (!x->note) { x->vel = 100; x->gate = 5; x->arp = 0; }
    x->note = (uint8_t)note;
}

/* a little tune to hear it with: A minor, F, C, G; every track has a job (hardcoded until the editor exists) */
static void seq_put(uint32_t p, uint32_t t, uint32_t s, uint32_t note, uint32_t vel, uint32_t gate, uint32_t arp)
{
    step_t *x = &pat[p][t][s];
    x->note = (uint8_t)note; x->vel = (uint8_t)vel; x->gate = (uint8_t)gate; x->arp = (uint8_t)arp;
}
static void seq_demo(void)
{
    static const uint8_t ROOT[4] = {45, 41, 48, 43}, SHAPE[4] = {2, 1, 1, 1};
    static const uint8_t TUNE[16] = {0, 0, 2, 0, 4, 0, 2, 0, 3, 0, 2, 0, 1, 0, 0, 0};      /* chord tone per step for the leads, 0 = rest */
    uint32_t p, s, r, sh;
    memset(pat, 0, sizeof pat);
    for (p = 0; p < 4u; p++) {
        r = ROOT[p]; sh = SHAPE[p];
        for (s = 0; s < 16u; s++) {
            uint32_t n = TUNE[s];
            if (n) {
                seq_put(p, 4, s, r + 24u + ((n * 3u) % 8u), 110, 4, 0);                      /* GB PU1: the tune */
                if (!(s & 1u)) seq_put(p, 8, s, r + 12u + n * 2u, 100, 5, 0);                /* SMS TN1: a line under it */
            }
            if (!(s & 3u)) seq_put(p, 6, s, r + 12u, 127, 5, 0);                             /* GB WAV: bass quarters */
        }
        seq_put(p, 0, 0, r + 24u, 100, 11, ARP(sh, ARP_UP, 1));                              /* NES PU1: arp stabs */
        seq_put(p, 0, 6, r + 24u, 100, 5, ARP(sh, ARP_UPDOWN, 1));
        seq_put(p, 0, 10, r + 24u, 100, 5, ARP(sh, ARP_UP, 2));
        seq_put(p, 1, 3, r + 31u, 90, 8, 0); seq_put(p, 1, 11, r + 36u, 90, 8, 0);           /* NES PU2: answers */
        for (s = 0; s < 16u; s += 3u) seq_put(p, 2, s, r + ((s & 3u) ? 12u : 0u), 127, 5, 0);   /* NES TRI: bass */
        seq_put(p, 9, 2, r + 24u, 90, 3, ARP(sh, ARP_DOWN, 1)); seq_put(p, 9, 10, r + 24u, 90, 3, ARP(sh, ARP_DOWN, 1));   /* SMS TN2: down arps */
        seq_put(p, 5, 0, r + 24u, 80, 22, ARP(sh, ARP_UPDOWN, 2));                           /* GB PU2: a long shimmer */
        seq_put(p, 10, 0, r + 12u, 127, 30, 0); seq_put(p, 10, 8, r + 19u, 127, 20, 0);             /* SMS TN3: low drone */
    }
    song_len = 4; song[0] = 0; song[1] = 1; song[2] = 2; song[3] = 3; seq_song_on = 0;
}
