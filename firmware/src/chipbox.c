/* SPDX-License-Identifier: GPL-3.0-only */
/* Chipbox: sound chips for the FM-1: NES, Game Boy and Master System voices you add as channels, a rack of steps, projects in flash.
 *
 *   PRESETS          down the channels (in the rack the last row adds one: OCT+ on it)
 *   ALGORITHM        rack: the page (past the last makes a new one)     chip screen: the knobs' page (tone / envelope / arp)
 *   SELECT           rack: the cursor along the steps
 *   KNOBS 1..4       chip screen: the sound of the channel; rack: knob 1 is the tempo
 *   keys             play the selected channel (and, in the rack, write it on the step under the cursor; the same note again clears it)
 *   EDIT hold        rack: remove the channel (OCT+ yes, OCT- no)
 *   HOME             tap: the next view (rack, chip, song); hold: the projects
 *   PLAY             the rack and the chip: the page round and round; the song screen: the song, page after page
 *   PLAY / SAVE / GLO   play stop, save, help       OCT- OCT+   octave (on their LEDs)       MASTER   volume
 *   USB-MIDI         CC 102 = play (64 and up) or stop, CC 103 = tempo / 2, CC 104 = save, CC 105 = open a project (0..7), CC 106 / 107 = add a channel of kind d2 / remove the last;
 *                    MIDI channel n plays channel n; program change picks the channel; the device sends CC 100 = CPU %
 *                    (worst block since boot) and CC 101 = open project | 16 if unsaved | 32 if loaded from flash
 *
 * Integers only: no float, no 64-bit division. tests/host.c runs this file on a computer. */
#include "platform.c"
#ifdef CHIPBOX_HOST
#define FM1_TICKS_PER_US 24u
static uint32_t fm1_ticks(void) { return 0; }
#endif
#include "chips.c"
#include "proj.c"

/* ---- one look per chip */
typedef struct {
    const char *name, *ch[4];
    uint16_t bg, bar, fg, acc, dim, key, lit;
} theme_t;
static const theme_t THEME[CHIPS] = {
    {"NES", {"PU1", "PU2", "TRI"},
     RGB(58, 58, 57), RGB(44, 44, 43), RGB(201, 199, 195), RGB(154, 42, 38), RGB(111, 100, 89), RGB(201, 199, 195), RGB(154, 42, 38)},       /* the console's own colours: charcoal, warm grey, brown-grey, red */
    {"GAME BOY", {"PU1", "PU2", "WAV"},
     RGB(15, 56, 15), RGB(48, 98, 48), RGB(155, 188, 15), RGB(155, 188, 15), RGB(110, 150, 30), RGB(139, 172, 15), RGB(15, 56, 15)},
    {"SMS", {"TN1", "TN2", "TN3"},
     RGB(8, 16, 60), RGB(22, 40, 110), RGB(240, 240, 255), RGB(70, 170, 255), RGB(70, 90, 150), RGB(220, 226, 245), RGB(70, 170, 255)},
};

static void text_on(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s, uint16_t c, uint16_t bg, int align)
{
    int32_t tw = text_w(f, s), tx = align == 1 ? ((int32_t)w - tw) / 2 : align == 2 ? (int32_t)w - tw : 0;
    cv_begin(w, f->h, bg);
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
}

/* ---- the sound: the chips, timed so the budget shows */
static volatile uint32_t cpu_max;               /* worst audio block so far, in ticks */
static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t t0 = fm1_ticks(), dt;
    chip_render(out, n);
    dt = (uint32_t)(fm1_ticks() - t0);
    if (dt > cpu_max) cpu_max = dt;
}
static int quiet(void)
{
    uint32_t c;
    for (c = 0; c < chan_n; c++)
        if (chip_level(c)) return 0;
    return 1;
}

/* ---- parameters under the knobs */
static uint32_t has_wave(uint32_t c) { return chan[c].kind == KD_GB_WAV; }
static uint32_t has_vol(uint32_t c) { return chan[c].kind != KD_NES_TRI; }                       /* the NES triangle has no level control */
static uint32_t has_duty(uint32_t c) { return chan[c].kind == KD_NES_PU || chan[c].kind == KD_GB_PU; }
static int8_t clamp_i(int32_t v, int32_t lo, int32_t hi) { return (int8_t)(v < lo ? lo : v > hi ? hi : v); }
static void knob_set(uint32_t c, uint32_t page, uint32_t k, int32_t d)      /* knob k of a page of cards, on channel c */
{
    inst_t *in = &chan[c].in;
    if (page == 3u) {                                           /* the wave: shape, fold, crush, twist */
        uint8_t *p = k == 0u ? &in->wave : k == 1u ? &in->wf : k == 2u ? &in->wc : &in->wt;
        *p = (uint8_t)clamp_i((int32_t)*p + d, 0, 15);
    } else if (page == 1u) {                                    /* the envelope: attack, decay, sustain, release */
        uint8_t *p = k == 0u ? &in->atk : k == 1u ? &in->dec : k == 2u ? &in->sus : &in->rel;
        *p = (uint8_t)clamp_i((int32_t)*p + d, 0, 15);
    } else if (page == 2u) {                                    /* the arp: chord, speed, direction */
        if (k == 0u) in->arp = (uint8_t)clamp_i((int32_t)in->arp + d, 0, 8);
        else if (k == 1u) in->arp_rate = (uint8_t)clamp_i((int32_t)in->arp_rate + d, 0, 3);
        else if (k == 2u) in->arp_dir = (uint8_t)clamp_i((int32_t)in->arp_dir + d, 0, 2);
    } else if (k == 0u) {                                       /* the tone: wave or duty, volume, vibrato, sweep or pan */
        int32_t v;
        if (has_wave(c)) { v = ((int32_t)in->wave + 2) / 5 + (d < 0 ? -1 : 1); in->wave = (uint8_t)((v < 0 ? 0 : v > 3 ? 3 : v) * 5); }       /* pulse, saw, triangle, sine */
        else if (has_duty(c)) { v = (int32_t)in->duty + d; in->duty = (uint8_t)(v < 0 ? 0 : v > 3 ? 3 : v); }
    }
    else if (k == 1u) in->vol = (uint8_t)clamp_i((int32_t)in->vol + d, 0, 15);
    else if (k == 2u) in->vib_dep = (uint8_t)clamp_i((int32_t)in->vib_dep + d, 0, 15);
    else if (KIND_CHIP[chan[c].kind] == CHIP_GB) in->pan = (uint8_t)clamp_i((int32_t)in->pan + d, 0, 2);
    else in->sweep = clamp_i((int32_t)in->sweep + d, -16, 16);
}

/* ---- the screen */
static const uint8_t AN_COUNT[CHIPS] = {3, 2, 1};
static void draw_header(const theme_t *t, uint32_t chip)
{
    (void)chip;
    lcd_fill(0, 0, 240, 240, t->bg);                /* (the top bar is header.c's) */
}
/* the waveform a channel plays, 2 periods in w x h */
enum { G_PULSE, G_TRI, G_SAW, G_SINE };
static void wave_glyph(uint32_t kind, uint32_t d8, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    static const int8_t SIN8[8] = {0, 71, 100, 71, 0, -71, -100, -71};
    int32_t p = w / 2, i, k, lo = y + h - 2, hi = lo - (h - 2), px = x, py = lo, nx, ny;
    for (k = 0; k < 2; k++) {
        int32_t x0 = x + k * p;
        if (kind == G_PULSE) {
            int32_t on = p * (int32_t)d8 / 8;
            cv_rect(x0, hi, 2, h, c); cv_rect(x0, hi, on, 2, c);
            cv_rect(x0 + on, hi, 2, h, c); cv_rect(x0 + on, lo, p - on, 2, c);
        } else {
            for (i = 0; i <= 8; i++) {
                nx = x0 + i * p / 8;
                ny = kind == G_TRI ? lo - (i <= 4 ? i : 8 - i) * (h - 2) / 4
                   : kind == G_SAW ? lo - (i == 8 ? 0 : i * (h - 2) / 8)
                   : y + h / 2 - SIN8[i & 7] * (h - 2) / 200;
                if (i) { cv_line(px, py, nx, ny, c); cv_line(px, py + 1, nx, ny + 1, c); }
                px = nx; py = ny;
            }
        }
    }
}
static void channel_glyph(uint32_t c, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t col)
{
    static const uint8_t D8[4] = {1, 2, 4, 6};
    const inst_t *in = &chan[c].in;
    if (chan[c].kind == KD_GB_WAV) {
        uint8_t wv[32];
        int32_t i;
        wave_make(in, wv);                                      /* the wave itself, as it is now */
        for (i = 0; i < 32; i++) cv_rect(x + i * w / 32, y + h - 2 - wv[i] * (h - 2) / 15, w / 32 + 1, 2, col);
    } else if (chan[c].kind == KD_NES_TRI) wave_glyph(G_TRI, 0, x, y, w, h, col);
    else wave_glyph(G_PULSE, has_duty(c) ? D8[in->duty & 3u] : 4u, x, y, w, h, col);
}
#define KEY0 53u                                /* key 0 = F3 */
static void note_name(char *s, uint32_t note)   /* "C#4"; blank for none (0 or 255) */
{
    static const char *const NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    s[0] = 0;
    if (note && note < 128u) {
        const char *p = NAME[note % 12u];
        uint32_t i = 0;
        while (*p) s[i++] = *p++;
        if (note >= 12u) s[i++] = (char)('0' + note / 12u - 1u);
        s[i] = 0;
    }
}
static void draw_note(const theme_t *t, uint32_t note)
{
    char s[8];
    note_name(s, note);
    text_on(160, 6, 72, &FONT_L, s, t->fg, t->bg, 2);
}
#include "rack.c"
#include "header.c"
#include "songview.c"
#include "stepview.c"
#include "fxview.c"
#include "flair.c"
#include "chipview.c"
#include "menu.c"
#include "picker.c"

static int32_t enc_act(uint32_t e) { int32_t v = fm1_enc_take(e); if (v) act_ms = fm1_ms; return v; }       /* knobs count as being touched */
#define fm1_enc_take enc_act

static void app_main(void)
{
    struct { int8_t oct; uint8_t an[CHIPS], proj, demo, unlocked; } set = {0, {AN_HW, AN_HW, 0}, 0, 0, 0};
    uint32_t prev = 0, k, t_change = 0, dirty = 0, note_drawn = ~0u, chip_drawn = ~0u;
    uint32_t last_note = 255u, view = 1, rack_force = 1, chip_page = 0, sel = 0;     /* view: 0 the chip, 1 the rack, 2 the song; sel: the channel (in the rack, one past the last is "add") */
    uint32_t home_was = 0, home_t0 = 0, home_used = 0, home_now, home_tap, edit_was = 0, edit_t0 = 0, edit_now, edit_used = 0, edit_rel, edit_clr = 255u;
    uint32_t btn_was = 0, crit_next = 0, crit_n = 0, sys_req = 0;
    int32_t ed_hold_k = -1;                             /* STEP: the key held now and where its note went: KNOB 2 sets that note's length */
    uint32_t ed_hold_step = 0;
    uint8_t held[27] = {0}, held_ch[27] = {0}, ch_drawn = 99;
    uint8_t rec_hold[27] = {0}, rec_pg[27] = {0}, rec_st[27] = {0}, rec_ch[27] = {0};     /* a recorded note still held: where it went, to set its length when the key comes up */
    uint32_t rec_t0[27] = {0};
    settings_load(&set, sizeof set);
    if (set.demo < DEMO_N) {                            /* the demos a device has not got yet go in, what was there moves down */
        uint32_t d;
        for (d = set.demo; d < DEMO_N; d++) { proj_shuffle_down(d); proj_demo_write(d); }
        set.demo = DEMO_N; set.proj = 0; dirty = 1; t_change = fm1_ms;
    }
    act_ms = fm1_ms;
    proj_cur = set.proj < PROJ_SLOTS ? set.proj : 0;                  /* the project you left open; Korobeiniki on a new device */
    proj_loaded = proj_load(proj_cur) == 0;
    if (!proj_loaded) { if (proj_cur == 0u) { seq_tetris(); proj_edited(); } else proj_blank(); }
    if (set.oct < -2 || set.oct > 2) set.oct = 0;
    for (k = 0; k < CHIPS; k++)
        if (set.an[k] >= AN_COUNT[k]) set.an[k] = AN_COUNT[k] > 1u ? 1u : 0u;
    set.an[CHIP_NES] = set.an[CHIP_GB] = AN_HW;     /* always the analog stage */
    for (k = 0; k < CHIPS; k++) analog_mode[k] = set.an[k];

    for (;;) {
        static uint32_t cc_ms;
        const theme_t *t;
        uint32_t keys, down, up, rel, b, lit = 0, base = (uint32_t)(KEY0 + 12 * set.oct), cpu, redraw = 0, in_menu = menu_on, nrows, ch_chip;
        uint8_t st, d1, d2;
        int32_t e;
        platform_poll();
        if (!chan_n && !view) { view = 1; rack_force = 1; chip_drawn = ~0u; }       /* no channels: only the rack makes sense */
        if (rec_on && (view == 2u || view == 3u || !seq_run || sel >= chan_n || pick_on)) { rec_on = 0; duck_ch = 255; }     /* recording stops with the playing, the song screen, the + row */
        if (view == 3u) rack_cur = ed_cur;
        else if (!seq_run || seq_hold) rack_cur = seq_step;                  /* stopped or paused: the playhead is the cursor */
        nrows = chan_n + (view == 1u && chan_n < MAXCH ? 1u : 0u);
        if (sel >= nrows) sel = nrows ? nrows - 1u : 0u;
        if (sel < chan_n && chip_page >= (has_wave(sel) ? 4u : 3u)) { chip_page = 0; redraw = 1; }
        ch_chip = sel < chan_n ? KIND_CHIP[chan[sel].kind] : CHIP_NES;
        t = &THEME[ch_chip];

        keys = fm1_in.notes;
        if (keys != prev || fm1_in.buttons != btn_was) act_ms = fm1_ms;
        btn_was = fm1_in.buttons;
        down = keys & ~prev;
        up = prev & ~keys;
        prev = keys;
        if (fx_layer && (menu_on || pick_on)) fx_off();
        if (fx_layer) {                                 /* the punch-in layer: the white keys are the pads, held = on; they play nothing else */
            uint32_t mask = 0, p, now;
            if (!menu_on && !pick_on) {
                uint32_t edge;
                for (p = 0; p < FXN; p++) if (keys >> FX_KEY[p] & 1u) mask |= FXB(p);
                edge = mask & ~fx_raw; fx_raw = mask;
                if (edge & FXB(FX_LATCH)) { fx_lat ^= 1u; if (!fx_lat) fx_tog = 0; }       /* LATCH: pads stay on when tapped, tap again to turn off */
                if (fx_lat) { fx_tog ^= edge & ~FXB(FX_LATCH); mask = fx_tog | FXB(FX_LATCH); }
                else mask &= ~FXB(FX_LATCH);
                now = mask & ~fx_held;
                for (p = 0; p < FXN; p++) if (now >> p & 1u) { if (p == FX_TAPE) anim_tape(); else anim_fx(p); }
                if (!mask && fx_held) { /* all let go: the animation finishes by itself */ }
                else if (mask && (fx_held & ~mask)) { for (p = 0; p < FXN; p++) if (mask >> p & 1u) { an_arg = p; break; } }
                fx_apply(mask);
            }
            down = up = 0;
        }
        for (k = 0; k < 27u; k++) {
            if (down >> k & 1u) {
                held[k] = (uint8_t)(base + k); held_ch[k] = (uint8_t)sel;
                if (sel < chan_n) {
                    chip_note(sel, held[k], 127);
                    last_note = held[k]; midi_send(0x90 | (sel & 15u), held[k], 100);
                    if (!menu_on && !pick_on && !rack_rm) {
                        if (rec_on && seq_run && !seq_hold && view != 2u) {      /* recording: the note goes on the step nearest the playhead */
                            uint32_t a, sb, stp = 0, pg;
                            do { a = seq_step; sb = seq_sub; } while (a != seq_step);
                            stp = (a * SEQ_PPS + sb + SEQ_PPS / 2u) / SEQ_PPS % SEQ_LEN; pg = seq_pat;
                            {   step_t *x = &pat[pg][sel][stp];
                                x->note = held[k]; x->vel = 100; x->gate = 5; rec_hold[k] = 1; rec_t0[k] = seq_ticks; rec_pg[k] = (uint8_t)pg; rec_st[k] = (uint8_t)stp; rec_ch[k] = (uint8_t)sel;
                            }
                            rack_inval = 1; proj_edited();
                        } else if (view == 1u && (!seq_run || seq_hold)) { seq_toggle(sel, rack_cur, held[k]); rack_inval = 1; proj_edited(); }
                        else if (view == 3u) { step_t *x = &pat[seq_view][sel][ed_cur]; x->note = held[k]; x->vel = 100; x->gate = 5; ed_hold_k = (int32_t)k; ed_hold_step = ed_cur; ed_cur = (ed_cur + 1u) % SEQ_LEN; proj_edited(); }       /* STEP: the key sets the block and moves on */       /* stopped or paused: on the step the playhead is at */
                    }
                }
            }
            if (up >> k & 1u) {
                if ((int32_t)k == ed_hold_k) ed_hold_k = -1;
                if (held_ch[k] < chan_n && last_note == held[k]) chip_off(held_ch[k]);
                midi_send(0x80 | (held_ch[k] & 15u), held[k], 0);
                if (rec_hold[k]) {                      /* a recorded note: its length is how long the key was down */
                    step_t *x = &pat[rec_pg[k]][rec_ch[k]][rec_st[k]];
                    uint32_t g = seq_ticks - rec_t0[k];
                    if (x->note == held[k]) x->gate = (uint8_t)(g < 1u ? 1u : g > 96u ? 96u : g);
                    rec_hold[k] = 0; rack_inval = 1;
                }
            }
        }

        b = fm1_input_edges(&rel);
        home_now = fm1_in.buttons >> BTN_HOME & 1u;     /* HOME: a tap goes between the views, a hold opens the projects */
        if (home_now && !home_was) { home_t0 = fm1_ms; home_used = 0; }
        sys_req = 0;                                    /* GLO + HOME, either one first: the menu */
        if (!pick_on && home_now && (((b & (1u << BTN_GLO)) != 0u) || (!home_was && (fm1_in.buttons >> BTN_GLO & 1u)))) { home_used = 1; sys_req = 1; b &= ~(1u << BTN_GLO); }
        if (home_now && !home_used && !menu_on && !pick_on && fm1_ms - home_t0 >= 600u) { pick_open(); home_used = 1; }
        home_tap = !home_now && home_was && !home_used;
        home_was = home_now;
        edit_now = fm1_in.buttons >> BTN_EDIT & 1u;
        edit_rel = !edit_now && edit_was;
        if (edit_now && !edit_was) { edit_t0 = fm1_ms; edit_used = 0; }
        edit_was = edit_now;
        if (pick_on) {                                  /* the projects: SELECT moves, OCT+ yes, OCT- no, hold EDIT to clear one */
            e = fm1_enc_take(ENC_SELECT);
            if (!e) e = fm1_enc_take(ENC_PRESETS);
            if (e) { pick_sel = (pick_sel + (e < 0 ? PROJ_SLOTS - 1u : 1u)) % PROJ_SLOTS; pick_del = 0; pick_dirty = 1; }
            if (edit_now && fm1_ms - edit_t0 >= 600u && !pick_del && pick_have[pick_sel] && (pick_sel >= DEMO_N || set.unlocked)) { pick_del = 1; pick_dirty = 1; }
            if ((b & (1u << BTN_OCT_UP)) || (home_tap && !pick_del)) {       /* OCT+ (yes) or a HOME tap opens the slot under the frame */
                if (pick_del) {
                    proj_clear(pick_sel);
                    if (pick_sel == proj_cur) { proj_blank(); proj_dirty = 0; sel = 0; }
                    pick_del = 0; pick_scan(); pick_dirty = 1;
                } else {
                    if (pick_sel != proj_cur) {
                        proj_open(pick_sel); anim_start(AN_OPEN, pick_sel);
                        set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; last_note = 255u; sel = 0; rack_rm = 0;
                    }
                    pick_on = 0; rack_force = 1; chip_drawn = ~0u;
                }
            } else if (b & (1u << BTN_OCT_DOWN)) {
                if (pick_del) { pick_del = 0; pick_dirty = 1; } else { pick_on = 0; rack_force = 1; chip_drawn = ~0u; }
            } else if ((b & (1u << BTN_GLO)) || home_tap) { pick_del = 0; pick_on = 0; rack_force = 1; chip_drawn = ~0u; }
            fm1_enc_take(ENC_PRESETS); fm1_enc_take(ENC_ALGORITHM);
            for (k = 0; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else {
        if ((b & (1u << BTN_SEQ)) && !menu_on && !pick_on && !fx_layer) {      /* SEQ: step entry for the channel you are on, and back */
            if (view == 3u) { view = 1; anim_start(AN_VIEW, 1u); rack_force = 1; chip_drawn = ~0u; }
            else if (view != 2u && sel < chan_n) { chip_off(sel); view = 3; ed_cur = 0; anim_start(AN_VIEW, 4u); rack_force = 1; chip_drawn = ~0u; }
        }
        if (sys_req) {                                  /* the menu (it replaces a help list that is open) */
            SYSM[2].text = set.unlocked ? "Lock Demos" : "Unlock Demos";
            menu_open_pick(SYSM, sizeof SYSM / sizeof SYSM[0]); menu_kind = MK_MENU;
        } else if (b & (1u << BTN_GLO)) {               /* help for the view you are on; GLO or HOME again goes back */
            if (menu_on) { menu_on = 0; rack_force = 1; chip_drawn = ~0u; }
            else if (fx_layer) { menu_open(HELP_FX, sizeof HELP_FX / sizeof HELP_FX[0]); menu_kind = MK_HELP; }
            else if (view == 2u) { menu_open(HELP_SONG, sizeof HELP_SONG / sizeof HELP_SONG[0]); menu_kind = MK_HELP; }
            else if (view == 3u) { menu_open(HELP_STEP, sizeof HELP_STEP / sizeof HELP_STEP[0]); menu_kind = MK_HELP; }
            else if (view) { menu_open(HELP_RACK, sizeof HELP_RACK / sizeof HELP_RACK[0]); menu_kind = MK_HELP; }
            else { menu_open(HELP_CHIP, sizeof HELP_CHIP / sizeof HELP_CHIP[0]); menu_kind = MK_HELP; }
        } else if (home_tap) {
            if (menu_on) menu_on = 0;
            else if (fx_layer) { fx_layer = 0; fx_off(); anim_start(AN_VIEW, view); }       /* HOME leaves the punch-in layer first */
            else if (view == 3u) { view = 1; anim_start(AN_VIEW, 1u); }
            else { view = view == 1u ? (chan_n ? 0u : 2u) : view == 0u ? 2u : 1u; anim_start(AN_VIEW, view); }       /* HOME: the rack, the chip, the song, the rack ... */
            rack_force = 1; chip_drawn = ~0u;
        }
        if (b & (1u << BTN_PLAY)) {                     /* PLAY: in the song, the song from the cursor; anywhere else, the page on show round and round */
            if (seq_run && seq_hold) { seq_rate = 2; seq_hold = 0; anim_start(AN_PLAY, 0); }                   /* paused on the tape: go on */
            else if (seq_run) { seq_stop(); rec_on = 0; duck_ch = 255; anim_start(AN_PAUSE, 0); }
            else { if (view == 2u) seq_play(song_cur < song_len ? song_cur : 0u, 0); else seq_play(0, 1); anim_start(AN_PLAY, 0); }
        }
        if ((b & (1u << BTN_FX)) && !menu_on) {          /* FX: the punch-in layer on and off */
            fx_layer ^= 1u;
            anim_start(AN_VIEW, fx_layer ? 3u : view);
            if (fx_layer) { for (k = 0; k < 27u; k++) if (prev >> k & 1u && held_ch[k] < chan_n) chip_off(held_ch[k]); }
            else fx_off();
            rack_force = 1; chip_drawn = ~0u;
        }
        if ((b & (1u << BTN_REC)) && !menu_on && view != 2u && view != 3u && sel < chan_n) {      /* REC: arm (and start the page round and round); again: disarm */
            if (rec_on) { rec_on = 0; duck_ch = 255; seq_stop(); anim_start(AN_REC, 1); }                  /* REC again: stop recording and the playing */
            else { rec_on = 1; duck_ch = (uint8_t)sel; if (!seq_run) seq_play(0, 1); anim_start(AN_REC, 0); }
        }
        if (!edit_now) edit_clr = 255u;
        if (rec_on && edit_now && seq_run && sel < chan_n) {      /* recording, EDIT held: the note at the playhead goes, and each one the playhead reaches while it is held */
            uint32_t cs = seq_step;
            if (cs != edit_clr) {
                step_t *x = &pat[seq_pat][sel][cs];
                edit_clr = cs;
                if (x->note) { x->note = 0; rack_inval = 1; proj_edited(); }
            }
        }
        if ((b & (1u << BTN_LFO)) && !menu_on && !pick_on) {          /* LFO: undo */
            uint32_t vv = seq_view, sc = song_cur;
            if (undo_go() == 0) {
                if (vv < page_n) seq_view = (uint8_t)vv;
                if (sc <= song_len) song_cur = sc;
                rack_inval = 1; rack_rm = 0; chip_drawn = ~0u; redraw = 1; anim_start(AN_UNDO, 1);
            } else anim_start(AN_UNDO, 0);
        }
        undo_tick();
        if (b & (1u << BTN_SAVE)) { if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; anim_start(AN_SAVE, 0); } }
        if (in_menu) {                                  /* in a menu only SELECT does anything (and, in a pick list, OCT+ yes, OCT- no) */
            e = fm1_enc_take(ENC_SELECT);
            if (e) menu_scroll(e);
            e = fm1_enc_take(ENC_PRESETS);
            if (e) menu_scroll(e);
            if (menu_on && (b & (1u << BTN_OCT_UP))) {
                if (menu_kind == MK_ADD) {
                    int c = chan_add(menu_sel - 1u);
                    if (c >= 0) { sel = (uint32_t)c; proj_edited(); anim_start(AN_ADD, KIND_CHIP[chan[c].kind] | (uint32_t)c << 8); }
                    menu_on = 0; rack_force = 1; chip_drawn = ~0u;
                } else if (menu_kind == MK_MENU) {
                    if (menu_sel == 1u) { menu_open(CONF_DEMO, sizeof CONF_DEMO / sizeof CONF_DEMO[0]); menu_kind = MK_DEMO; }
                    else if (menu_sel == 3u) { menu_open(CONF_FACTORY, sizeof CONF_FACTORY / sizeof CONF_FACTORY[0]); menu_kind = MK_FACTORY; }
                    else { set.unlocked ^= 1u; dirty = 1; t_change = fm1_ms; menu_on = 0; rack_force = 1; chip_drawn = ~0u; anim_start(AN_SAVE, 0); }
                } else if (menu_kind == MK_DEMO || menu_kind == MK_FACTORY) {
                    uint32_t d;
                    seq_stop(); rec_on = 0; duck_ch = 255;
                    if (menu_kind == MK_FACTORY) {              /* every project goes, the settings start again */
                        for (k = 0; k < PROJ_SLOTS; k++) proj_clear(k);
                        set.oct = 0; set.unlocked = 0;
                    } else if (proj_dirty) proj_save(proj_cur);
                    for (d = DEMO_N; d > 0u; d--) proj_demo_write(d - 1u);        /* the first goes last: it is the one left in memory */
                    proj_cur = 0; proj_dirty = 0; proj_loaded = 1; undo_reset();
                    set.proj = 0; set.demo = DEMO_N; dirty = 1; t_change = fm1_ms;
                    sel = 0; rack_rm = 0; last_note = 255u; view = 1;
                    menu_on = 0; rack_force = 1; chip_drawn = ~0u; anim_start(AN_OPEN, 0);
                }
            } else if (menu_on && (b & (1u << BTN_OCT_DOWN)) && menu_kind != MK_HELP) { menu_on = 0; rack_force = 1; chip_drawn = ~0u; }
            fm1_enc_take(ENC_ALGORITHM);
            for (k = 0; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else if (view == 2u) {                        /* the song: SELECT moves (and plays the page once), PRESETS changes the slot's page, OCT+ a new blank page, OCT- removes the slot, EDIT opens the page in the rack */
            if (song_cur > song_len) song_cur = song_len;
            e = fm1_enc_take(ENC_SELECT);
            if (e) {
                song_cur = (song_cur + (e < 0 ? song_len : 1u)) % (song_len + 1u); rack_inval = 1;
                if (song_cur < song_len) { seq_view = song[song_cur]; if (!seq_run || seq_loop == 2u) { seq_stop(); seq_play(0, 2); } }
            }
            e = fm1_enc_take(ENC_PRESETS);
            if (e && song_cur < song_len) {
                song_cycle(song_cur, e); rack_inval = 1; proj_edited();
                if (!seq_run || seq_loop == 2u) { seq_stop(); seq_play(0, 2); }
            }
            if (b & (1u << BTN_OCT_UP)) {               /* a new slot with a new blank page */
                uint32_t pn = page_n;
                song_insert_blank(song_cur);
                if (page_n != pn) { song_cur = song_cur < song_len ? (song_cur + 1u < song_len ? song_cur + 1u : song_len - 1u) : song_len - 1u; rack_inval = 1; proj_edited(); anim_start(AN_COPY, (uint32_t)seq_view + 1u); }
            }
            if ((b & (1u << BTN_OCT_DOWN)) && song_len > 1u && song_cur < song_len) {
                uint32_t was = song[song_cur];
                song_remove(song_cur); rack_inval = 1; proj_edited();
                if (song_cur >= song_len) song_cur = song_len - 1u;
                anim_start(AN_PAGE, was + 1u);
            }
            if (edit_now && !edit_used && song_cur < song_len && fm1_ms - edit_t0 >= 600u) {      /* hold EDIT: a copy of this slot's page, in this slot */
                int np = seq_dup_page(song[song_cur]);
                if (np >= 0) { song[song_cur] = (uint8_t)np; seq_view = (uint8_t)np; rack_inval = 1; proj_edited(); anim_start(AN_COPY, (uint32_t)np + 1u); }
                edit_used = 1;
            }
            if (edit_rel && !edit_used && song_cur < song_len) {                                  /* a tap on EDIT: this page, in the rack */
                seq_view = song[song_cur]; view = 1; rack_force = 1; chip_drawn = ~0u;
            }
            e = fm1_enc_take(ENC_KNOB1);                /* knob 1: the tempo */
            if (e) { int32_t v = (int32_t)seq_bpm + e; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); proj_edited(); anim_tempo(); }
            fm1_enc_take(ENC_ALGORITHM);
            for (k = 1; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else if (view == 3u) {                        /* STEP: SELECT moves the cursor, PRESETS the channel, ALGORITHM the page, EDIT clears the block, the octave buttons as everywhere */
            uint32_t oct_was = (uint32_t)(set.oct + 2);
            e = fm1_enc_take(ENC_SELECT);
            if (e) ed_cur = (ed_cur + (e < 0 ? SEQ_LEN - 1u : 1u)) % SEQ_LEN;
            e = fm1_enc_take(ENC_PRESETS);
            if (e && chan_n) { chip_off(sel); sel = (sel + (e < 0 ? chan_n - 1u : 1u)) % chan_n; anim_start(AN_CHAN, sel | (uint32_t)KIND_CHIP[chan[sel].kind] << 8); }
            e = fm1_enc_take(ENC_ALGORITHM);
            if (e) seq_page(e);
            e = fm1_enc_take(ENC_KNOB1);
            if (e) { int32_t v = (int32_t)seq_bpm + e; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); proj_edited(); anim_tempo(); }
            e = fm1_enc_take(ENC_KNOB2);                /* KNOB 2: how long the note is, in steps: the one you are holding, or the one under the cursor */
            if (e && sel < chan_n) {
                uint32_t st = ed_hold_k >= 0 ? ed_hold_step : ed_cur;
                step_t *x = &pat[seq_view][sel][st];
                if (x->note) {
                    int32_t n = ((int32_t)x->gate + 1) / 6 + e, mx = (int32_t)(SEQ_LEN - st);
                    n = n < 1 ? 1 : n > mx ? mx : n;
                    x->gate = (uint8_t)(n * 6 - 1); proj_edited();
                }
            }
            for (k = 2; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
            if ((b & (1u << BTN_EDIT)) && sel < chan_n) { step_t *x = &pat[seq_view][sel][ed_cur]; if (x->note) { x->note = 0; proj_edited(); } }
            if (b & (1u << BTN_OCT_DOWN) && set.oct > -2) { set.oct--; dirty = 1; t_change = fm1_ms; }
            if (b & (1u << BTN_OCT_UP) && set.oct < 2) { set.oct++; dirty = 1; t_change = fm1_ms; }
            if ((uint32_t)(set.oct + 2) != oct_was) anim_cont(AN_OCT, oct_was << 4 | (uint32_t)(set.oct + 2), 900u);
        } else {
        if (view == 1u && sel == chan_n && chan_n < MAXCH && (b & (1u << BTN_OCT_UP))) {                 /* OCT+ (yes) on the "+" row: add a channel */
            menu_open_pick(ADD_MENU, sizeof ADD_MENU / sizeof ADD_MENU[0]); menu_kind = MK_ADD;
        } else if (rack_rm) {                           /* removing a channel: OCT+ yes, OCT- no */
            if (b & (1u << BTN_OCT_UP)) { uint32_t gone = KIND_CHIP[chan[sel].kind]; seq_stop(); chan_remove(sel); proj_edited(); rack_rm = 0; rack_inval = 1; anim_start(AN_REMOVE, gone | (uint32_t)sel << 8); }
            else if (b & (1u << BTN_OCT_DOWN)) { rack_rm = 0; rack_inval = 1; }
        } else {
            uint32_t oct_was = (uint32_t)(set.oct + 2);
            if (b & (1u << BTN_OCT_DOWN) && set.oct > -2) { set.oct--; dirty = 1; t_change = fm1_ms; }
            if (b & (1u << BTN_OCT_UP) && set.oct < 2) { set.oct++; dirty = 1; t_change = fm1_ms; }
            if ((uint32_t)(set.oct + 2) != oct_was) anim_cont(AN_OCT, oct_was << 4 | (uint32_t)(set.oct + 2), 900u);
        }
        if (view == 1u && sel < chan_n && edit_now && !edit_used && !rack_rm && !rec_on && fm1_ms - edit_t0 >= 600u) { rack_rm = 1; rack_inval = 1; edit_used = 1; anim_start(AN_DELETE, 0); }     /* hold EDIT on a channel: remove it? */
        e = fm1_enc_take(ENC_PRESETS);                  /* always: down the list of channels */
        if (e && nrows) {
            if (sel < chan_n) chip_off(sel);
            sel = (sel + (e < 0 ? nrows - 1u : 1u)) % nrows;
            rack_rm = 0; rack_inval = 1; redraw = 1; last_note = 255u;
            if (sel < chan_n && !fx_layer) anim_start(AN_CHAN, sel | (uint32_t)KIND_CHIP[chan[sel].kind] << 8);
            if (rec_on) duck_ch = (uint8_t)(sel < chan_n ? sel : 255u);
        }
        e = fm1_enc_take(ENC_ALGORITHM);                /* the rack: the page; the chip screen: the knobs' page */
        if (e) {
            if (view == 1u) {
                uint32_t pn = page_n, was = seq_view;
                seq_page(e); rack_inval = 1;
                if (page_n != pn) proj_edited();
                if (seq_view != was) anim_start(AN_PAGE, (was + 1u) | (e < 0 ? 256u : 0u));
            }
            else if (sel < chan_n) { uint32_t np = has_wave(sel) ? 4u : 3u; chip_page = (chip_page + (e < 0 ? np - 1u : 1u)) % np; redraw = 1; }
        }
        e = fm1_enc_take(ENC_SELECT);                   /* the tape dial: middle pauses, right goes on, left goes back; in the punch-in layer only while TAPE is held */
        if (e && (!fx_layer || (fx_held & FXB(FX_TAPE)))) { seq_shuttle(e); anim_tape(); rack_inval = 1; }
        for (k = 0; k < 4u; k++) {
            e = fm1_enc_take(ENC_KNOB1 + k);
            if (!e) continue;
            if (view == 1u) {
                if (k == 0u) { int32_t v = (int32_t)seq_bpm + e; seq_bpm = (uint32_t)(v < 40 ? 40 : v > 240 ? 240 : v); rack_inval = 1; proj_edited(); anim_tempo(); }     /* knob 1: the tempo */
            } else if (sel < chan_n) {
                knob_set(sel, chip_page, k, e); redraw = 1; proj_edited();
                if (chip_page == 3u) anim_cont(AN_WAVE, (uint32_t)sel, 1200u);
            }
        }
        }
        }
        while (midi_get(&st, &d1, &d2)) {
            uint32_t mt = st & 0xFu;                         /* MIDI channel n plays channel n */
            if ((st & 0xF0u) == 0xC0u) {                    /* program change: pick the channel */
                if (chan_n) { if (sel < chan_n) chip_off(sel); sel = d1 % chan_n; last_note = 255u; rack_inval = 1; }
                continue;
            }
            if ((st & 0xF0u) == 0xB0u && d1 == 102u) { if (d2 >= 64u) seq_play(0, 0); else if (seq_run) seq_stop(); continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 103u) { seq_bpm = d2 * 2u; proj_edited(); continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 104u) { if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; } continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 108u) { fx_apply(d2 == 0u ? 0u : d2 <= FXN ? fx_held | FXB(d2 - 1u) : d2 <= 2u * FXN ? fx_held & ~FXB(d2 - 1u - FXN) : fx_held); if (d2 && d2 <= FXN) anim_fx(d2 - 1u); continue; }     /* test: pad d2-1 on; d2 17..32: pad off; 0: all off */
            if ((st & 0xF0u) == 0xB0u && d1 == 106u) { if (chan_add(d2) >= 0) proj_edited(); continue; }              /* test: add a channel of kind d2 */
            if ((st & 0xF0u) == 0xB0u && d1 == 107u) { if (chan_n) { seq_stop(); chan_remove(chan_n - 1u); proj_edited(); } continue; }     /* test: remove the last channel */
            if ((st & 0xF0u) == 0xB0u && d1 == 105u && d2 < PROJ_SLOTS) { proj_open(d2); set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; rack_force = 1; chip_drawn = ~0u; sel = 0; continue; }
            if ((st & 0xF0u) == 0x90u && d2) { if (mt < chan_n) chip_note(mt, d1, d2); }
            else if ((st & 0xF0u) == 0x80u || (st & 0xF0u) == 0x90u) { if (mt < chan_n) chip_off(mt); }
        }

        for (k = 0; k < 27u; k++)
            if (prev >> k & 1u && last_note == held[k]) lit |= 1u << k;
        if (seq_run && sel < chan_n && vc[sel].on) {            /* playing: the key of the note the playhead has reached lights */
            int32_t ki = (int32_t)vc[sel].note - (int32_t)base;
            if (ki >= 0 && ki < 27) lit |= 1u << ki;
        }
        if (fx_layer) {                                 /* the pads' keys glow, the held ones are bright */
            uint32_t p;
            for (k = 0; k < 27u; k++) { fm1_led_key(KEY_LED(k), 0); fm1_led_glow(KEY_LED(k), 0); }
            for (p = 0; p < FXN; p++) { fm1_led_glow(KEY_LED(FX_KEY[p]), 1); if (fx_mask >> p & 1u) fm1_led_key(KEY_LED(FX_KEY[p]), 1); }
        } else for (k = 0; k < 27u; k++) { fm1_led_glow(KEY_LED(k), 0); fm1_led_key(KEY_LED(k), lit >> k & 1u); }
        fx_solo_ch = (fx_held & FXB(FX_SOLO)) && sel < chan_n ? (uint8_t)sel : 255u;
        proj_unlocked = set.unlocked;
        fm1_led_glow(BTN_LFO, undo_cnt > 0u);
        fm1_led_key(BTN_SEQ, view == 3u);
        fm1_led_key(BTN_FX, fx_layer);
        fm1_led_key(BTN_PLAY, seq_run);
        fm1_led_key(BTN_REC, rec_on);
        fm1_led_glow(BTN_SAVE, proj_dirty);             /* a dim light: edits not saved yet */
        fm1_led_key(BTN_SAVE, proj_t_saved && fm1_ms - proj_t_saved < 500u);
        fm1_led_key(BTN_OCT_DOWN, set.oct < -1);    /* octave: dim at +-1, bright at +-2 */
        fm1_led_glow(BTN_OCT_DOWN, set.oct == -1);
        fm1_led_key(BTN_OCT_UP, set.oct > 1);
        fm1_led_glow(BTN_OCT_UP, set.oct == 1);
        if ((pick_on && pick_del) || rack_rm) { fm1_led_key(BTN_OCT_DOWN, 1); fm1_led_key(BTN_OCT_UP, 1); }     /* yes or no */

        cpu = cpu_max / (58u * FM1_TICKS_PER_US);     /* percent of the 5.8 ms block */
        if (fm1_ms - cc_ms >= 500u) { cc_ms = fm1_ms; midi_send(0xB0, 100, cpu > 127u ? 127u : cpu); midi_send(0xB0, 101, proj_cur | (proj_dirty ? 0x10u : 0u) | (proj_loaded ? 0x20u : 0u)); }
        if (an_kind == AN_CRIT && act_ms >= an_t0) an_end = 0;           /* touched: the critter goes */
        if (!anim_on() && !rec_on && !fx_layer && !menu_on && !pick_on && fm1_ms - act_ms > CRIT_MS && fm1_ms >= crit_next) {
            anim_start(AN_CRIT, crit_n++ % 6u); crit_next = fm1_ms + 15000u;
        }
        if (pick_on) { if (pick_dirty) { pick_draw(); pick_dirty = 0; } }
        else if (menu_on) { if (menu_dirty) { menu_draw(t->acc); menu_dirty = 0; }      /* (the three bands cover the whole screen: no clear first, that was the flash) */ }
        else if (fx_layer) {
            if (rack_force) { fx_view_force = 1; hdr_force = 1; rack_force = 0; }
            fx_view_update(); hdr_update(view, sel, t, ch_chip, last_note);
        }
        else if (view == 3u && sel < chan_n) { ed_update(sel, base, rack_force); rack_force = 0; hdr_update(3, sel, t, ch_chip, last_note); }
        else if (view == 2u) { sg_update(rack_force); rack_force = 0; hdr_update(2, sel, t, ch_chip, last_note); }
        else if (view == 1u) { rack_update(sel, rack_force); rack_force = 0; hdr_update(1, sel, t, ch_chip, last_note); }
        else if (sel < chan_n) {
            if (chip_drawn != ch_chip) {
                chip_drawn = ch_chip; draw_header(t, ch_chip); hdr_force = 1;
                note_drawn = ~0u; ch_drawn = 99;
            }
            if (redraw || ch_drawn != sel) { chip_panels(t, sel, chip_page); ch_drawn = (uint8_t)sel; }
            hdr_update(0, sel, t, ch_chip, last_note);
        }

        if (proj_dirty && !seq_run && !pick_on && quiet() && fm1_ms - proj_t_edit > 2000u && (!proj_t_fail || fm1_ms - proj_t_fail > 10000u)) {   /* stopped and silent: save the project */
            if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; } else proj_t_fail = fm1_ms;
        }
        if (dirty && quiet() && fm1_ms - t_change > 2000u) {
            settings_save(&set, sizeof set);
            dirty = 0;
        }
    }
}
