/* SPDX-License-Identifier: GPL-3.0-only */
/* The brothers' Bluetooth link, as a unit of its own (not part of the app's unity build: its headers are felucca-bsp's). Linked into the app only. The
 * Bluetooth radio and link layer (stack/, chip/: not ours, see README.md) run in the radio's two interrupts (priority 2, below the audio and the tick).
 *
 * THE BROADCAST. No connection, no pairing, no MIDI: the leader advertises (non-connectable) a ten byte packet about every 20 ms, and the followers
 * listen. The leader writes the next packet into the radio's buffer right after each advertising event, so it holds the beat position as of the moment
 * it goes out. A follower timestamps each packet in the radio's receive interrupt, keeps a phase-locked estimate of the leader's tick count, and from
 * it makes the same clock, start and stop messages that MIDI would (bt_rt(), the app's), so each firmware's MIDI clock "follow" does the rest.
 *
 * The packet (manufacturer data, company id 0xFFFF = "for testing"), 18 bytes: 0xB8, seq, state (bit 0 running, bits 1..7 the start count), the tick
 * period in 2 us units (u16), the position in 1/256 ticks (u32), the pattern (bit 7: valid, 0..15), the kick mask (u16: the steps of the leader's pattern
 * with a kick, for the sidechain), the held punch-in effects (u16, one bit per brothers' effect: see FX below), the scale and root (low 4 bits root 0..11,
 * high 4 bits scale, 0 = none), the swing (0..96, 96 = full), a spare byte, a check byte. The position counts 24 ticks a quarter note from the leader's start; the beat
 * lines of a 16 step pattern are the ticks with (tick - 1) % 96 == 0.
 *
 * The app calls: brothers_set(mode) from the menu (0 off, 1 lead, 2 follow), brothers_poll() from the main loop, and as the leader (the audio
 * interrupt) brothers_lead_tick() at each clock tick and brothers_lead_run() at start and stop. The app supplies bt_rt(). */
#include <stdint.h>
#define FM1B_BLE 1
#define FM1B_BLE_OWN 1
#define BLE_HW_IRQ_PRIO 2u
#define BLE_DIAG 0
#define BLE_ADV_INTERVAL 32u                              /* 20 ms: the shortest */
#define BLE_SCAN_ACTIVE 0                                 /* listen only: no SCAN_REQ */
#define FM1_IRQ_NO_FAULT 1                                /* (firmware/ble/chip/fm1_irq.h: no crash record of its own) */
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "FM-1 Drumlab"                    /* (build.py: -DBLE_DEVICE_NAME=... per firmware) */
#endif
#define FM1B_BLE_CENTRAL 1                                /* every brother can listen (the stack's central side, level 3: no connecting out, no pairing) */
#define BLE_CENTRAL 3
#define RING_PUBLISH() __asm__ volatile("csync" ::: "memory")
#include "fm1_guard.h"
#include "fm1_time.h"

volatile uint8_t brothers_mode;                           /* 0 off, 1 lead, 2 follow */
volatile uint32_t brothers_state;                         /* 0 radio off, 1 radio up */
static volatile uint32_t lead_n, lead_ts, lead_per;       /* ticks so far, when the last one came (fm1_ticks), the period in fm1_ticks */
static volatile uint8_t lead_run, lead_start;
static volatile uint8_t lead_pat = 0xFFu, lead_scale, lead_swing;                   /* the leader's extras: sent in every packet (0xFF: no pattern) */
static volatile uint16_t lead_kick, lead_fx;
#define ETA_US 21500u                                     /* the packet goes out about this long after it is written (an interval + the advDelay's middle - the event's start) */
#define DEFAULT_PER (500000u)                             /* fm1_ticks of one tick at 120 BPM (24 MHz) */

static uint8_t brothers_adv_build(uint8_t *ad);
#include "ble_stack.c"

static inline uint32_t irq_save(void) { __asm__ volatile("cli" ::: "memory"); return 0; }
static inline void irq_restore(uint32_t f) { (void)f; __asm__ volatile("csync\n\tsti" ::: "memory"); }

uint32_t shim_ms(void) { return fm1_ticks() / FM1_TICKS_PER_US / 1000u; }
void shim_log(const char *s) { (void)s; }
BLE_API int ble_app_midi_peek(uint32_t *pkt, uint32_t *t_ms) { (void)pkt; (void)t_ms; return 0; }
BLE_API void ble_app_midi_pop(void) {}
BLE_API void ble_app_midi_in(uint32_t pkt, uint16_t ts, uint16_t last_ts) { (void)pkt; (void)ts; (void)last_ts; }
BLE_API void ble_app_state(void) {}
BLE_API void ble_app_bond(const uint8_t rand[8], uint16_t ediv, const uint8_t ltk[16]) { (void)rand; (void)ediv; (void)ltk; }
BLE_API void ble_app_peer_id(const uint8_t irk[16], const uint8_t addr[6], uint8_t addr_rand) { (void)irk; (void)addr; (void)addr_rand; }

extern void bt_rt(uint32_t st);                           /* the app: a clock (0xF8), start (0xFA) or stop (0xFC) from the link, to its MIDI follow */

/* ---- the leader (the audio interrupt and the radio's event interrupt) ---- */
/* off: this tick's sample within the audio block being made (the render runs once a block: the time of the call is the block's, the tick is off samples later);
 * per: the tick period in fm1_ticks (the sequencer knows the tempo exactly: 60,000,000 / BPM) */
void brothers_lead_tick(uint32_t off, uint32_t per)
{
    lead_ts = fm1_ticks() + off * 544u;                       /* 24 MHz / 44.1 kHz = 544.2 */
    lead_per = per;
    lead_n++;
}
void brothers_lead_run(uint32_t run)
{
    if (run) { lead_n = 0; lead_ts = fm1_ticks(); lead_start = (uint8_t)((lead_start + 1u) & 0x7Fu); }
    lead_run = (uint8_t)run;
}
/* what else the leader says (any context): the pattern now playing (0xFF: none), its kick steps, the punch-in effects held, scale (root | scale << 4), swing */
void brothers_lead_set(uint32_t pat, uint32_t kick, uint32_t fx, uint32_t scale, uint32_t swing)
{
    lead_pat = (uint8_t)pat; lead_kick = (uint16_t)kick; lead_fx = (uint16_t)fx; lead_scale = (uint8_t)scale; lead_swing = (uint8_t)swing;
}
static uint8_t brothers_adv_build(uint8_t *ad)            /* the advertising data, in the radio's event interrupt; 0: nothing (not leading) */
{
    static uint8_t seq;
    uint32_t per = lead_per ? lead_per : DEFAULT_PER, since, pos, i;
    int32_t s = (int32_t)(fm1_ticks() - lead_ts) + (int32_t)(ETA_US * FM1_TICKS_PER_US);              /* (the last tick may be a few samples ahead of now) */
    since = s < 0 ? 0u : (uint32_t)s;
    uint8_t *p = ad + 4, c = 0x5Au;
    if (brothers_mode != 1u)
        return 0;
    if (since > 8u * per) since = 8u * per;               /* (a stopped sequencer: whatever, run is 0; 8 periods x 256 fits 32 bits) */
    pos = lead_n * 256u + (since * 256u) / per;
    ad[0] = 21u; ad[1] = 0xFFu; ad[2] = 0xFFu; ad[3] = 0xFFu;
    p[0] = 0xB8u; p[1] = ++seq; p[2] = (uint8_t)(lead_run | lead_start << 1);
    p[3] = (uint8_t)(per / (2u * FM1_TICKS_PER_US)); p[4] = (uint8_t)(per / (2u * FM1_TICKS_PER_US) >> 8);
    p[5] = (uint8_t)pos; p[6] = (uint8_t)(pos >> 8); p[7] = (uint8_t)(pos >> 16); p[8] = (uint8_t)(pos >> 24);
    p[9] = lead_pat == 0xFFu ? 0u : (uint8_t)(0x80u | (lead_pat & 15u));
    p[10] = (uint8_t)lead_kick; p[11] = (uint8_t)(lead_kick >> 8);
    p[12] = (uint8_t)lead_fx; p[13] = (uint8_t)(lead_fx >> 8);
    p[14] = lead_scale; p[15] = lead_swing; p[16] = 0;
    for (i = 0; i < 17u; i++) c ^= p[i];
    p[17] = c;
    return 22u;
}

/* ---- the follower (the main loop) ---- */
static struct {
    uint8_t lock, run, start, addr[6], playing, pat, scale, swing;
    uint16_t kick, fx;
    uint32_t t_ref, pos_ref, per_us, emitted, last_rx;
} fol;
static uint32_t fol_pos(uint32_t t)                        /* the leader's position (1/256 ticks) at fm1_ticks time t */
{
    uint32_t dt_us = (t - fol.t_ref) / FM1_TICKS_PER_US;
    if (dt_us > 4000000u) dt_us = 4000000u;
    return fol.pos_ref + (dt_us * 256u) / fol.per_us;
}
static void fol_packet(const uint8_t *p, uint32_t ts)
{
    uint32_t per = ((uint32_t)p[3] | (uint32_t)p[4] << 8) * 2u, pos = (uint32_t)p[5] | (uint32_t)p[6] << 8 | (uint32_t)p[7] << 16 | (uint32_t)p[8] << 24;
    uint8_t run = p[2] & 1u, start = p[2] >> 1;
    if (per < 5000u || per > 120000u)
        return;
    ts -= 400u * FM1_TICKS_PER_US;                         /* (the interrupt's lateness after the packet: a guess, tuned by ear) */
    if (!fol.lock) {
        fol.lock = 1; fol.per_us = per; fol.pos_ref = pos; fol.t_ref = ts; fol.emitted = pos >> 8; fol.playing = 0; fol.start = start;
    } else {
        uint32_t est = fol_pos(ts);
        int32_t err = (int32_t)(pos - est);
        if (err > 3 * 256 || err < -3 * 256) { fol.pos_ref = pos; fol.emitted = pos >> 8; }                 /* a jump: the leader moved */
        else fol.pos_ref = est + (uint32_t)(err / 8);                                                       /* a nudge */
        fol.t_ref = ts;
        fol.per_us = (fol.per_us * 7u + per) / 8u;
    }
    fol.last_rx = fm1_ticks();
    fol.run = run;
    fol.pat = p[9]; fol.kick = (uint16_t)(p[10] | p[11] << 8); fol.fx = (uint16_t)(p[12] | p[13] << 8); fol.scale = p[14]; fol.swing = p[15];
    if (start != fol.start) { fol.start = start; if (fol.playing) { bt_rt(0xFCu); fol.playing = 0; } fol.emitted = pos >> 8; }   /* the leader started again: wait for the next bar */
}
static void fol_take(void)
{
    uint8_t pdu[40];
    uint16_t rssi;
    uint8_t n;
    while ((n = ble_ll_scan_take(pdu, &rssi)) != 0) {
        const uint8_t *d = pdu + 8;
        uint8_t c = 0x5Au, i;
        if ((pdu[0] & 0x0Fu) != 0x2u || n != 8u + 22u || d[0] != 21u || d[1] != 0xFFu || d[2] != 0xFFu || d[3] != 0xFFu || d[4] != 0xB8u)
            continue;
        for (i = 0; i < 17u; i++) c ^= d[4 + i];
        if (c != d[21])
            continue;
        if (!fol.lock) { for (i = 0; i < 6u; i++) fol.addr[i] = pdu[2 + i]; }              /* the first leader heard */
        else { for (i = 0; i < 6u && fol.addr[i] == pdu[2 + i]; i++) ; if (i < 6u) continue; }
        fol_packet(d + 4, ble_ll_scan_ts);
    }
}
static void fol_run(void)
{
    uint32_t now = fm1_ticks(), c, k = 0;
    if (!fol.lock)
        return;
    if (now - fol.last_rx > 1500u * 1000u * FM1_TICKS_PER_US) {                         /* the leader went quiet */
        if (fol.playing) { bt_rt(0xFCu); fol.playing = 0; }
        fol.lock = 0;
        return;
    }
    c = fol_pos(now) >> 8;
    if (fol.playing && !fol.run) { bt_rt(0xFCu); fol.playing = 0; }
    while ((int32_t)(c - fol.emitted) > 0 && k++ < 24u) {
        fol.emitted++;
        if (!fol.playing && fol.run && (fol.emitted - 1u) % 96u == 0u) { bt_rt(0xFAu); fol.playing = 1; }   /* joins at a bar line */
        if (fol.playing) bt_rt(0xF8u);
    }
    if ((int32_t)(c - fol.emitted) > 24)                                                 /* far behind (a long stall): skip */
        fol.emitted = c;
}

/* ---- the radio ---- */
static struct ble_rf_trims OWN_TRIMS_ = {{0x0B, 0x0B}, {1, 7, 4, 7, 11, 1, 7}, {0}, {0}};   /* the SDK board's values (crystal load, PA bias, Wi-Fi gains) */

static void radio_up(void)
{
    uint8_t addr[6];
    uint32_t f, r = fm1_ble_rng32();
    addr[0] = (uint8_t)r; addr[1] = (uint8_t)(r >> 8); addr[2] = (uint8_t)(r >> 16); addr[3] = (uint8_t)(r >> 24);
    r = fm1_ble_rng32();
    addr[4] = (uint8_t)r; addr[5] = (uint8_t)((r >> 8) | 0xC0u);       /* a static random address */
    fm1_ble_crumb_boot();                                               /* the breadcrumb (.noinit) from zero: the crash screen reads it */
    ble_init(addr, 1);
    f = irq_save();
    fm1_guard_unlock_top();
    fm1_ble_irq_attach(BLE_HW_IRQ_PRIO);                                /* the two vectors, masked until the radio runs */
    fm1_ble_irqs_hold(1);
    fm1_guard_lock_top();
    irq_restore(f);
    fm1_rf_cal_on = 1;                                                  /* the radio's own calibration at every start */
    f = irq_save();                                                     /* about 210 ms with none of our interrupts (audio, USB, timer): on a bench pair, with them on, the start-up hung or faulted
                                                                         * (a corrupted return, a bus fault) about 1 start in 20 on one unit and none in 270 with them off (2026-11-12) */
    ble_hw_wl82_start(&OWN_TRIMS_);
    irq_restore(f);
    fm1_ble_irqs_hold(0);
    brothers_state = 1;
}

void brothers_set(uint32_t mode)                                        /* the menu: 0 off, 1 lead, 2 follow */
{
    if (mode > 2u) mode = 0;
    if (mode && !brothers_state) radio_up();
    if (!brothers_state || mode == brothers_mode) { brothers_mode = (uint8_t)mode; return; }
    fm1_ble_irqs_hold(1);
    ble_ll_scan(0);
    ble_enable(0);
    brothers_mode = (uint8_t)mode;
    if (mode) {
        ble_enable(1);                                                  /* advertising (the leader's packets replace its data each event) */
        if (mode == 2u) ble_ll_scan(1);                                 /* a follower listens instead: scanning replaces advertising */
    }
    fm1_ble_irqs_hold(0);
    if (fol.playing) { bt_rt(0xFCu); fol.playing = 0; }
    fol.lock = 0;
}

void brothers_poll(void)                                                /* the main loop, often */
{
    if (brothers_mode == 2u) { fol_take(); fol_run(); }
}
/* what the leader said last (0 / 0xFF when not following) */
uint32_t brothers_pat(void) { return fol.lock && (fol.pat & 0x80u) ? (uint32_t)(fol.pat & 15u) : 0xFFu; }
uint32_t brothers_fx(void) { return fol.lock ? fol.fx : 0u; }
uint32_t brothers_scale(void) { return fol.lock ? fol.scale : 0u; }
uint32_t brothers_swing(void) { return fol.lock ? fol.swing : 0u; }
uint32_t brothers_kick(void) { return fol.lock ? fol.kick : 0u; }
/* THE SIDECHAIN: the volume to play at now (0..256, 256 = untouched). Every step of the leader's pattern that has a kick (the steps, from the position:
 * a bar is 96 ticks, a step 6) pulls the volume down to 256 - depth at once and lets it back up over 200 ms. Only while the leader is playing. */
uint32_t brothers_duck(uint32_t depth)
{
    uint32_t pos, c, step0, s, best = 256u;
    if (!fol.lock || !fol.run || !fol.kick || !fol.per_us)
        return 256u;
    pos = fol_pos(fm1_ticks());
    c = pos >> 8;
    if (c < 1u)
        return 256u;
    step0 = (c - 1u) / 6u;                                    /* the step that is playing */
    for (s = 0; s < 4u && s <= step0; s++) {
        uint32_t st = (step0 - s) & 15u;
        if (fol.kick >> st & 1u) {
            uint32_t age_ticks_q8 = pos - ((step0 - s) * 6u + 1u) * 256u;       /* since that step began, 1/256 ticks */
            uint32_t age_us = (age_ticks_q8 >> 4) * fol.per_us >> 4;            /* (x per_us / 256, in two shifts: no overflow) */
            if (age_us < 200000u) {
                uint32_t g = 256u - depth + depth * age_us / 200000u;
                if (g < best) best = g;
            }
        }
    }
    return best;
}
/* THE KEY. The leader works out the key of what it plays (brothers_detect: the lowest note is the root; a minor third beats a major third = minor) and says it
 * in every packet; a follower with Key on moves its notes to the nearest note of that key (brothers_snap). scale: root (0..11) | type << 4 (1 major, 2 minor; 0 none). */
uint32_t brothers_detect(const uint8_t *notes, uint32_t n)
{
    uint32_t pc[12] = {0}, i, low = 255u, root, distinct = 0;
    for (i = 0; i < n; i++) if (notes[i]) { pc[notes[i] % 12u]++; if (notes[i] < low) low = notes[i]; }
    for (i = 0; i < 12u; i++) if (pc[i]) distinct++;
    if (distinct < 3u || low == 255u)
        return 0u;
    root = low % 12u;
    return root | (pc[(root + 3u) % 12u] > pc[(root + 4u) % 12u] ? 2u : 1u) << 4;
}
uint32_t brothers_snap(uint32_t note, uint32_t scale)
{
    static const uint16_t SET[3] = {0xFFFu, 0xAB5u, 0x5ADu};  /* every note; major: 0 2 4 5 7 9 11; natural minor: 0 2 3 5 7 8 10 (bit n = the n-th semitone above the root) */
    uint32_t root = scale & 15u, type = scale >> 4 & 15u, d;
    if (!type || type > 2u)
        return note;
    for (d = 0; d < 12u; d++) {                                /* the nearest note in the key, a lower one first on a tie */
        uint32_t lo = note >= d ? note - d : 0u, hi = note + d;
        if (SET[type] >> ((lo + 12u - root) % 12u) & 1u) return lo;
        if (SET[type] >> ((hi + 12u - root) % 12u) & 1u) return hi;
    }
    return note;
}
/* the leader's tempo, BPM, as the last packet said (0: none) */
uint32_t brothers_bpm(void) { return fol.lock && fol.per_us ? (2500000u + fol.per_us / 2u) / fol.per_us : 0u; }
/* the crash screen (platform.c): where the radio was. o[0] mode | up << 4 | last start-up group << 8 | scan found << 16; o[1] breadcrumb;
 * o[2] interrupts since the radio went on; o[3] the last register touched; o[4] BBP | SPI timeouts << 16;
 * o[5] scan rc | trim rc << 8 | mid bank << 16; o[6] start-up ops */
void brothers_dbg(uint32_t *o)
{
    uint32_t i;
    if (!brothers_state) {                                              /* never started: the breadcrumb is RAM noise */
        for (i = 0; i < 8u; i++) o[i] = 0;
        o[0] = brothers_mode;
        return;
    }
    o[0] = (uint32_t)brothers_mode | (brothers_state ? 16u : 0u) | (uint32_t)fm1_ble_rf_stat.section << 8 | (uint32_t)fm1_ble_rf_stat.scan_found << 16;
    o[1] = fm1_ble_crumb;
    o[2] = fm1_ble_crumb_irqs;
    o[3] = fm1_ble_bc.addr;
    o[4] = fm1_ble_rf_stat.bbp_timeouts | fm1_ble_rf_stat.spi_timeouts << 16;
    o[5] = ((uint32_t)fm1_rfc_scan_rc & 0xFFu) | ((uint32_t)fm1_rfc_trim_rc & 0xFFu) << 8 | (uint32_t)fm1_rfc.mid << 16;
    o[6] = fm1_ble_rf_stat.ops;
    o[7] = 0;
}
/* what the menu shows: 0 nothing heard, 1 a leader heard (locked), 2 playing along */
uint32_t brothers_link(void) { return brothers_mode == 2u ? (fol.lock ? (fol.playing ? 2u : 1u) : 0u) : brothers_mode == 1u ? (lead_run ? 2u : 1u) : 0u; }
