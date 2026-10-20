/* SPDX-License-Identifier: GPL-3.0-only */
/* Projects: 8 slots in flash, each with two banks. A save writes the bank that is not the newest and puts its header
 * (magic, number, length, crc) last, so a power cut mid-save leaves the old version whole. Included by chipbox.c.
 *   bank: [header 16 B][payload]; payload: version, tempo, mutes, page lengths, song, every channel's sound, then the steps
 * An erase stalls the sound (~50 ms a sector), so saves happen when stopped, or when you press SAVE. */
#define PROJ_MAGIC 0x31424A50u                  /* "PJB1" */
#define PROJ_HDR 16u
#define PROJ_USED 9u                            /* channels in use: 3 per chip */
#define PROJ_STEPS_AT 126u                      /* where the steps start in the payload */

static uint8_t proj_buf[PROJ_BANK];
static uint8_t proj_cur;                        /* the open slot */
static uint32_t proj_dirty, proj_t_edit, proj_t_saved, proj_t_fail, proj_loaded;   /* proj_loaded: the open project came from flash */
static inst_t inst_def[CHIPS][3];               /* the sounds a blank project starts with */

static uint32_t proj_crc(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu, k;
    while (n--) {
        c ^= *p++;
        for (k = 0; k < 8u; k++) c = c & 1u ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}
static void proj_edited(void) { proj_dirty = 1; proj_t_edit = fm1_ms; }

static uint32_t proj_pack(uint8_t *p)           /* the project to bytes: the length */
{
    uint32_t n = 0, i, pg, c, ch, s;
    p[n++] = 1;
    p[n++] = (uint8_t)seq_bpm; p[n++] = (uint8_t)(seq_bpm >> 8);
    p[n++] = (uint8_t)seq_mute; p[n++] = (uint8_t)(seq_mute >> 8);
    for (pg = 0; pg < SEQ_PATS; pg++) p[n++] = pat_len[pg];
    p[n++] = song_len;
    for (i = 0; i < 16u; i++) p[n++] = song[i];
    p[n++] = seq_song_on;
    for (c = 0; c < CHIPS; c++)
        for (ch = 0; ch < 3u; ch++) {
            const inst_t *x = &inst[c][ch];
            p[n++] = x->wave; p[n++] = x->duty; p[n++] = x->atk; p[n++] = x->dec; p[n++] = x->sus; p[n++] = x->rel;
            p[n++] = x->vib_spd; p[n++] = x->vib_dep; p[n++] = (uint8_t)x->sweep; p[n++] = x->pan; p[n++] = x->vol;
        }
    for (pg = 0; pg < SEQ_PATS; pg++)
        for (c = 0; c < CHIPS; c++)
            for (ch = 0; ch < 3u; ch++)
                for (s = 0; s < pat_len[pg]; s++) {
                    const step_t *x = &pat[pg][c * 4u + ch][s];
                    p[n++] = x->note; p[n++] = x->vel; p[n++] = x->gate; p[n++] = x->arp;
                }
    return n;
}
static int proj_unpack(const uint8_t *p, uint32_t len)      /* 0: loaded; else nothing was changed */
{
    uint32_t n = 0, i, pg, c, ch, s, need;
    uint8_t lens[SEQ_PATS];
    if (len < PROJ_STEPS_AT || p[0] != 1u) return -1;
    for (pg = 0; pg < SEQ_PATS; pg++) {
        lens[pg] = p[5u + pg];
        if (lens[pg] < 1u || lens[pg] > SEQ_MAXLEN) return -1;
    }
    need = PROJ_STEPS_AT;
    for (pg = 0; pg < SEQ_PATS; pg++) need += lens[pg] * PROJ_USED * 4u;
    if (len < need) return -1;
    seq_bpm = (uint32_t)p[1] | (uint32_t)p[2] << 8;
    seq_mute = (uint16_t)(p[3] | p[4] << 8);
    for (pg = 0; pg < SEQ_PATS; pg++) pat_len[pg] = lens[pg];
    song_len = p[9] > 16u ? 16u : p[9];
    for (i = 0; i < 16u; i++) song[i] = p[10u + i];
    seq_song_on = p[26];
    n = 27;
    for (c = 0; c < CHIPS; c++)
        for (ch = 0; ch < 3u; ch++) {
            inst_t *x = &inst[c][ch];
            x->wave = p[n]; x->duty = p[n + 1]; x->atk = p[n + 2]; x->dec = p[n + 3]; x->sus = p[n + 4]; x->rel = p[n + 5];
            x->vib_spd = p[n + 6]; x->vib_dep = p[n + 7]; x->sweep = (int8_t)p[n + 8]; x->pan = p[n + 9]; x->vol = p[n + 10];
            n += 11;
        }
    memset(pat, 0, sizeof pat);
    for (pg = 0; pg < SEQ_PATS; pg++)
        for (c = 0; c < CHIPS; c++)
            for (ch = 0; ch < 3u; ch++)
                for (s = 0; s < pat_len[pg]; s++) {
                    step_t *x = &pat[pg][c * 4u + ch][s];
                    x->note = p[n]; x->vel = p[n + 1]; x->gate = p[n + 2]; x->arp = p[n + 3];
                    n += 4;
                }
    seq_view = 0; seq_pat = 0; seq_next = 255; seq_pos = 0;
    return 0;
}
static void proj_blank(void)                    /* an empty project */
{
    uint32_t pg;
    memset(pat, 0, sizeof pat);
    for (pg = 0; pg < SEQ_PATS; pg++) pat_len[pg] = 16;
    song_len = 4; song[0] = 0; song[1] = 1; song[2] = 2; song[3] = 3; seq_song_on = 0;
    seq_bpm = 120; seq_mute = 0; seq_view = 0; seq_pat = 0; seq_next = 255; seq_pos = 0;
    memcpy(inst, inst_def, sizeof inst);
}

/* ---- the banks */
static uint32_t proj_base(uint32_t slot, uint32_t bank) { return PROJ_AT + (slot * 2u + bank) * PROJ_BANK; }
typedef struct { uint32_t magic, seq, len, crc; } proj_hdr_t;
static int proj_header(uint32_t slot, uint32_t bank, proj_hdr_t *h)    /* 1: a bank that looks written */
{
    if (pf_read(proj_base(slot, bank), h, sizeof *h)) return 0;
    return h->magic == PROJ_MAGIC && h->len > 0u && h->len <= PROJ_BANK - PROJ_HDR;
}
static int proj_newest(uint32_t slot, proj_hdr_t *h)           /* the newer bank: 0 or 1, or -1 when the slot is empty */
{
    proj_hdr_t a, b;
    int ha = proj_header(slot, 0, &a), hb = proj_header(slot, 1, &b);
    if (ha && hb) { if ((int32_t)(b.seq - a.seq) > 0) { *h = b; return 1; } *h = a; return 0; }
    if (ha) { *h = a; return 0; }
    if (hb) { *h = b; return 1; }
    return -1;
}
static int proj_bank_ok(uint32_t slot, uint32_t bank, const proj_hdr_t *h)    /* the payload into proj_buf and its crc */
{
    if (pf_read(proj_base(slot, bank) + PROJ_HDR, proj_buf, h->len)) return 0;
    return proj_crc(proj_buf, h->len) == h->crc;
}
static int proj_exists(uint32_t slot) { proj_hdr_t h; return proj_newest(slot, &h) >= 0; }

static int proj_load(uint32_t slot)             /* 0: the slot's project is now the open one's data */
{
    proj_hdr_t h;
    int b = proj_newest(slot, &h);
    uint32_t tries;
    for (tries = 0; b >= 0 && tries < 2u; tries++, b ^= 1) {       /* the newer bank, then the older if that one is damaged */
        if (tries && !proj_header(slot, (uint32_t)b, &h)) continue;
        if (proj_bank_ok(slot, (uint32_t)b, &h) && proj_unpack(proj_buf, h.len) == 0) return 0;
    }
    return -1;
}
static int proj_save(uint32_t slot)
{
    proj_hdr_t h, nh;
    int b = proj_newest(slot, &h);
    uint32_t bank = b < 0 ? 0u : (uint32_t)(b ^ 1), len = proj_pack(proj_buf), i, sectors, base = proj_base(slot, bank);
    sectors = (PROJ_HDR + len + 0xFFFu) / 0x1000u;
    for (i = 0; i < sectors; i++) if (pf_erase(base + i * 0x1000u)) return -1;
    if (pf_write(base + PROJ_HDR, proj_buf, len)) return -1;
    nh.magic = PROJ_MAGIC; nh.seq = b < 0 ? 1u : h.seq + 1u; nh.len = len; nh.crc = proj_crc(proj_buf, len);
    if (pf_write(base, &nh, sizeof nh)) return -1;
    return 0;
}
static void proj_open(uint32_t slot)            /* switch projects: stop, keep what is unsaved, load (or start blank) */
{
    seq_stop();
    if (proj_dirty && proj_save(proj_cur) == 0) proj_dirty = 0;
    proj_loaded = proj_load(slot) == 0;
    if (!proj_loaded) proj_blank();
    proj_cur = (uint8_t)slot; proj_dirty = 0;
}
static void proj_clear(uint32_t slot)           /* an empty slot: both banks' first sector */
{
    pf_erase(proj_base(slot, 0));
    pf_erase(proj_base(slot, 1));
}

/* what a slot looks like in the picker: per channel, a bit a step on page A */
static void proj_preview(uint32_t slot, uint16_t *rows)
{
    uint32_t c, s, len0 = 0;
    proj_hdr_t h;
    int b;
    memset(rows, 0, PROJ_USED * sizeof rows[0]);
    if (slot == proj_cur) {                     /* the open one: what is in memory, saved or not */
        for (c = 0; c < PROJ_USED; c++)
            for (s = 0; s < 16u && s < pat_len[0]; s++)
                if (pat[0][(c / 3u) * 4u + c % 3u][s].note) rows[c] |= (uint16_t)(1u << s);
        return;
    }
    b = proj_newest(slot, &h);
    if (b < 0 || pf_read(proj_base(slot, (uint32_t)b) + PROJ_HDR + 5u, &len0, 1)) return;
    len0 &= 0xFFu;
    if (len0 < 1u || len0 > SEQ_MAXLEN) return;
    for (c = 0; c < PROJ_USED; c++) {
        static uint8_t buf[SEQ_MAXLEN * 4u];
        if (pf_read(proj_base(slot, (uint32_t)b) + PROJ_HDR + PROJ_STEPS_AT + c * len0 * 4u, buf, len0 * 4u)) return;
        for (s = 0; s < 16u && s < len0; s++)
            if (buf[s * 4u]) rows[c] |= (uint16_t)(1u << s);
    }
}
