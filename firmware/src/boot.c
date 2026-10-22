/* SPDX-License-Identifier: GPL-3.0-only */
/* Power-on: a spoof of some console's start-up with the Chipbox name, a different one each time. The screen waits out the speakers' pop,
 * the sound comes with the logo. Included by chipbox.c after the views. */
#define BOOT_N 14u
typedef struct { uint16_t t; uint8_t ch, note; } bev_t;         /* at t ms: a note on channel ch, or off (note 0) */
static const char BNAME[] = "Chipbox";

static void boot_text(int32_t cx, int32_t y, const char *s, uint16_t c)       /* big, bold, centred on cx */
{
    int32_t x = cx - text_w(&FONT_L, s) / 2;
    cv_text(x, y, &FONT_L, s, c); cv_text(x + 1, y, &FONT_L, s, c);
}
static void boot_fill(uint16_t c) { cv_rect(0, 0, 240, 240, c); }

/* Game Boy: the name rolls down, lands, the coin */
static void bf_gb(uint32_t t)
{
    int32_t y = t < 300u ? -48 : t < 1300u ? -48 + (int32_t)(t - 300u) * 148 / 1000 : 100;
    boot_fill(RGB(155, 188, 15));
    cv_text((240 - text_w(&FONT_L, BNAME)) / 2 + 1, y + 1, &FONT_L, BNAME, RGB(48, 98, 48));
    boot_text(120, y, BNAME, RGB(15, 56, 15));
}
/* Famicom Disk System: a sign flies in from the left flashing colours, settles on one; striped bands top and bottom, a stub below */
static uint32_t boot_seed;
static void bf_fds(uint32_t t)
{
    static const uint16_t FILL[5] = {RGB(40, 90, 230), RGB(200, 40, 40), RGB(30, 150, 60), RGB(230, 120, 30), RGB(30, 170, 200)};
    static const uint16_t EDGE[5] = {RGB(120, 180, 255), RGB(255, 150, 140), RGB(130, 230, 140), RGB(255, 205, 120), RGB(150, 240, 255)};
    uint32_t i, s = 12345u, ci = t < 1100u ? (t / 70u) % 5u : boot_seed % 5u;
    int32_t p, dx = 0, y;
    uint16_t fill = FILL[ci], edge = EDGE[ci];
    boot_fill(RGB(0, 0, 8));
    for (i = 0; i < 40u; i++) {
        s = s * 1664525u + 1013904223u;
        cv_rect((int32_t)(s >> 8) % 240, 30 + (int32_t)(s >> 20) % 180, 1 + (int32_t)(i & 1u), 1 + (int32_t)(i & 1u), (i % 5u) == 0u ? RGB(255, 120, 120) : RGB(200, 210, 255));
    }
    for (y = 0; y < 26; y += 4) { cv_rect(0, y, 240, 2, edge); cv_rect(0, 238 - y, 240, 2, edge); }              /* the bands: lines getting thicker toward the edge of the screen */
    cv_rect(0, 26, 240, 2, fill); cv_rect(0, 212, 240, 2, fill);
    if (t < 200u) return;
    if (t < 1100u) { p = 1000 - (int32_t)(t - 200u) * 1000 / 900; dx = -(p * p / 1000) * 260 / 1000; }
    cv_rect(36 + dx, 62, 168, 76, edge); cv_rect(39 + dx, 65, 162, 70, RGB(10, 20, 60)); cv_rect(41 + dx, 67, 158, 66, fill);
    cv_rect(36 + dx, 62, 3, 3, RGB(0, 0, 8)); cv_rect(201 + dx, 62, 3, 3, RGB(0, 0, 8)); cv_rect(36 + dx, 135, 3, 3, RGB(0, 0, 8)); cv_rect(201 + dx, 135, 3, 3, RGB(0, 0, 8));
    cv_text(120 + dx - text_w(&FONT_L, BNAME) / 2 + 2, 72, &FONT_L, BNAME, RGB(10, 20, 60)); cv_text(120 + dx - text_w(&FONT_L, BNAME) / 2 + 3, 72, &FONT_L, BNAME, RGB(10, 20, 60));
    boot_text(120 + dx, 70, BNAME, edge);
    if (t >= 1100u) cv_text(120 - text_w(&FONT_S, "PLEASE SET SONG DISK") / 2, 112, &FONT_S, "PLEASE SET SONG DISK", RGB(255, 255, 255));
    cv_rect(118 + dx, 138, 4, 22, RGB(150, 150, 160)); cv_rect(112 + dx, 160, 16, 8, RGB(150, 150, 160)); cv_rect(114 + dx, 162, 12, 4, RGB(90, 90, 100)); cv_rect(118 + dx, 163, 4, 2, RGB(210, 50, 50));
}
/* Atari: the letters scramble, flash, settle; the rainbow stripes grow out from the middle */
static void bf_atari(uint32_t t)
{
    static const uint16_t RB[4] = {RGB(236, 130, 40), RGB(214, 64, 44), RGB(150, 70, 170), RGB(0, 98, 210)};
    int32_t x, i, hw = t > 300u ? (int32_t)(t - 300u) * 120 / 600 : 0;
    char c[2] = {0, 0};
    boot_fill(RGB(0, 0, 0));
    x = 120 - text_w(&FONT_L, BNAME) / 2;
    for (i = 0; BNAME[i]; i++) {
        c[0] = BNAME[i];
        if (t < 900u) c[0] = (char)('A' + ((uint32_t)(i * 977 + (int32_t)(t / 70u) * 131) * 2654435761u >> 16) % 26u);
        cv_text(x, 90, &FONT_L, c, t >= 900u && t < 980u ? RGB(255, 255, 255) : RGB(244, 226, 196));
        cv_text(x + 1, 90, &FONT_L, c, t >= 900u && t < 980u ? RGB(255, 255, 255) : RGB(244, 226, 196));
        x += text_w(&FONT_L, c);
    }
    if (hw > 120) hw = 120;
    for (i = 0; i < 4; i++) cv_rect(120 - hw, 140 + i * 5, 2 * hw, 4, RB[i]);
    if (t > 1200u) cv_text(120 - text_w(&FONT_S, "VIDEO MUSIC SYSTEM") / 2, 172, &FONT_S, "VIDEO MUSIC SYSTEM", RGB(244, 226, 196));
}
/* Neo Geo: the name slams in from the left in red with a silver edge, a flash, a bar underneath */
static void bf_neo(uint32_t t)
{
    int32_t x = t < 600u ? -170 : t < 900u ? -170 + (int32_t)(t - 600u) * 170 / 300 : 0;
    boot_fill(t >= 900u && t < 960u ? RGB(255, 255, 255) : RGB(0, 0, 0));
    if (t >= 600u) {
        boot_text(120 + x + 2, 92, BNAME, RGB(190, 195, 210));
        boot_text(120 + x, 90, BNAME, RGB(214, 28, 40));
    }
    if (t >= 1100u) {
        cv_rect(14, 138, 212, 16, RGB(214, 28, 40));
        cv_text(120 - text_w(&FONT_S, "16 CHANNEL PRO-GEAR SPEC") / 2, 138, &FONT_S, "16 CHANNEL PRO-GEAR SPEC", RGB(255, 255, 255));
    }
}
static void boot_bee(int32_t x, int32_t y, int32_t k, uint32_t flap)     /* a bee, k times life size */
{
    int32_t up = flap ? 8 : 5;
    cv_rect(x + 3 * k, y - up * k, 6 * k, 5 * k, RGB(255, 255, 255)); cv_rect(x + 10 * k, y - up * k, 6 * k, 5 * k, RGB(255, 255, 255));
    cv_rect(x, y - 3 * k, 18 * k, 12 * k, RGB(250, 210, 30)); cv_rect(x + 5 * k, y - 3 * k, 3 * k, 12 * k, RGB(30, 24, 20)); cv_rect(x + 11 * k, y - 3 * k, 3 * k, 12 * k, RGB(30, 24, 20));
    cv_rect(x + 14 * k, y, 3 * k, 3 * k, RGB(30, 24, 20));
}
/* PC Engine: a bee crosses the white screen, the orange name lands behind it */
static void bf_pce(uint32_t t)
{
    int32_t bx = t < 300u ? -30 : (int32_t)(t - 300u) * 290 / 1200 - 30, by = 150 + VIB[(t / 40u) & 31u] / 6;
    boot_fill(RGB(232, 230, 222));
    if (t >= 1500u) boot_text(120, 90, BNAME, RGB(242, 132, 34));
    if (t >= 300u && t < 1600u) boot_bee(bx, by, 1, 1u);
}
/* a home computer's blue screen: the lines type in, a cursor blinks */
static void bf_c64(uint32_t t)
{
    uint16_t lb = RGB(136, 126, 203);
    boot_fill(lb);
    cv_rect(24, 28, 192, 184, RGB(64, 49, 141));
    if (t >= 500u) cv_text(32, 40, &FONT_S, "**** CHIPBOX V" APP_VER " ****", lb);
    if (t >= 900u) cv_text(32, 72, &FONT_S, "16 CHANNELS FREE", lb);
    if (t >= 1300u) cv_text(32, 104, &FONT_S, "READY.", lb);
    if (t >= 1300u && ((t / 400u) & 1u) == 0u) cv_rect(32, 126, 10, 14, lb);
}

/* Amiga: a black screen, a red box blinks at the top, the old guru's words */
static void bf_amiga(uint32_t t)
{
    boot_fill(RGB(0, 0, 0));
    if (t >= 600u && ((t / 450u) & 1u) == 0u) {
        cv_rect(12, 20, 216, 3, RGB(255, 0, 0)); cv_rect(12, 77, 216, 3, RGB(255, 0, 0)); cv_rect(12, 20, 3, 60, RGB(255, 0, 0)); cv_rect(225, 20, 3, 60, RGB(255, 0, 0));
    }
    if (t >= 600u) {
        cv_text(120 - text_w(&FONT_S, "Chipbox is starting.") / 2, 28, &FONT_S, "Chipbox is starting.", RGB(255, 0, 0));
        cv_text(120 - text_w(&FONT_S, "Guru Meditation 16.CHAN") / 2, 50, &FONT_S, "Guru Meditation 16.CHAN", RGB(255, 0, 0));
    }
}
/* MSX: blue, a beep, the BASIC prompt */
static void bf_msx(uint32_t t)
{
    boot_fill(RGB(84, 85, 237));
    if (t >= 500u) cv_text(16, 40, &FONT_S, "Chipbox BASIC version " APP_VER, RGB(255, 255, 255));
    if (t >= 900u) cv_text(16, 72, &FONT_S, "16 channels free", RGB(255, 255, 255));
    if (t >= 1300u) cv_text(16, 104, &FONT_S, "Ok", RGB(255, 255, 255));
    if (t >= 1300u && ((t / 400u) & 1u) == 0u) cv_rect(16, 124, 10, 14, RGB(255, 255, 255));
}

/* TI-99/4A: a cyan screen, the colour bars scroll, a key prompt */
static void bf_ti(uint32_t t)
{
    static const uint16_t BAR[8] = {RGB(255, 0, 0), RGB(255, 160, 0), RGB(255, 255, 0), RGB(0, 200, 0), RGB(0, 200, 200), RGB(0, 0, 255), RGB(150, 0, 200), RGB(255, 0, 150)};
    int32_t y;
    boot_fill(RGB(100, 220, 230));
    cv_text(120 - text_w(&FONT_S, "Chipbox Home Computer") / 2, 50, &FONT_S, "Chipbox Home Computer", RGB(0, 0, 0));
    if (t >= 300u) for (y = 0; y < 8; y++) cv_rect(30, 90 + y * 6, 180, 6, BAR[((uint32_t)y + t / 100u) & 7u]);
    if (t >= 1200u && ((t / 400u) & 1u) == 0u) cv_text(120 - text_w(&FONT_S, "READY - PRESS ANY KEY") / 2, 180, &FONT_S, "READY - PRESS ANY KEY", RGB(0, 0, 0));
}
/* ColecoVision: the letters pop in one by one in rainbow colours over a rainbow bar */
static void bf_coleco(uint32_t t)
{
    static const uint16_t RB[7] = {RGB(255, 40, 40), RGB(255, 150, 30), RGB(255, 235, 40), RGB(60, 210, 60), RGB(40, 210, 220), RGB(70, 90, 255), RGB(210, 70, 230)};
    int32_t i, x = 120 - text_w(&FONT_L, BNAME) / 2;
    char c[2] = {0, 0};
    boot_fill(RGB(20, 30, 140));
    for (i = 0; BNAME[i]; i++) {
        c[0] = BNAME[i];
        if (t >= 600u + (uint32_t)i * 80u) { cv_text(x, 90, &FONT_L, c, RB[i]); cv_text(x + 1, 90, &FONT_L, c, RB[i]); }
        x += text_w(&FONT_L, c);
    }
    if (t >= 600u + 7u * 80u) for (i = 0; i < 7; i++) cv_rect(30, 140 + i * 3, 180, 3, RB[i]);
}
/* Atari ST: a green desktop, a menu bar, the busy bee */
static void bf_st(uint32_t t)
{
    boot_fill(RGB(0, 0, 0));
    if (t < 300u) return;
    cv_rect(0, 0, 240, 240, RGB(0, 160, 90));
    cv_rect(0, 0, 240, 15, RGB(255, 255, 255)); cv_rect(0, 15, 240, 1, RGB(0, 0, 0));
    cv_text(6, -1, &FONT_S, "Desk  File  View  Options", RGB(0, 0, 0));
    cv_rect(24, 40, 30, 26, RGB(0, 0, 0)); cv_rect(26, 42, 26, 22, RGB(255, 255, 255)); cv_rect(30, 46, 18, 8, RGB(0, 0, 0));
    cv_rect(190, 176, 26, 30, RGB(0, 0, 0)); cv_rect(192, 180, 22, 24, RGB(255, 255, 255)); cv_rect(188, 174, 30, 4, RGB(0, 0, 0));
    if (t >= 600u) boot_bee(92, 100 + VIB[(t / 60u) & 31u] / 14, 3, (t / 150u) & 1u);
}
/* Classic Mac: the smiling box on grey, a low bong, a welcome */
static void bf_mac(uint32_t t)
{
    boot_fill(RGB(210, 210, 210));
    if (t >= 500u) {
        cv_rect(90, 64, 60, 72, RGB(0, 0, 0)); cv_rect(93, 67, 54, 66, RGB(235, 235, 235));
        cv_rect(98, 74, 44, 40, RGB(0, 0, 0)); cv_rect(100, 76, 40, 36, RGB(200, 225, 200));
        cv_rect(108, 86, 3, 7, RGB(0, 0, 0)); cv_rect(129, 86, 3, 7, RGB(0, 0, 0)); cv_rect(119, 90, 3, 10, RGB(0, 0, 0)); cv_rect(117, 99, 5, 2, RGB(0, 0, 0));
        cv_rect(108, 104, 24, 2, RGB(0, 0, 0)); cv_rect(106, 102, 2, 2, RGB(0, 0, 0)); cv_rect(132, 102, 2, 2, RGB(0, 0, 0));
        cv_rect(104, 122, 32, 3, RGB(0, 0, 0));
    }
    if (t >= 1800u) cv_text(120 - text_w(&FONT_S, "Welcome to Chipbox") / 2, 160, &FONT_S, "Welcome to Chipbox", RGB(0, 0, 0));
}
/* PC: the BIOS counts its memory, one beep */
static void bf_pc(uint32_t t)
{
    char num[12];
    uint32_t kb = t < 400u ? 0u : t >= 1300u ? 640u : (t - 400u) * 640u / 900u, n;
    boot_fill(RGB(0, 0, 0));
    if (t >= 200u) cv_text(8, 24, &FONT_S, "Chipbox BIOS v" APP_VER, RGB(170, 170, 170));
    if (t >= 400u) {
        n = fmt_u(num, kb); num[n] = 'K'; num[n + 1u] = 0;
        cv_text(8, 56, &FONT_S, "Memory Test:", RGB(170, 170, 170));
        cv_text(8 + text_w(&FONT_S, "Memory Test: "), 56, &FONT_S, num, RGB(170, 170, 170));
        if (t >= 1300u) cv_text(8 + text_w(&FONT_S, "Memory Test: 640K "), 56, &FONT_S, "OK", RGB(170, 170, 170));
    }
    if (t >= 1700u) cv_text(8, 88, &FONT_S, "Starting Chipbox...", RGB(170, 170, 170));
}
/* Virtual Boy: red on black, the name in layers, a grid floor */
static void bf_vb(uint32_t t)
{
    int32_t i;
    boot_fill(RGB(0, 0, 0));
    if (t >= 400u) {
        for (i = 0; i < 8; i++) cv_line(120, 130, 120 + (i - 4) * 70 + 35, 240, RGB(150, 0, 0));
        for (i = 1; i < 7; i++) cv_rect(0, 130 + i * i * 3, 240, 1, RGB(150, 0, 0));
    }
    if (t >= 600u) {
        boot_text(120 + 4, 64 + 4, BNAME, RGB(90, 0, 0));
        boot_text(120 + 2, 64 + 2, BNAME, RGB(160, 0, 0));
        boot_text(120, 64, BNAME, RGB(255, 20, 20));
    }
}
static void boot_screen(uint32_t n)
{
    static const bev_t GB[] = {{1300, 0, 83}, {1390, 0, 88}, {1900, 0, 0}};
    static const bev_t FDS[] = {{400, 0, 72}, {520, 0, 76}, {640, 0, 79}, {760, 0, 84}, {880, 0, 79}, {1000, 0, 88}, {1120, 0, 84}, {1240, 0, 79}, {1360, 0, 76}, {1480, 0, 72}, {1800, 0, 0}};
    static const bev_t ATARI[] = {{900, 0, 72}, {1000, 0, 76}, {1100, 0, 79}, {1200, 0, 84}, {1700, 0, 0}};
    static const bev_t NEO[] = {{900, 0, 52}, {1050, 0, 57}, {1900, 0, 0}};
    static const bev_t PCE[] = {{1500, 0, 84}, {1620, 0, 88}, {1740, 0, 91}, {1860, 0, 96}, {2400, 0, 0}};
    static const bev_t C64[] = {{1300, 0, 72}, {1360, 0, 0}};
    static const bev_t AMIGA[] = {{600, 0, 50}, {800, 0, 0}};
    static const bev_t MSX[] = {{500, 0, 81}, {600, 0, 0}};
    static const bev_t TI[] = {{400, 0, 81}, {520, 0, 0}};
    static const bev_t COLECO[] = {{600, 0, 64}, {700, 0, 67}, {800, 0, 72}, {900, 0, 76}, {1300, 0, 0}};
    static const bev_t PC[] = {{1500, 0, 83}, {1700, 0, 0}};
    static const bev_t VB[] = {{600, 0, 64}, {700, 0, 0}};
    static const bev_t *const EV[BOOT_N] = {GB, FDS, ATARI, NEO, PCE, C64, AMIGA, MSX, TI, COLECO, 0, 0, PC, VB};
    static const uint8_t EVN[BOOT_N] = {3, 11, 5, 3, 5, 2, 2, 2, 2, 5, 0, 0, 2, 2};
    static const uint16_t END[BOOT_N] = {2700, 3000, 2600, 3000, 3000, 2600, 2400, 2400, 2600, 2600, 2600, 2800, 2800, 2600};
    static void (*const DRAW[BOOT_N])(uint32_t) = {bf_gb, bf_fds, bf_atari, bf_neo, bf_pce, bf_c64, bf_amiga, bf_msx, bf_ti, bf_coleco, bf_st, bf_mac, bf_pc, bf_vb};
    static const uint8_t KIND[BOOT_N] = {KD_NES_PU, KD_NES_PU, KD_TIA, KD_MD, KD_PCE, KD_AY, KD_NES_PU, KD_AY, KD_AY, KD_SMS, KD_AY, KD_AY, KD_AY, KD_NES_PU};
    uint32_t t0 = fm1_ms, t, k = 0, next = 0, b, nch = 0;
    n %= BOOT_N;
    while (nch < 1u && chan_add(KIND[n]) >= 0) { chan[nch].in.vib_dep = 0; nch++; }
    if (!nch) return;
    if (n == 0u) { chan[0].in.duty = 2; chan[0].in.dec = 12; chan[0].in.sus = 8; chan[0].in.rel = 3; }                 /* the coin */
    else if (n == 1u) { chan[0].in.duty = 1; chan[0].in.dec = 6; chan[0].in.sus = 10; chan[0].in.rel = 2; }
    else if (n == 2u) { chan[0].in.wave = 5; chan[0].in.dec = 12; chan[0].in.sus = 10; chan[0].in.rel = 2; }
    else if (n == 3u) { fm_set(0, 0, 1, 10, 6, 4); chan[0].in.dec = 13; chan[0].in.sus = 12; chan[0].in.rel = 5; }         /* a big voice */
    else if (n == 4u) { chan[0].in.wave = 10; chan[0].in.wf = 3; chan[0].in.dec = 8; chan[0].in.sus = 0; chan[0].in.rel = 4; }
    else if (n == 6u || n == 13u) { chan[0].in.duty = 2; chan[0].in.dec = 15; chan[0].in.sus = 10; chan[0].in.rel = 2; }
    else if (n == 9u) { chan[0].in.dec = 15; chan[0].in.sus = 12; }
    else { chan[0].in.sus = 15; chan[0].in.dec = 0; chan[0].in.rel = 1; chan[0].in.vol = 10; }
    k = 0;
    do {
        t = fm1_ms - t0;
        if (t >= next) {
            for (b = 0; b < 3u; b++) {
                cv_begin(240, 80, 0);
                cv_oy = -(int32_t)(b * 80u);
                DRAW[n](t);
                cv_oy = 0;
                cv_blit(0, b * 80u);
            }
            next = t + 40u;
        }
        while (k < EVN[n] && EV[n][k].t <= t) { if (EV[n][k].note) chip_note(EV[n][k].ch, EV[n][k].note, 120); else chip_off(EV[n][k].ch); k++; }
        platform_poll();
    } while (t < END[n]);
    lcd_sync();
    while (nch--) chan_remove(0);
}
