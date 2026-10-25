/* SPDX-License-Identifier: GPL-3.0-only */
/* The Add Channel list: the consoles grouped by type, year, maker or country (KNOB1), an info strip for the one on the bar, the keys playing the one
 * on the bar (an audition), and a card for it (SEL). Included by chipbox.c after menu.c. */
enum { AT_FM, AT_WAVE, AT_PSG, AT_MISC };
typedef struct { const char *maker, *country, *info; uint16_t year; uint8_t type; } addinfo_t;     /* the chip's own year and maker */
static const addinfo_t ADDI[CHIPS] = {
    {"Ricoh", "Japan", "2A03 pulse + tri", 1983, AT_PSG},          /* NES */
    {"Sharp", "Japan", "LR35902\npulse + wave", 1989, AT_WAVE},     /* Game Boy */
    {"TI", "USA", "SN76489 tones", 1979, AT_PSG},                  /* Master System */
    {"Hudson", "Japan", "HuC6280 wave", 1987, AT_WAVE},            /* PC Engine */
    {"Atari", "USA", "TIA 2 voices", 1977, AT_PSG},                /* Atari 2600 */
    {"Konami", "Japan", "VRC6 pulse + saw", 1989, AT_PSG},         /* Konami VRC6 */
    {"GI", "USA", "AY8910 tones", 1978, AT_PSG},                   /* AY-3-8910 */
    {"Yamaha", "Japan", "YM2612 FM", 1988, AT_FM},                 /* Mega Drive */
    {"Nintendo", "Japan", "VSU 6 bit wave", 1995, AT_WAVE},        /* Virtual Boy */
    {"Atari", "USA", "POKEY tones", 1979, AT_PSG},                 /* Atari 8-bit */
    {"IBM", "USA", "1 bit beeper", 1981, AT_MISC},                 /* PC Speaker */
    {"Yamaha", "Japan", "YM3812 FM", 1985, AT_FM},                 /* AdLib */
    {"Namco", "Japan", "WSG 4 bit wave", 1980, AT_WAVE},           /* Namco WSG */
    {"Namco", "Japan", "N163 wave", 1987, AT_WAVE},                /* Namco 163 */
    {"Sunsoft", "Japan", "5B tones", 1992, AT_PSG},                /* Sunsoft 5B */
    {"Nintendo", "Japan", "MMC5 pulse", 1990, AT_PSG},             /* NES MMC5 */
    {"Ricoh", "Japan", "2C33 wave", 1986, AT_WAVE},              /* Famicom Disk */
    {"Konami", "Japan", "SCC 8 bit wave", 1987, AT_WAVE},          /* Konami SCC */
    {"Yamaha", "Japan", "YM2413 FM", 1986, AT_FM},                 /* MSX Music */
    {"Konami", "Japan", "VRC7 FM", 1990, AT_FM},                   /* Konami VRC7 */
    {"Yamaha", "Japan", "YM2151 FM", 1983, AT_FM},                 /* Arcade FM */
    {"Yamaha", "Japan", "YMF262 FM", 1990, AT_FM},                 /* Sound Blaster */
    {"Bandai", "Japan", "ASWAN wave", 1999, AT_WAVE},              /* WonderSwan */
    {"Philips", "Netherlands", "SAA1099 tones", 1984, AT_PSG},     /* Sam Coupe */
    {"MOS", "USA", "6560 tones", 1977, AT_PSG},                    /* VIC-20 */
    {"MOS", "USA", "7360 tones", 1984, AT_PSG},                    /* Plus/4 */
    {"Atari", "USA", "MIKEY shift reg", 1989, AT_PSG},             /* Atari Lynx */
    {"Yamaha", "Japan", "YM2608 FM + SSG", 1986, AT_FM},           /* PC-98 */
    {"Yamaha", "Japan", "YM2610 FM + SSG", 1987, AT_FM},           /* Neo Geo */
    {"Toshiba", "Japan", "T6W28 tones", 1998, AT_PSG},             /* Neo Geo Pocket */
    {"Yamaha", "Japan", "Y8950 FM", 1984, AT_FM},                 /* MSX-Audio */
    {"Yamaha", "Japan", "YM2414 FM", 1987, AT_FM},                 /* Yamaha TX81Z */
    {"TI", "USA", "SN76477 sounds", 1978, AT_MISC},                /* SN76477 */
    {"Namco", "Japan", "CUS30 + YM2151", 1987, AT_WAVE},               /* Namco System 1 */
    {"Oki", "Japan", "MSM5232 organ", 1983, AT_MISC},              /* Korg Poly-800 */
    {"Ferranti", "UK", "ULA 1 bit beeper", 1982, AT_MISC},         /* ZX Spectrum */
};
static const char *const ADD_LABEL[4] = {"Used in", "Composers", "Games", "Trivia"};
static const char *const ADDX[CHIPS][4] = {                                  /* what a rested console tells you: where it was used, who wrote for it, what it played, a fact; each checked against two sources */
    {"Famicom, NES, and Punch-Out!! arcade", "Koji Kondo and Hirokazu Tanaka", "Super Mario Bros., The Legend of Zelda, and Super Mario Bros. 3", "The 2A03 is a 6502 CPU with decimal mode removed."},
    {"Game Boy, Game Boy Color, and Game Boy Advance (in Game Boy mode)", "Minako Hamano, Kazumi Totaka, and Junichi Masuda", "Tetris, Pokemon Red and Blue, and Link's Awakening", "The Game Boy has four sound channels: two pulse, one wave and one noise."},
    {"Master System, Game Gear, SG-1000, and ColecoVision", "Yuzo Koshiro and Tokuhiko Uwabo", "Alex Kidd in Miracle World, Phantasy Star, and Sonic the Hedgehog", "The SN76489 family also powered the BBC Micro and the IBM PCjr."},
    {"PC Engine, TurboGrafx-16, and SuperGrafx", "-", "Bonk's Adventure, Bomberman '93, and R-Type", "The HuC6280 is a 65C02 CPU with six sound channels built in."},
    {"Atari 2600 and Atari 7800", "-", "Adventure, Pitfall!, and Space Invaders", "Jay Miner designed the TIA, and later led the Amiga's engineering."},
    {"Famicom (Konami cartridges, Japan only)", "Jun Funahashi, Manami Matsumae, and Shigemasa Matsuo", "Akumajo Densetsu, Madara, and Esper Dream 2", "Only three games ever used the VRC6, all Famicom games from Japan."},
    {"ZX Spectrum 128, Amstrad CPC, MSX, and Atari ST (a YM2149 clone)", "Tim Follin and Ben Daglish", "Frogger, Moon Patrol, and Bomb Jack", "Gyruss has five AY-3-8910 chips on its board."},
    {"Mega Drive, Genesis, FM Towns, and Mega-Tech and Mega-Play arcade boards", "Yuzo Koshiro, Motohiro Kawashima, and Masato Nakamura", "Sonic the Hedgehog, Streets of Rage 2, and The Revenge of Shinobi", "The YM2612's sixth FM channel can be switched to play 8-bit samples."},
    {"Virtual Boy", "Hitoshi Fukushima, Kazumi Totaka, and Ken Kojima", "Mario's Tennis, Red Alarm, and Virtual Boy Wario Land", "-"},
    {"Atari 400/800/XL, Atari 5200, and Atari arcade boards", "Earl Vickers and Hal Canon", "Centipede, Gauntlet, and Ballblazer", "POKEY was designed by Doug Neubauer, who also made Star Raiders."},
    {"IBM PC, PC XT, and PC AT", "Al Lowe", "King's Quest II and Paratrooper", "The PC speaker is driven by channel 2 of the Intel 8253 or 8254 timer chip."},
    {"AdLib card, Sound Blaster 1.0, and Pro AudioSpectrum", "Bobby Prince", "Doom and King's Quest IV", "The original Pro AudioSpectrum used two OPL2 chips for stereo FM."},
    {"Pac-Man and Galaga arcade boards", "Toshio Kai and Nobuyuki Ohnogi", "Pac-Man and Galaga", "The original WSG has 3 voices and 8 waveforms of 32 four-bit steps."},
    {"Famicom cartridges (Japan only)", "-", "Mappy Kids, Rolling Thunder, and Megami Tensei II", "Most Namco 163 games used only 4 of its 8 wave channels."},
    {"Gimmick! on the Famicom, and nothing else", "Masashi Kageyama", "Gimmick!", "Mr. Gimmick, the 1993 Scandinavian NES release, drops the chip."},
    {"Famicom cartridges with the MMC5", "-", "Just Breed, Shin 4-Nin Uchi Mahjong, and Metal Slader Glory", "Shin 4-Nin Uchi Mahjong is the only game to use the MMC5 PCM channel."},
    {"Famicom Disk System", "Hirokazu Tanaka", "Metroid and Kid Icarus", "NES cartridge versions of Disk System games have no extra wave channel."},
    {"Konami MSX cartridges and Konami arcade boards", "Masahiro Ikariko and Tsuyoshi Sekito", "Nemesis 2, Metal Gear 2: Solid Snake, and Salamander (MSX)", "SCC channels 4 and 5 share one waveform."},
    {"MSX (FM-PAC), Master System (Japan), and arcade boards", "-", "Time Soldiers (arcade), Sky Soldiers, and Rampart (arcade)", "Its drum mode turns 3 of the 9 channels into 5 percussion sounds."},
    {"Famicom (Konami cartridge)", "Akio Dobashi", "Lagrange Point", "The VRC7 is a cut-down Yamaha YM2413 with mostly fixed instruments."},
    {"Sharp X68000, Sega arcade boards, Capcom CPS-1, and Konami arcade boards", "Hiroshi Kawaguchi, Yoko Shimomura, and Miki Higashino", "Out Run, Street Fighter II, and Salamander", "-"},
    {"Sound Blaster Pro 2, Pro AudioSpectrum 16, and AdLib Gold", "-", "-", "It has 18 two-operator FM channels, with up to six paired into four-operator voices."},
    {"WonderSwan, WonderSwan Color, and SwanCrystal", "Minako Adachi", "Final Fantasy, Gunpey, and Riviera: The Promised Land", "The WonderSwan was only ever released in Japan."},
    {"SAM Coupe, Creative Music System, and Game Blaster", "-", "-", "Each Creative Music System card carried two SAA1099 chips, for 12 voices."},
    {"Commodore VIC-20", "-", "-", "The VIC-20 was the first computer to sell one million units."},
    {"Commodore 16, Plus/4, and Commodore 116", "-", "-", "The TED chip made both the video and the sound of the Plus/4 and C16."},
    {"Atari Lynx", "Robert Vieira and Alex Rudis", "Electrocop, Blue Lightning, and Chip's Challenge", "Each of Mikey's four sound channels has its own 8-bit DAC."},
    {"NEC PC-88 and PC-98 computers (with the right sound board)", "Yuzo Koshiro", "The Scheme", "The YM2608 has 6 FM channels, 3 SSG channels, ADPCM and 6 rhythm sounds."},
    {"Neo Geo AES and MVS and Taito arcade boards", "Takushi Hiyamuta and Yasuo Yamate", "Metal Slug, Samurai Shodown, and Fatal Fury", "The Neo Geo's sound is run by a 4 MHz Z80 driving the YM2610."},
    {"Neo Geo Pocket and Neo Geo Pocket Color", "Takushi Hiyamuta", "Sonic Pocket Adventure, SNK vs. Capcom: Card Fighters' Clash, and Metal Slug 1st Mission", "The Neo Geo Pocket's chip is a stereo version of the SN76489."},
    {"MSX cartridges (Panasonic FS-CA1, Philips NMS-1205, Toshiba HX-MU900)", "-", "-", "The Y8950 is a YM3526 with ADPCM added."},
    {"Yamaha TX81Z, DX11, YS200, and Korg Z3", "-", "-", "The TX81Z was the first instrument to use the YM2414."},
    {"Space Invaders arcade, ABC 80, and Gakken EX-System", "-", "Space Invaders, Stratovox, and Vanguard", "The SN76477 has no registers: its sounds are set by outside resistors and capacitors."},
    {"Namco System 1, System 86, and the Thunder Ceptor board", "Junko Ozawa and Shinji Hosoe", "Pac-Mania, Dragon Spirit, and Splatterhouse", "-"},
    {"Korg Poly-800 and Taito arcade boards", "-", "-", "The Korg Poly-800 synth used an arcade sound chip, the Oki MSM5232."},
    {"ZX Spectrum 16K, 48K, and 128K", "Tim Follin", "Chronos (48K version)", "The Spectrum's beeper is one bit, toggled through bit 4 of port 254."},
};
static const char *const ADD_TYPE_NAME[4] = {"FM", "WAVETABLE", "PSG", "OTHER"};
static const char *const ADD_TYPE_TAG[4] = {"FM", "WAVE", "PSG", "MISC"};
static const char *const ADD_COUNTRY[4] = {"Japan", "USA", "UK", "Netherlands"};
static const char *const ADD_MODE[4] = {"TYPE", "YEAR", "MAKER", "COUNTRY"};
static const char ADD_NAME[CHIPS][15] = {"NES", "Game Boy", "Master System", "PC Engine", "Atari 2600", "Konami VRC6", "AY-3-8910", "Mega Drive", "Virtual Boy", "Atari 8-bit", "PC Speaker", "AdLib",
                                         "Namco WSG", "Namco 163", "Sunsoft 5B", "NES MMC5", "Famicom Disk", "Konami SCC", "MSX Music", "Konami VRC7", "Arcade FM", "Sound Blaster", "WonderSwan",
                                         "Sam Coupe", "VIC-20", "Plus/4", "Atari Lynx", "PC-98", "Neo Geo", "Neo Geo Pocket", "MSX-Audio", "Yamaha TX81Z", "SN76477", "Namco System 1", "Korg Poly-800", "ZX Spectrum"};

static uint8_t add_mode;                        /* 0 type, 1 year, 2 maker, 3 country: kept between visits */
static uint8_t add_row[CHIPS + 24];             /* the list: a console's number, or 0x80 | the heading's number */
static char add_head[24][14];
static uint32_t add_rows, add_cur, add_top, add_card, add_dirty;
static uint32_t add_rest_t0, add_info_on, add_info_n, add_info_phase, add_info_t0;
static int32_t add_info_y, add_info_end;        /* resting on a console: after a while its notes scroll up the strip */
static uint8_t add_info_par[40], add_info_off[40];                                        /* each wrapped line: its paragraph, and where in the paragraph's text it starts */
static uint16_t add_info_len[40];
#define ADD_REST_MS 5000u

static int add_cmp(const char *p, const char *q) { while (*p && *p == *q) { p++; q++; } return (int)(uint8_t)*p - (int)(uint8_t)*q; }
static uint32_t add_key(uint32_t ch)            /* the group a console falls in, in the order the groups run */
{
    const addinfo_t *a = &ADDI[ch];
    uint32_t i, k = 0;
    if (add_mode == 0u) return a->type;
    if (add_mode == 1u) return a->year / 10u;
    if (add_mode == 2u) {                       /* the makers in alphabetical order: how many other makers sort before this one */
        for (i = 0; i < CHIPS; i++) {
            uint32_t j, first = 1;
            if (add_cmp(ADDI[i].maker, a->maker) >= 0) continue;
            for (j = 0; j < i; j++) if (add_cmp(ADDI[j].maker, ADDI[i].maker) == 0) { first = 0; break; }
            k += first;
        }
        return k;
    }
    for (i = 0; i < 4u; i++) { const char *p = ADD_COUNTRY[i], *q = a->country; while (*p && *p == *q) { p++; q++; } if (*p == *q) return i; }
    return 4;
}
static void add_head_text(char *d, uint32_t ch)
{
    const addinfo_t *a = &ADDI[ch];
    const char *s = add_mode == 0u ? ADD_TYPE_NAME[a->type] : add_mode == 2u ? a->maker : a->country;
    uint32_t i = 0;
    if (add_mode == 1u) { d[0] = (char)('0' + a->year / 1000u); d[1] = (char)('0' + a->year / 100u % 10u); d[2] = (char)('0' + a->year / 10u % 10u); d[3] = '0'; d[4] = 's'; d[5] = 0; return; }
    while (s[i] && i < 13u) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void add_build(void)                     /* the rows for the mode: a heading, then its consoles by year */
{
    uint8_t used[CHIPS];
    uint32_t i, j, n = 0, heads = 0;
    memset(used, 0, sizeof used);
    for (;;) {
        uint32_t best = 255, key = 0, hn;
        for (i = 0; i < CHIPS; i++) if (!used[i]) { uint32_t k = add_key(i); if (best == 255u || k < key) { best = i; key = k; } }
        if (best == 255u) break;
        hn = heads < 24u ? heads++ : 23u;
        add_head_text(add_head[hn], best);
        add_row[n++] = (uint8_t)(0x80u | hn);
        for (;;) {                              /* this group's consoles, the oldest first */
            uint32_t pick = 255;
            for (i = 0; i < CHIPS; i++) if (!used[i] && add_key(i) == key && (pick == 255u || ADDI[i].year < ADDI[pick].year)) pick = i;
            if (pick == 255u) break;
            used[pick] = 1; add_row[n++] = (uint8_t)pick;
        }
    }
    add_rows = n;
    (void)j;
}
static uint32_t add_chip(void) { return add_cur < add_rows && !(add_row[add_cur] & 0x80u) ? add_row[add_cur] : 0u; }
static uint32_t add_place(uint32_t chip)        /* the bar on a console, the list scrolled to it */
{
    uint32_t i, y;
    for (i = 0; i < add_rows; i++) if (add_row[i] == chip) { add_cur = i; break; }
    if (add_cur < add_top) add_top = add_cur;
    if (add_top > 0u && add_cur == add_top && (add_row[add_cur - 1u] & 0x80u)) add_top--;      /* its heading shows */
    for (;;) {                                  /* scroll down until it fits above the strip */
        y = 22u;
        for (i = add_top; i <= add_cur; i++) y += (add_row[i] & 0x80u) ? 17u : 19u;
        if (y <= 166u || add_top >= add_cur) break;
        add_top++;
    }
    add_dirty = 1;
    return 0;
}
static void add_touch(void) { add_rest_t0 = fm1_ms; if (add_info_on) { add_info_on = 0; menu_dirty = 1; } }       /* something happened: the strip goes back to the picture */
static void add_open(uint32_t chip)             /* the list on a console (0: the first) */
{
    menu_open_pick(ADD_MENU, sizeof ADD_MENU / sizeof ADD_MENU[0]); menu_kind = MK_ADD;
    add_build(); add_card = 0; add_top = 0; add_cur = 0;
    add_place(chip < CHIPS ? chip : add_row[1]);
    add_touch();
    menu_dirty = 1;
}
static void add_scroll(int32_t d)               /* SELECT / PRESETS: the next console, past the headings; no wrap */
{
    uint32_t v = add_cur;
    while (d < 0 ? v > 0u : v + 1u < add_rows) { v = d < 0 ? v - 1u : v + 1u; if (!(add_row[v] & 0x80u)) break; }
    if (add_row[v] & 0x80u) return;
    add_cur = v; add_place(add_row[v]); add_touch(); menu_dirty = 1;
}
static void add_mode_step(int32_t d)            /* KNOB1: the next way to group them; the bar stays on its console */
{
    uint32_t chip = add_chip();
    add_mode = (uint8_t)(d > 0 ? (add_mode + 1u) & 3u : (add_mode + 3u) & 3u);
    add_build(); add_top = 0; add_cur = 0; add_place(chip); add_touch(); menu_dirty = 1;
}

/* ---- the audition: play the keys and the console on the bar answers. A hidden channel on its first voice is added with the first key and gone again when the bar moves on or the list closes */
static void add_aud_stop(void)
{
    if (add_aud_c >= 0) { chip_off((uint32_t)add_aud_c); chan_remove((uint32_t)add_aud_c); add_aud_c = -1; menu_dirty = 1; }
}
static uint32_t add_aud_kind(void)              /* the voice on the bar: a console's first, or the channel picked in the second list (not drums); 255 none */
{
    uint32_t kind = 255;
    if (menu_kind == MK_ADD) kind = ADD_SUB[add_chip()].kind[0];
    else if (menu_kind == MK_ADD2 && menu_sel >= 1u && menu_sel <= ADD_SUB[add_console].n - 1u) kind = ADD_SUB[add_console].kind[menu_sel - 1u];
    return kind != 255u && IS_DRUM(kind) ? 255u : kind;
}
static void add_key_on(uint32_t note)
{
    uint32_t kind = add_aud_kind();
    add_touch();
    if (kind == 255u) return;
    if (add_aud_c >= 0 && chan[add_aud_c].kind != kind) add_aud_stop();
    if (add_aud_c < 0) { int c; if (chan_n >= MAXCH) return; c = chan_add(kind); if (c < 0) return; add_aud_c = c; }
    chip_note((uint32_t)add_aud_c, note, 127u);
}
static void add_key_off(uint32_t note) { (void)note; if (add_aud_c >= 0) chip_off((uint32_t)add_aud_c); }
static void add_aud_tick(void)                  /* main loop: the channel on loan goes when the bar moves to another voice, or the list is left */
{
    if (!menu_on || (menu_kind != MK_ADD && menu_kind != MK_ADD2)) { add_aud_stop(); return; }
    if (add_aud_c >= 0 && chan[add_aud_c].kind != add_aud_kind()) add_aud_stop();
}

/* ---- the notes that scroll up the strip after a rest */
static void add_info_wrap(uint32_t ch)           /* the three paragraphs, cut into lines that fit the strip */
{
    uint32_t p;
    add_info_n = 0;
    for (p = 0; p < 4u; p++) {
        char full[200], line[48];
        uint32_t n = 0, i = 0, ls = 0, ll = 0, labn;
        const char *l = ADD_LABEL[p], *b = ADDX[ch][p];
        if (!b || !b[0] || (b[0] == '-' && !b[1])) continue;
        while (*l && n < 150u) full[n++] = *l++;
        full[n++] = ':'; full[n++] = ' ';
        labn = n;
        while (*b && n < 190u) full[n++] = *b++;
        full[n] = 0;
        while (i < n && add_info_n < 38u) {          /* greedy: as many words as fit */
            uint32_t j = i, lastsp = 0;
            ll = 0;
            while (j < n) {
                line[ll++] = full[j++];
                line[ll] = 0;
                if (full[j - 1u] == ' ') lastsp = ll;
                if (text_w(&FONT_S, line) > 222) { if (lastsp) { j -= ll - lastsp; ll = lastsp; } else { j--; ll--; } break; }
                if (ll > 44u) break;
            }
            while (ll && line[ll - 1u] == ' ') ll--;
            add_info_par[add_info_n] = (uint8_t)p; add_info_off[add_info_n] = (uint8_t)i; add_info_len[add_info_n] = (uint16_t)ll;
            add_info_n++;
            i = j; ls = 0;
            while (i < n && full[i] == ' ') i++;
        }
        (void)labn; (void)ls;
    }
}
static void add_info_draw(void)                  /* the strip alone: the notes a pixel higher each time */
{
    const uint32_t cur = add_chip();
    const theme_t *t = &THEME[cur];
    int32_t y;
    uint32_t i;
    cv_begin(240, 72, t->bg);
    cv_rect(0, 0, 240, 2, t->acc);
    y = add_info_y;
    for (i = 0; i < add_info_n; i++) {
        char full[200], buf[48];
        uint32_t p = add_info_par[i], n = 0, k;
        const char *l = ADD_LABEL[p], *b = ADDX[cur][p];
        while (*l) full[n++] = *l++;
        full[n++] = ':'; full[n++] = ' ';
        while (*b && n < 190u) full[n++] = *b++;
        full[n] = 0;
        for (k = 0; k < add_info_len[i] && k < 47u; k++) buf[k] = full[add_info_off[i] + k];
        buf[k] = 0;
        if (y > -16 && y < 72) {
            if (add_info_off[i] == 0u) {         /* a paragraph's first line: its label in the console's colour */
                uint32_t ll = 0, x;
                while (ADD_LABEL[p][ll]) ll++;
                { char lb[16]; uint32_t q; for (q = 0; q < ll + 1u && q < 15u; q++) lb[q] = buf[q]; lb[q] = 0; x = cv_text(8, y, &FONT_S, lb, t->acc); cv_text(x + 4, y, &FONT_S, buf + ll + 1u, t->fg); }
            } else cv_text(8, y, &FONT_S, buf, t->fg);
        }
        y += 15 + ((i + 1u < add_info_n && add_info_par[i + 1u] != p) ? 7 : 0);
    }
    add_info_end = y;                            /* where the block ends */
    cv_blit(0, 168);
}
static void add_info_tick(void)                  /* main loop: 5 s of rest starts it: the notes slide up, rest, scroll on if there is more, rest, and the picture is back */
{
    static uint32_t last;
    if (!menu_on || menu_kind != MK_ADD || add_card) { add_info_on = 0; return; }
    if (!add_info_on) {
        if (fm1_ms - add_rest_t0 >= ADD_REST_MS) {
            add_info_wrap(add_chip());
            if (!add_info_n) { add_rest_t0 = fm1_ms; return; }
            add_info_on = 1; add_info_phase = 0; add_info_y = 74; last = fm1_ms; add_info_t0 = fm1_ms; add_info_draw();
        }
        return;
    }
    if (add_info_phase == 0u) {                  /* in from below */
        if (fm1_ms - last >= 25u) { last = fm1_ms; add_info_y -= 4; if (add_info_y <= 8) { add_info_y = 8; add_info_phase = 1; add_info_t0 = fm1_ms; } add_info_draw(); }
    } else if (add_info_phase == 1u) {           /* rest, then on if it does not all fit */
        if (fm1_ms - add_info_t0 >= 2000u) { add_info_phase = add_info_end > 70 ? 2u : 3u; add_info_t0 = fm1_ms; last = fm1_ms; }
    } else if (add_info_phase == 2u) {           /* a pixel at a time until the last line is in view */
        if (fm1_ms - last >= 45u) { last = fm1_ms; add_info_y--; add_info_draw(); if (add_info_end <= 66) { add_info_phase = 3; add_info_t0 = fm1_ms; } }
    } else if (fm1_ms - add_info_t0 >= 2500u) {  /* the end: the picture again, and 5 s before it comes round once more */
        add_info_on = 0; add_rest_t0 = fm1_ms; menu_dirty = 1;
    }
}

/* ---- drawing */
static void add_tag(int32_t x, int32_t y, const char *s, uint16_t col)
{
    int32_t w = text_w(&FONT_S, s) + 8;
    cv_rect(x, y, w, 14, col); cv_rect(x + 1, y + 1, w - 2, 12, R_BG);
    cv_text(x + 4, y, &FONT_S, s, col);
}
static void add_year(char *d, uint32_t y) { d[0] = (char)('0' + y / 1000u); d[1] = (char)('0' + y / 100u % 10u); d[2] = (char)('0' + y / 10u % 10u); d[3] = (char)('0' + y % 10u); d[4] = 0; }
static void add_icon(uint32_t chip, int32_t x, int32_t y, int32_t sc, uint16_t bg)       /* the console's picture, drawn bigger */
{
    uint16_t was = hdr_bg;
    hdr_bg = bg;
    cv_sc = sc; cv_sx = x; cv_sy = y;
    ico_console(chip, 0, 0);
    cv_sc = 1; cv_sx = 0; cv_sy = 0;
    hdr_bg = was;
}
static void add_body(uint16_t accent)
{
    const uint32_t cur = add_chip();
    const theme_t *t = &THEME[cur];
    char yb[8];
    int32_t y = 22;
    uint32_t i;
    if (add_card) {                             /* the card: SEL opens it, SELECT turns the pages, OCT+ adds, OCT- or SEL closes it */
        uint32_t rank = 0;
        char nb[10];
        cv_rect(0, 0, 240, 240, t->bg);
        skin_box(cur, t, 6, 6, 228, 228, t->bar);
        ico_logo(cur, 16, 16);
        for (i = 0; i <= add_cur; i++) if (!(add_row[i] & 0x80u)) rank++;
        nb[0] = 0; { uint32_t k = 0; if (rank >= 10u) nb[k++] = (char)('0' + rank / 10u); nb[k++] = (char)('0' + rank % 10u); nb[k++] = ' '; nb[k++] = 'o'; nb[k++] = 'f'; nb[k++] = ' '; nb[k++] = (char)('0' + CHIPS / 10u); nb[k++] = (char)('0' + CHIPS % 10u); nb[k] = 0; }
        cv_text(240 - 16 - text_w(&FONT_S, nb), 16, &FONT_S, nb, t->dim);
        add_icon(cur, 63, 44, 3, t->bar);
        cv_text(120 - text_w(&FONT_L, ADD_NAME[cur]) / 2, 120, &FONT_L, ADD_NAME[cur], t->fg);
        add_year(yb, ADDI[cur].year);
        { int32_t w = text_w(&FONT_S, yb) + text_w(&FONT_S, "  ") * 2 + text_w(&FONT_S, ADDI[cur].maker) + text_w(&FONT_S, ADDI[cur].country), x = 120 - w / 2;
          x = cv_text(x, 154, &FONT_S, yb, t->acc); x = cv_text(x, 154, &FONT_S, "  ", t->fg); x = cv_text(x, 154, &FONT_S, ADDI[cur].maker, t->fg); x = cv_text(x, 154, &FONT_S, "  ", t->fg); cv_text(x, 154, &FONT_S, ADDI[cur].country, t->fg); }
        {   char fl[40]; uint32_t q; for (q = 0; q < 39u && ADDI[cur].info[q]; q++) fl[q] = ADDI[cur].info[q] == '\n' ? ' ' : ADDI[cur].info[q];       /* a newline in the info is a space here */
            fl[q] = 0; cv_text(120 - text_w(&FONT_S, fl) / 2, 174, &FONT_S, fl, t->dim); }
        if (add_aud_c >= 0) channel_glyph((uint32_t)add_aud_c, 20, 194, 200, 14, t->acc);
        else cv_rect(20, 201, 200, 1, t->dim);
        cv_text(20, 214, &FONT_S, "OCT+ Add", t->fg); cv_text(240 - 20 - text_w(&FONT_S, "OCT- Back"), 214, &FONT_S, "OCT- Back", t->dim);
        return;
    }
    cv_text(8, 2, &FONT_S, "Add Channel", accent);
    {   const int32_t tw = text_w(&FONT_S, ADD_MODE[add_mode]) + 8, kw = text_w(&FONT_S, "<K1>"), x0 = 234 - (tw + 6 + kw);       /* the mode and the knob that turns it, flush right whatever the word */
        add_tag(x0, 4, ADD_MODE[add_mode], accent);
        cv_text(x0 + tw + 6, 2, &FONT_S, "<K1>", RGB(110, 110, 124));
    }
    for (i = add_top; i < add_rows && y < 166; i++) {
        if (add_row[i] & 0x80u) {
            const char *h = add_head[add_row[i] & 0x7Fu];
            cv_text(8, y + 1, &FONT_S, h, RGB(150, 150, 164));
            cv_rect(8 + text_w(&FONT_S, h) + 6, y + 9, 200 - text_w(&FONT_S, h), 1, RGB(60, 60, 74));
            y += 17;
        } else {
            const uint32_t ch = add_row[i];
            if (i == add_cur) cv_rect(0, y - 1, 240, 19, R_SEL);
            cv_rect(6, y + 3, 3, 11, THEME[ch].acc);
            cv_text(14, y + 1, &FONT_S, ADD_NAME[ch], i == add_cur ? R_WHITE : RGB(206, 206, 214));
            if (add_mode == 1u) { add_year(yb, ADDI[ch].year); cv_text(240 - 8 - text_w(&FONT_S, yb), y + 1, &FONT_S, yb, RGB(150, 150, 164)); }
            else add_tag(240 - 10 - (text_w(&FONT_S, ADD_TYPE_TAG[ADDI[ch].type]) + 8), y + 2, ADD_TYPE_TAG[ADDI[ch].type], THEME[ch].acc);
            y += 19;
        }
    }
    cv_rect(0, 168, 240, 72, t->bg); cv_rect(0, 168, 240, 2, t->acc);
    {
        add_icon(cur, 8, 180, 2, t->bg);
        cv_text(94, 175, &FONT_S, ADD_NAME[cur], t->fg);
        { int32_t x; add_year(yb, ADDI[cur].year); x = cv_text(94, 194, &FONT_S, yb, t->acc); x = cv_text(x, 194, &FONT_S, "  ", t->fg); cv_text(x, 194, &FONT_S, ADDI[cur].maker, t->fg); }
        {   char l1[40]; const char *in = ADDI[cur].info; uint32_t n = 0, sp = 0;       /* what does not fit beside the icon goes on a second line */
            while (in[n] && in[n] != '\n' && n < 39u) { l1[n] = in[n]; l1[n + 1u] = 0; if (text_w(&FONT_S, l1) > 140) break; if (in[n] == ' ') sp = n; n++; }
            if (in[n] == '\n') sp = n;
            if (!in[n]) cv_text(94, 213, &FONT_S, in, t->dim);
            else { l1[sp] = 0; cv_text(94, 210, &FONT_S, l1, t->dim); cv_text(94, 223, &FONT_S, in + sp + 1u, t->dim); }
        }
    }
}
static void add_draw(uint16_t accent)
{
    uint32_t b;
    for (b = 0; b < 3u; b++) {
        cv_begin(240, 80, R_BG);
        cv_oy = -(int32_t)b * 80;
        add_body(accent);
        cv_oy = 0;
        cv_blit(0, b * 80u);
    }
}
