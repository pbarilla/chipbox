/* SPDX-License-Identifier: GPL-3.0-only */
/* Punch-in effects, included by chips.c after seq.c: 16 pads, each on while its key is held.
 *   the master chain (gate, kill, lo-fi, filters, echo, tape fade) runs on the mix in chip_render, only while something is on
 *   pitch effects (dive, wobble, tape stop) are one offset for every voice, moved at the 60 Hz tick
 *   the tape pad hands SELECT to the tape dial and lets the pitch follow the speed; beat repeat steers the sequencer
 * The main loop calls fx_apply with the pads that are held; one writer on each side. */
enum { FX_TAPE, FX_R8, FX_R16, FX_R32, FX_LPF, FX_HPF, FX_ECHO, FX_VERB, FX_CRUSH, FX_DECI, FX_GATE, FX_DIVE, FX_WOB, FX_SOLO, FX_KILL, FX_LATCH, FXN };
#define FXB(i) (1u << (i))
#define ECHO_MAX 22050u                         /* half a second of echo: at slow tempos it is cut short */

static volatile uint32_t fx_mask;               /* the pads held */
static uint32_t fx_held;                        /* main loop: the pads handled so far */
static int8_t fx_rate_save;                     /* the tape dial before the tape pad took it */
static uint8_t fx_hold_save, fx_run_save;
static int32_t fx_dive, fx_wob_ph;
static uint32_t fx_lp_i, fx_hp_i, fx_cnt, fx_echo_tail, fx_verb_tail, echo_clr = ECHO_MAX;       /* echo_clr: how much of the delay line has been wiped since its tail ended */
static int16_t echo_buf[ECHO_MAX][2] POOL;

#define VB_N 4
static const uint16_t VB_LEN[VB_N] = {1116, 1277, 1422, 1617};     /* comb delays, samples */
static const uint16_t AP_LEN[2] = {225, 556};
static int16_t vb_comb[VB_N][1617] POOL, vb_ap[2][556] POOL;
static uint16_t vb_pos[VB_N], ap_pos[2];
static int32_t vb_lp[VB_N];

static void fx_press(uint32_t i)                /* main loop: a pad went down */
{
    static const uint8_t RPT[3] = {12, 6, 3};
    switch (i) {
    case FX_TAPE:                               /* the tape starts paused: the dial goes on from there, right to play, left to rewind */
        fx_rate_save = seq_rate; fx_hold_save = seq_hold; fx_run_save = seq_run;
        if (seq_run) { uint32_t c; seq_rate = 0; seq_hold = 1; for (c = 0; c < chan_n; c++) ev_push(EV_OFF, c, 0, 0); }
        break;
    case FX_R8: case FX_R16: case FX_R32:
        if (seq_run && !seq_hold) { rpt_step = seq_step; rpt_page = seq_pat; rpt_cnt = 0; fx_rpt = RPT[i - FX_R8]; }
        break;
    default: break;
    }
}
static void fx_release(uint32_t i)
{
    switch (i) {
    case FX_TAPE:                               /* the dial goes back to where it was */
        if (!fx_run_save) seq_stop();
        else if (seq_run) { seq_rate = fx_rate_save; seq_hold = fx_hold_save; }
        break;
    case FX_R8: case FX_R16: case FX_R32: fx_rpt = 0; break;
    default: break;
    }
}
static void fx_apply(uint32_t mask)             /* the pads held now: press and release what changed */
{
    uint32_t i, ch = mask ^ fx_held;
    for (i = 0; i < FXN; i++)
        if (ch & FXB(i)) { if (mask & FXB(i)) fx_press(i); else fx_release(i); }
    fx_held = mask;
    fx_mask = mask;
}

static void fx_tick(void)                       /* 60 Hz, audio side: the ramps */
{
    uint32_t m = fx_mask;
    int32_t wob = 0, tape;
    static const int16_t SPD[4] = {0, 128, 256, 512};
    int32_t tgt = (m & FXB(FX_TAPE)) && seq_run ? SPD[seq_rate < 0 ? -seq_rate : seq_rate] : 256;     /* the speed the dial asks for */
    if (fx_speed > tgt) fx_speed = tgt + (fx_speed - tgt) * 13 / 16;
    else if (fx_speed < tgt) fx_speed += (tgt - fx_speed) / 3 + 4;
    if (fx_speed < 4 && tgt == 0) fx_speed = 0;
    if (fx_speed > 512) fx_speed = 512;
    if (fx_speed > tgt && fx_speed - tgt < 4) fx_speed = tgt;
    tape = fx_speed < 256 ? -(256 - fx_speed) * 24 : (fx_speed - 256) * 12;       /* an octave for double or half, two down at a halt */
    if (m & FXB(FX_DIVE)) { fx_dive -= 85; if (fx_dive < -3072) fx_dive = -3072; }       /* an octave in 0.6 s */
    else if (fx_dive < 0) { fx_dive += 220; if (fx_dive > 0) fx_dive = 0; }
    if (m & FXB(FX_WOB)) { fx_wob_ph += 683; wob = VIB[(fx_wob_ph >> 8) & 31] * 100 / 127; }     /* 5 Hz, +- 0.4 semitone */
    fx_pitch = tape + fx_dive + wob;
    if (m & FXB(FX_LPF)) { if (++fx_cnt % 3u == 0u && fx_lp_i < 15u) fx_lp_i++; }
    else if (fx_lp_i) fx_lp_i = fx_lp_i > 2u ? fx_lp_i - 2u : 0u;
    if (m & FXB(FX_HPF)) { if (++fx_cnt % 3u == 0u && fx_hp_i < 15u) fx_hp_i++; }
    else if (fx_hp_i) fx_hp_i = fx_hp_i > 2u ? fx_hp_i - 2u : 0u;
    if (m & FXB(FX_ECHO)) fx_echo_tail = 150u; else if (fx_echo_tail) fx_echo_tail--;       /* the echo rings on for 2.5 s */
    if (m & FXB(FX_VERB)) fx_verb_tail = 200u; else if (fx_verb_tail) fx_verb_tail--;
}

/* the master chain, over n stereo frames, only while something needs it */
static int32_t fx_g = 256, fx_held_l, fx_held_r, fx_lp1[2], fx_lp2[2];
static uint32_t fx_hold_n, fx_gp, echo_pos, vb_dirty;
static void fx_master(int32_t *out, uint32_t n)
{
    uint32_t m = fx_mask, i, bpm = seq_bpm < 40u ? 40u : seq_bpm > 240u ? 240u : seq_bpm;
    uint32_t on = m & (FXB(FX_VERB) | FXB(FX_CRUSH) | FXB(FX_DECI) | FXB(FX_GATE) | FXB(FX_KILL) | FXB(FX_LPF) | FXB(FX_HPF) | FXB(FX_ECHO))
                  || fx_lp_i || fx_hp_i || fx_echo_tail || fx_verb_tail || fx_speed < 256 || fx_g != 256;
    uint32_t gate_p = 661500u / bpm, echo_d = 1323000u / bpm;
    uint32_t hold = (m & FXB(FX_DECI)) ? 8u : (m & FXB(FX_CRUSH)) ? 3u : 1u;
    int32_t a_lp = FX_LP_A[fx_lp_i], a_hp = FX_HP_A[fx_hp_i], tg = fx_speed >= 256 ? 256 : fx_speed * fx_speed >> 8;
    if (fx_echo_tail) echo_clr = 0;
    else if (echo_clr < ECHO_MAX) {                     /* the delay line is wiped a piece at a time once its tail is over, so the next echo starts clean */
        uint32_t w = ECHO_MAX - echo_clr < 2048u ? ECHO_MAX - echo_clr : 2048u;
        memset(echo_buf[echo_clr], 0, w * sizeof echo_buf[0]);
        echo_clr += w;
    }
    if (!fx_verb_tail && vb_dirty) { memset(vb_comb, 0, sizeof vb_comb); memset(vb_ap, 0, sizeof vb_ap); memset(vb_lp, 0, sizeof vb_lp); vb_dirty = 0; }
    if (!on) { fx_lp1[0] = fx_lp1[1] = fx_lp2[0] = fx_lp2[1] = 0; return; }
    if (echo_d > ECHO_MAX) echo_d = ECHO_MAX;
    for (i = 0; i < n; i++) {
        int32_t x[2], target = 256, c;
        if (m & FXB(FX_KILL)) target = 0;
        if (m & FXB(FX_GATE)) { if (++fx_gp >= gate_p) fx_gp = 0; if (fx_gp >= gate_p / 2u) target = 0; }
        fx_g += target > fx_g ? (target - fx_g > 4 ? 4 : target - fx_g) : (fx_g - target > 4 ? -4 : target - fx_g);        /* a short ramp: no clicks */
        x[0] = out[2u * i] * fx_g >> 8; x[1] = out[2u * i + 1u] * fx_g >> 8;
        if (hold > 1u || (m & FXB(FX_CRUSH))) {
            if (++fx_hold_n >= hold) { fx_hold_n = 0; fx_held_l = x[0]; fx_held_r = x[1]; }
            x[0] = fx_held_l; x[1] = fx_held_r;
            if (m & FXB(FX_CRUSH)) { x[0] = (x[0] >> 9) * 512; x[1] = (x[1] >> 9) * 512; }
        }
        for (c = 0; c < 2; c++) {
            int32_t v = x[c];
            if (a_hp) { fx_lp1[c] += ((v - fx_lp1[c]) * a_hp) >> 14; v -= fx_lp1[c]; }
            if (a_lp < 16384) { fx_lp2[c] += ((v - fx_lp2[c]) * a_lp) >> 14; v = fx_lp2[c]; }
            if (fx_echo_tail) {                  /* the delay: what comes back, and what goes in while the pad is held */
                int32_t d = echo_buf[echo_pos][c], w = (m & FXB(FX_ECHO)) ? v : 0, nb = w + (d * 140 >> 8);
                nb = nb > 32767 ? 32767 : nb < -32767 ? -32767 : nb;
                echo_buf[echo_pos][c] = (int16_t)nb;
                v += d * 150 >> 8;
            }
            v = v * tg >> 8;
            out[2u * i + (uint32_t)c] = v;
        }
        if (fx_verb_tail) {                      /* the reverb: four combs and two allpasses on the mono mix, spread to both sides */
            int32_t in = (m & FXB(FX_VERB)) ? (out[2u * i] + out[2u * i + 1u]) >> 3 : 0, sum = 0, k, w;
            vb_dirty = 1;
            for (k = 0; k < VB_N; k++) {
                int32_t d = vb_comb[k][vb_pos[k]];
                vb_lp[k] += (d - vb_lp[k]) >> 2;                    /* the highs die first */
                w = in + (vb_lp[k] * 222 >> 8);
                vb_comb[k][vb_pos[k]] = (int16_t)(w > 32767 ? 32767 : w < -32767 ? -32767 : w);
                if (++vb_pos[k] >= VB_LEN[k]) vb_pos[k] = 0;
                sum += d;
            }
            for (k = 0; k < 2; k++) {
                int32_t d = vb_ap[k][ap_pos[k]];
                w = sum + (d >> 1);
                vb_ap[k][ap_pos[k]] = (int16_t)(w > 32767 ? 32767 : w < -32767 ? -32767 : w);
                sum = d - (w >> 1);
                if (++ap_pos[k] >= AP_LEN[k]) ap_pos[k] = 0;
            }
            sum = sum * 3 >> 1;
            out[2u * i] += sum; out[2u * i + 1u] += sum;
        }
        if (fx_echo_tail && ++echo_pos >= echo_d) echo_pos = 0;
    }
}
