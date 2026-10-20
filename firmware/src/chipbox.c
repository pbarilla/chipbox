/* SPDX-License-Identifier: GPL-3.0-only */
/* Chipbox: sound chips for the FM-1: NES, Game Boy and Master System, which can sound together. Pick a chip, the sound and the screen change with it.
 *
 *   keys             play the selected channel (and, in the rack, write it)
 *   PRESETS          down the channels (all three chips, NES PU1 .. SMS TN3)
 *   ALGORITHM        rack: the page A..D        chip screen: the knobs' page (tone / envelope)
 *   SELECT           rack: the cursor along the steps
 *   keys             in the rack: set the step under the cursor to that note, or clear it if it has it
 *   OCT- OCT+        octave (on their LEDs)             MASTER          volume
 *   KNOB 1..4        wave or duty, volume, vibrato, sweep (Game Boy: pan)
 *   HOME             the rack: every channel's steps and the playhead (HOME again: back to the chip)
 *   PLAY             start / stop the demo tune (the sequencer's first run: no screen for it yet)
 *   USB-MIDI         CC 102 = play (64 and up) or stop, CC 103 = tempo / 2, CC 104 = save, CC 105 = open a project (0..7); the device sends CC 101 = open project | 16 if unsaved | 32 if loaded from flash; MIDI channel n plays chip n / 4, channel n % 4 (all three chips can sound together); program change n picks the keys' chip;
 *                    the CPU load goes out as control change 100 (percent of the audio budget)
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
     RGB(24, 24, 28), RGB(52, 52, 60), RGB(220, 220, 220), RGB(214, 48, 40), RGB(110, 110, 122), RGB(200, 200, 200), RGB(214, 48, 40)},
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
    uint32_t ch, c;
    for (ch = 0; ch < CHIPS; ch++)
        for (c = 0; c < 4u; c++)
            if (chip_level(ch, c)) return 0;
    return 1;
}

/* ---- parameters under the knobs */
static inst_t *cur_inst(uint32_t chip, uint32_t c) { return &inst[chip][c]; }
static uint32_t has_wave(uint32_t chip, uint32_t c) { return chip == CHIP_GB && c == 2u; }
static uint32_t has_vol(uint32_t chip, uint32_t c) { return !(chip == CHIP_NES && c == 2u); }     /* the NES triangle has no level control */
static uint32_t has_duty(uint32_t chip, uint32_t c) { return c < 2u && chip != CHIP_SMS; }
static void knob_a(uint32_t chip, uint32_t c, int32_t d)    /* wave or duty */
{
    inst_t *in = cur_inst(chip, c);
    int32_t v;
    if (has_wave(chip, c)) { v = (int32_t)in->wave + d; in->wave = (uint8_t)(v < 0 ? 0 : v > 3 ? 3 : v); }
    else if (has_duty(chip, c)) { v = (int32_t)in->duty + d; in->duty = (uint8_t)(v < 0 ? 0 : v > 3 ? 3 : v); }
}
static int8_t clamp_i(int32_t v, int32_t lo, int32_t hi) { return (int8_t)(v < lo ? lo : v > hi ? hi : v); }
static void env_knob(uint32_t chip, uint32_t c, uint32_t k, int32_t d)     /* the envelope page: attack, decay, sustain, release */
{
    inst_t *in = cur_inst(chip, c);
    uint8_t *p = k == 0u ? &in->atk : k == 1u ? &in->dec : k == 2u ? &in->sus : &in->rel;
    *p = (uint8_t)clamp_i((int32_t)*p + d, 0, 15);
}
static void knob_set(uint32_t chip, uint32_t c, uint32_t k, int32_t d)
{
    inst_t *in = cur_inst(chip, c);
    if (k == 0u) knob_a(chip, c, d);
    else if (k == 1u) in->vol = (uint8_t)clamp_i((int32_t)in->vol + d, 0, 15);
    else if (k == 2u) in->vib_dep = (uint8_t)clamp_i((int32_t)in->vib_dep + d, 0, 15);
    else if (chip == CHIP_GB) in->pan = (uint8_t)clamp_i((int32_t)in->pan + d, 0, 2);
    else in->sweep = clamp_i((int32_t)in->sweep + d, -16, 16);
}

/* ---- the screen */
static const uint8_t AN_COUNT[CHIPS] = {3, 2, 1};
static void draw_header(const theme_t *t, uint32_t chip)
{
    uint32_t x;
    lcd_fill(0, 0, 240, 240, t->bg);
    text_on(8, 6, 150, &FONT_L, t->name, t->fg, t->bg, 0);
    if (chip == CHIP_NES) { lcd_fill(0, 38, 240, 2, t->acc); lcd_fill(0, 40, 240, 1, t->fg); }                  /* the Famicom's red and white */
    else if (chip == CHIP_GB) { for (x = 0; x < 240u; x += 6u) lcd_fill(x, 38, 4, 3, t->fg); }                   /* a dotted rule */
    else { lcd_fill(0, 38, 240, 1, t->fg); lcd_fill(0, 41, 240, 1, t->fg); }                                      /* a double line */
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
static void channel_glyph(uint32_t chip, uint32_t c, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t col)
{
    const inst_t *in = &inst[chip][c];
    static const uint8_t D8[4] = {1, 2, 4, 6};
    if (chip == CHIP_GB && c == 2u) {
        static const uint8_t K[4] = {G_PULSE, G_SAW, G_TRI, G_SINE};
        wave_glyph(K[in->wave & 3u], 4, x, y, w, h, col);
    } else if (chip == CHIP_NES && c == 2u) wave_glyph(G_TRI, 0, x, y, w, h, col);
    else wave_glyph(G_PULSE, has_duty(chip, c) ? D8[in->duty & 3u] : 4u, x, y, w, h, col);
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
#include "chipview.c"
#include "menu.c"
#include "picker.c"

static void app_main(void)
{
    struct { int8_t oct; uint8_t chip, ch, an[CHIPS], proj; } set = {0, CHIP_NES, 0, {AN_HW, AN_HW, 0}, 0};
    uint32_t prev = 0, k, t_change = 0, dirty = 0, note_drawn = ~0u, chip_drawn = ~0u;
    uint32_t last_note = 255u, rack = 0, rack_force = 0, chip_page = 0;
    uint32_t home_was = 0, home_t0 = 0, home_used = 0, home_now, home_tap, edit_was = 0, edit_t0 = 0, edit_now;
    uint8_t held[27] = {0}, ch_drawn = 99;
    settings_load(&set, sizeof set);
    memcpy(inst_def, inst, sizeof inst_def);
    proj_cur = set.proj < PROJ_SLOTS ? set.proj : 0;                  /* the project you left open; the demo tune on a new device */
    proj_loaded = proj_load(proj_cur) == 0;
    if (!proj_loaded) { if (proj_cur == 0u) { seq_demo(); proj_edited(); } else proj_blank(); }
    if (set.oct < -2 || set.oct > 2) set.oct = 0;
    if (set.chip >= CHIPS) set.chip = CHIP_NES;
    if (set.ch >= CHIP_NCH[set.chip]) set.ch = 0;
    for (k = 0; k < CHIPS; k++)
        if (set.an[k] >= AN_COUNT[k]) set.an[k] = AN_COUNT[k] > 1u ? 1u : 0u;
    set.an[CHIP_NES] = set.an[CHIP_GB] = AN_HW;     /* always the analog stage */
    for (k = 0; k < CHIPS; k++) analog_mode[k] = set.an[k];

    for (;;) {
        const theme_t *t = &THEME[set.chip];
        static uint32_t cc_ms;
        uint32_t keys, down, up, rel, b, lit = 0, base = (uint32_t)(KEY0 + 12 * set.oct), cpu, redraw = 0;
        uint8_t st, d1, d2;
        int32_t e;
        platform_poll();

        keys = fm1_in.notes;
        down = keys & ~prev;
        up = prev & ~keys;
        prev = keys;
        for (k = 0; k < 27u; k++) {
            if (down >> k & 1u) {
                held[k] = (uint8_t)(base + k); chip_note(set.chip, set.ch, held[k], 127); last_note = held[k]; midi_send(0x90 | set.ch, held[k], 100);
                if (rack && !menu_on && !pick_on) { seq_toggle(set.chip * 4u + set.ch, rack_cur, held[k]); rack_inval = 1; proj_edited(); }
            }
            if (up >> k & 1u) { if (last_note == held[k]) chip_off(set.chip, set.ch); midi_send(0x80 | set.ch, held[k], 0); }
        }

        b = fm1_input_edges(&rel);
        home_now = fm1_in.buttons >> BTN_HOME & 1u;     /* HOME: a tap goes between the views, a hold opens the projects */
        if (home_now && !home_was) { home_t0 = fm1_ms; home_used = 0; }
        if (home_now && !home_used && !menu_on && !pick_on && fm1_ms - home_t0 >= 600u) { pick_open(); home_used = 1; }
        home_tap = !home_now && home_was && !home_used;
        home_was = home_now;
        edit_now = fm1_in.buttons >> BTN_EDIT & 1u;
        if (edit_now && !edit_was) edit_t0 = fm1_ms;
        edit_was = edit_now;
        if (pick_on) {                                  /* the projects: SELECT moves, OCT+ yes, OCT- no, hold EDIT to clear one */
            e = fm1_enc_take(ENC_SELECT);
            if (e) { pick_sel = (pick_sel + (e < 0 ? PROJ_SLOTS - 1u : 1u)) % PROJ_SLOTS; pick_del = 0; pick_dirty = 1; }
            if (edit_now && fm1_ms - edit_t0 >= 600u && !pick_del && pick_have[pick_sel]) { pick_del = 1; pick_dirty = 1; }
            if ((b & (1u << BTN_OCT_UP)) || (home_tap && !pick_del)) {       /* OCT+ (yes) or a HOME tap opens the slot under the frame */
                if (pick_del) {
                    proj_clear(pick_sel);
                    if (pick_sel == proj_cur) { seq_stop(); proj_blank(); proj_dirty = 0; }
                    pick_del = 0; pick_scan(); pick_dirty = 1;
                } else {
                    if (pick_sel != proj_cur) {
                        proj_open(pick_sel);
                        set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; last_note = 255u;
                    }
                    pick_on = 0; rack_force = 1; chip_drawn = ~0u;
                }
            } else if (b & (1u << BTN_OCT_DOWN)) {
                if (pick_del) { pick_del = 0; pick_dirty = 1; } else { pick_on = 0; rack_force = 1; chip_drawn = ~0u; }
            } else if ((b & (1u << BTN_GLO)) || home_tap) { pick_del = 0; pick_on = 0; rack_force = 1; chip_drawn = ~0u; }
            fm1_enc_take(ENC_PRESETS); fm1_enc_take(ENC_ALGORITHM);
            for (k = 0; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else {
        if (b & (1u << BTN_OCT_DOWN) && set.oct > -2) { set.oct--; dirty = 1; t_change = fm1_ms; }
        if (b & (1u << BTN_OCT_UP) && set.oct < 2) { set.oct++; dirty = 1; t_change = fm1_ms; }
        if (b & (1u << BTN_GLO)) {                      /* help for the view you are on; GLO or HOME again goes back */
            if (menu_on) { menu_on = 0; rack_force = 1; chip_drawn = ~0u; }
            else menu_open(rack ? HELP_RACK : HELP_CHIP, rack ? sizeof HELP_RACK / sizeof HELP_RACK[0] : sizeof HELP_CHIP / sizeof HELP_CHIP[0]);
        } else if (home_tap) {
            if (menu_on) menu_on = 0; else rack ^= 1u;
            rack_force = 1; chip_drawn = ~0u;
        }
        if (b & (1u << BTN_PLAY)) { if (seq_run) seq_stop(); else seq_play(); }
        if (b & (1u << BTN_SAVE)) { if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; } }
        if (menu_on) {                                  /* in a menu only SELECT does anything */
            e = fm1_enc_take(ENC_SELECT);
            if (e) menu_scroll(e);
            fm1_enc_take(ENC_PRESETS); fm1_enc_take(ENC_ALGORITHM);
            for (k = 0; k < 4u; k++) fm1_enc_take(ENC_KNOB1 + k);
        } else {
        e = fm1_enc_take(ENC_PRESETS);                  /* always: down the whole list of channels, NES PU1 .. SMS TN3 */
        if (e) {
            uint32_t n = 0, f = 0, i, was = set.chip;
            chip_off(set.chip, set.ch);
            for (i = 0; i < CHIPS; i++) { if (i == set.chip) f = n + set.ch; n += CHIP_NCH[i]; }
            f = (f + (e < 0 ? n - 1u : 1u)) % n;
            for (i = 0; i < CHIPS && f >= CHIP_NCH[i]; i++) f -= CHIP_NCH[i];
            set.chip = (uint8_t)i; set.ch = (uint8_t)f;
            dirty = 1; t_change = fm1_ms;
            if (set.chip != was) { last_note = 255u; continue; }       /* redraw the whole screen in the new look */
        }
        e = fm1_enc_take(ENC_ALGORITHM);                /* the page: A..D */
        if (e) { if (rack) { seq_page(e); rack_inval = 1; } else { chip_page ^= 1u; redraw = 1; } }
        e = fm1_enc_take(ENC_SELECT);                   /* the cursor, along the steps */
        if (e && rack) { rack_cur = (rack_cur + (e < 0 ? 15u : 1u)) & 15u; rack_inval = 1; }
        for (k = 0; k < 4u; k++) {
            e = fm1_enc_take(ENC_KNOB1 + k);
            if (e) { if (rack || !chip_page) knob_set(set.chip, set.ch, k, e); else env_knob(set.chip, set.ch, k, e); redraw = 1; proj_edited(); }
        }
        }
        }
        while (midi_get(&st, &d1, &d2)) {
            uint32_t mt = st & 0xFu;                         /* MIDI channel = track: chip mt / 4, channel mt % 4 */
            if ((st & 0xF0u) == 0xC0u) {                    /* program change: pick the chip */
                chip_off(set.chip, set.ch);
                set.chip = (uint8_t)(d1 % CHIPS);
                if (set.ch >= CHIP_NCH[set.chip]) set.ch = 0;
                dirty = 1; t_change = fm1_ms; last_note = 255u;
                t = &THEME[set.chip];
                continue;
            }
            if ((st & 0xF0u) == 0xB0u && d1 == 102u) { if (d2 >= 64u) seq_play(); else if (seq_run) seq_stop(); continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 103u) { seq_bpm = d2 * 2u; proj_edited(); continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 104u) { if (proj_save(proj_cur) == 0) { proj_dirty = 0; proj_t_saved = fm1_ms | 1u; } continue; }
            if ((st & 0xF0u) == 0xB0u && d1 == 105u && d2 < PROJ_SLOTS) { proj_open(d2); set.proj = (uint8_t)proj_cur; dirty = 1; t_change = fm1_ms; rack_force = 1; chip_drawn = ~0u; continue; }
            if ((st & 0xF0u) == 0x90u && d2) chip_note(mt >> 2, mt & 3u, d1, d2);
            else if ((st & 0xF0u) == 0x80u || (st & 0xF0u) == 0x90u) chip_off(mt >> 2, mt & 3u);
        }

        for (k = 0; k < 27u; k++)
            if (prev >> k & 1u && last_note == held[k]) lit |= 1u << k;
        for (k = 0; k < 27u; k++)
            fm1_led_key(KEY_LED(k), lit >> k & 1u);
        fm1_led_key(BTN_PLAY, seq_run);
        fm1_led_glow(BTN_SAVE, proj_dirty);             /* a dim light: edits not saved yet */
        fm1_led_key(BTN_SAVE, proj_t_saved && fm1_ms - proj_t_saved < 500u);
        fm1_led_key(BTN_OCT_DOWN, set.oct < -1);    /* octave: dim at +-1, bright at +-2 */
        fm1_led_glow(BTN_OCT_DOWN, set.oct == -1);
        fm1_led_key(BTN_OCT_UP, set.oct > 1);
        fm1_led_glow(BTN_OCT_UP, set.oct == 1);
        if (pick_on && pick_del) { fm1_led_key(BTN_OCT_DOWN, 1); fm1_led_key(BTN_OCT_UP, 1); }     /* yes or no */

        cpu = cpu_max / (58u * FM1_TICKS_PER_US);     /* percent of the 5.8 ms block */
        if (fm1_ms - cc_ms >= 500u) { cc_ms = fm1_ms; midi_send(0xB0, 100, cpu > 127u ? 127u : cpu); midi_send(0xB0, 101, proj_cur | (proj_dirty ? 0x10u : 0u) | (proj_loaded ? 0x20u : 0u)); }
        if (pick_on) { if (pick_dirty) { pick_draw(); pick_dirty = 0; } }
        else if (menu_on) { if (menu_dirty) { lcd_fill(0, 0, 240, 240, R_BG); menu_draw(THEME[set.chip].acc); menu_dirty = 0; } }
        else if (rack) { rack_update(set.chip, set.ch, rack_force); rack_force = 0; }
        else {
            if (chip_drawn != set.chip) {
                chip_drawn = set.chip; draw_header(t, set.chip);
                note_drawn = ~0u; ch_drawn = 99;
            }
            if (redraw || ch_drawn != set.ch) { chip_panels(t, set.chip, set.ch, chip_page); ch_drawn = set.ch; }
            if (last_note != note_drawn) { note_drawn = last_note; draw_note(t, last_note); }
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
