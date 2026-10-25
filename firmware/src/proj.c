/* SPDX-License-Identifier: GPL-3.0-only */
/* Projects: 8 slots in flash, each with two banks. A save writes the bank that is not the newest and puts its header
 * (magic, number, length, crc) last, so a power cut mid-save leaves the old version whole. Included by chipbox.c.
 *   bank: [header 16 B][payload]; payload: version, tempo, pages, channels (kind and sound), then every note as one event
 * An erase stalls the sound (~50 ms a sector), so saves happen when stopped, or when you press SAVE. */
#define PROJ_MAGIC 0x32424A50u                  /* "PJB2" */
#define PROJ_HDR 16u
#define PROJ_INST 17u                           /* bytes of a channel's sound (version 2 had 14: no wave fold, crush, twist) */
#define PROJ_EV 6u                              /* bytes of a note: page, channel, step, note, velocity, length */

static uint8_t proj_buf[PROJ_BANK];
static uint8_t proj_cur;                        /* the open slot */
static uint32_t proj_dirty, proj_t_edit, proj_t_saved, proj_t_fail, proj_loaded;   /* proj_loaded: the open project came from flash */

static uint32_t proj_crc(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu, k;
    while (n--) {
        c ^= *p++;
        for (k = 0; k < 8u; k++) c = c & 1u ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}
static uint32_t undo_open, undo_t;               /* a group of edits is open: it closes after a quiet moment */
static void undo_push(void);
static void proj_edited(void) { if (!undo_open) undo_push(); undo_open = 1; undo_t = fm1_ms; proj_dirty = 1; proj_t_edit = fm1_ms; }

static uint32_t proj_pack(uint8_t *p)           /* the project to bytes: the length, or 0 when it does not fit a bank */
{
    uint32_t n = 0, pg, c, s, ev = 0, evat;
    p[n++] = 3;
    p[n++] = (uint8_t)seq_bpm; p[n++] = (uint8_t)(seq_bpm >> 8);
    p[n++] = (uint8_t)page_n; p[n++] = (uint8_t)chan_n;
    for (c = 0; c < chan_n; c++) {
        const inst_t *x = &chan[c].in;
        p[n++] = chan[c].kind;
        p[n++] = x->wave; p[n++] = x->duty; p[n++] = x->atk; p[n++] = x->dec; p[n++] = x->sus; p[n++] = x->rel;
        p[n++] = x->vib_spd; p[n++] = x->vib_dep; p[n++] = (uint8_t)x->sweep; p[n++] = x->pan; p[n++] = x->vol;
        p[n++] = x->arp; p[n++] = x->arp_rate; p[n++] = x->arp_dir;
        p[n++] = x->wf; p[n++] = x->wc; p[n++] = x->wt;
    }
    evat = n; n += 2;
    for (pg = 0; pg < page_n; pg++)
        for (c = 0; c < chan_n; c++)
            for (s = 0; s < SEQ_LEN; s++) {
                const step_t *x = &pat[pg][c][s];
                if (!x->note && !(IS_DRUM(chan[c].kind) && (x->vel & 7u))) continue;
                if (n + PROJ_EV > PROJ_BANK - PROJ_HDR) return 0;
                p[n++] = (uint8_t)pg; p[n++] = (uint8_t)c; p[n++] = (uint8_t)s; p[n++] = x->note; p[n++] = x->vel; p[n++] = x->gate;
                ev++;
            }
    p[evat] = (uint8_t)ev; p[evat + 1u] = (uint8_t)(ev >> 8);
    if (n + 1u + song_len > PROJ_BANK - PROJ_HDR) return 0;
    p[n++] = (uint8_t)song_len;                 /* the song: a page for each slot */
    for (s = 0; s < song_len; s++) p[n++] = song[s];
    return n;
}
static int proj_check(const uint8_t *p, uint32_t len)       /* 0: these bytes are a project this firmware can load */
{
    uint32_t n, i, c, ev, pages, chans, isz;
    if (len < 7u || (p[0] != 2u && p[0] != 3u)) return -1;
    isz = p[0] == 2u ? 14u : PROJ_INST;
    pages = p[3]; chans = p[4];
    if (pages < 1u || pages > SEQ_PAGES || chans > MAXCH) return -1;
    n = 5u + chans * (1u + isz);
    if (len < n + 2u) return -1;
    for (c = 0; c < chans; c++) if (p[5u + c * (1u + isz)] >= KINDS) return -1;
    ev = (uint32_t)p[n] | (uint32_t)p[n + 1u] << 8;
    n += 2u;
    if (len < n + ev * PROJ_EV) return -1;
    for (i = 0; i < ev; i++) {
        const uint8_t *e = p + n + i * PROJ_EV;
        if (e[0] >= pages || e[1] >= chans || e[2] >= SEQ_LEN) return -1;
    }
    return 0;
}
static int proj_unpack(const uint8_t *p, uint32_t len)      /* 0: loaded; else nothing was changed */
{
    uint32_t n, i, c, ev, pages, chans, isz;
    if (proj_check(p, len)) return -1;
    isz = p[0] == 2u ? 14u : PROJ_INST;
    pages = p[3]; chans = p[4];
    n = 5u + chans * (1u + isz);
    ev = (uint32_t)p[n] | (uint32_t)p[n + 1u] << 8;
    n += 2u;
    seq_stop();
    seq_clear();                                /* (the voices are silent while this runs; a sound is read when a note starts) */
    seq_bpm = (uint32_t)p[1] | (uint32_t)p[2] << 8;
    page_n = pages;
    for (c = 0; c < chans; c++) {
        const uint8_t *q = p + 5u + c * (1u + isz);
        chan_t *x;
        chan_add(q[0]);
        x = &chan[c];
        x->in.wave = q[1]; x->in.duty = q[2]; x->in.atk = q[3]; x->in.dec = q[4]; x->in.sus = q[5]; x->in.rel = q[6];
        x->in.vib_spd = q[7]; x->in.vib_dep = q[8]; x->in.sweep = (int8_t)q[9]; x->in.pan = q[10]; x->in.vol = q[11];
        x->in.arp = q[12]; x->in.arp_rate = q[13]; x->in.arp_dir = q[14];
        if (isz == 14u) { static const uint8_t OLD[4] = {0, 5, 10, 15}; x->in.wave = OLD[q[1] & 3u]; }       /* version 2: wave was one of four presets */
        else { x->in.wf = q[15] & 15u; x->in.wc = q[16] & 15u; x->in.wt = q[17] & 15u; }
    }
    for (i = 0; i < ev; i++) {
        const uint8_t *e = p + n + i * PROJ_EV;
        step_t *x = &pat[e[0]][e[1]][e[2]];
        x->note = e[3]; x->vel = e[4]; x->gate = e[5];
    }
    n += ev * PROJ_EV;
    if (n < len && p[n] >= 1u && p[n] <= SONG_MAX && n + 1u + p[n] <= len) {       /* the song (older projects have none: the pages in order) */
        song_len = p[n];
        for (i = 0; i < song_len; i++) song[i] = p[n + 1u + i] < pages ? p[n + 1u + i] : (uint8_t)(pages - 1u);
    } else {
        song_len = pages < SONG_MAX ? pages : SONG_MAX;
        for (i = 0; i < song_len; i++) song[i] = (uint8_t)i;
    }
    return 0;
}
/* undo: the project as bytes before each group of edits, the last UNDO_N of them. LFO steps back one. */
#define UNDO_N 6u
static uint8_t undo_buf[UNDO_N][PROJ_BANK - PROJ_HDR] POOL, undo_base[PROJ_BANK - PROJ_HDR] POOL;
static uint32_t undo_len[UNDO_N], undo_cnt, undo_head, undo_base_len, undo_base_ok;
static void undo_push(void)                     /* a new group of edits begins: how it was goes on the stack */
{
    if (!undo_base_ok) return;
    memcpy(undo_buf[undo_head], undo_base, undo_base_len); undo_len[undo_head] = undo_base_len;
    undo_head = (undo_head + 1u) % UNDO_N;
    if (undo_cnt < UNDO_N) undo_cnt++;
}
static void undo_tick(void)                     /* main loop: a quiet moment closes the group; the project as it is then is the base */
{
    if (add_aud_c >= 0) return;                 /* the Add Channel audition has a channel on loan: not part of the project */
    if (!undo_base_ok || (undo_open && fm1_ms - undo_t > 600u)) { undo_base_len = proj_pack(undo_base); undo_base_ok = undo_base_len != 0u; undo_open = 0; }
}
static void undo_reset(void) { undo_cnt = 0; undo_base_ok = 0; undo_open = 0; }
static int undo_go(void)                        /* 0: stepped back one */
{
    uint32_t i;
    if (!undo_cnt) return -1;
    i = (undo_head + UNDO_N - 1u) % UNDO_N;
    if (proj_unpack(undo_buf[i], undo_len[i]) != 0) return -1;
    undo_head = i; undo_cnt--;
    memcpy(undo_base, undo_buf[i], undo_len[i]); undo_base_len = undo_len[i]; undo_base_ok = 1; undo_open = 0;
    proj_dirty = 1; proj_t_edit = fm1_ms;
    return 0;
}
static void proj_blank(void) { seq_stop(); chan_lock = 1; seq_clear(); chan_lock = 0; undo_reset(); }      /* an empty project: no channels yet */

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
static uint32_t proj_mark;                      /* a demo's banks carry this bit in their number, so an older firmware's project in the slot can be told from it */
static int proj_write_buf(uint32_t slot, uint32_t len)     /* proj_buf's first len bytes into the slot's older bank */
{
    proj_hdr_t h, nh;
    int b = proj_newest(slot, &h);
    uint32_t bank = b < 0 ? 0u : (uint32_t)(b ^ 1), i, sectors, base = proj_base(slot, bank);
    if (!len) return -1;                        /* too big for a bank */
    sectors = (PROJ_HDR + len + 0xFFFu) / 0x1000u;
    for (i = 0; i < sectors; i++) if (pf_erase(base + i * 0x1000u)) return -1;
    if (pf_write(base + PROJ_HDR, proj_buf, len)) return -1;
    nh.magic = PROJ_MAGIC; nh.seq = (b < 0 ? 1u : h.seq + 1u) | proj_mark; nh.len = len; nh.crc = proj_crc(proj_buf, len);
    if (pf_write(base, &nh, sizeof nh)) return -1;
    return 0;
}
static int proj_save(uint32_t slot)             /* slots 0 and 1 are the demos: nothing is ever saved into them */
{
    if (slot < PROJ_DEMOS) return 0;
    return proj_write_buf(slot, proj_pack(proj_buf));
}
static void proj_open(uint32_t slot)            /* switch projects: stop, keep what is unsaved, load (or start blank) */
{
    seq_stop();
    if (proj_dirty && proj_save(proj_cur) == 0) proj_dirty = 0;
    proj_loaded = proj_load(slot) == 0;
    if (!proj_loaded) proj_blank();
    proj_cur = (uint8_t)slot; proj_dirty = 0; undo_reset();
}
static void proj_clear(uint32_t slot)           /* an empty slot: both banks' first sector */
{
    pf_erase(proj_base(slot, 0));
    pf_erase(proj_base(slot, 1));
}

/* what a slot looks like in the picker: a row per channel (its chip), a bit per step of page 1 */
static void proj_preview(uint32_t slot, uint16_t *rows, uint8_t *chips, uint32_t *nch)
{
    uint32_t c, i, ev, n, isz;
    proj_hdr_t h;
    int b;
    *nch = 0;
    memset(rows, 0, MAXCH * sizeof rows[0]);
    if (slot == proj_cur) {                     /* the open one: what is in memory, saved or not */
        *nch = chan_n;
        for (c = 0; c < chan_n; c++) {
            chips[c] = KIND_CHIP[chan[c].kind];
            for (i = 0; i < SEQ_LEN; i++) if (pat[0][c][i].note) rows[c] |= (uint16_t)(1u << i);
        }
        return;
    }
    b = proj_newest(slot, &h);
    if (b < 0 || pf_read(proj_base(slot, (uint32_t)b) + PROJ_HDR, proj_buf, h.len)) return;
    if (h.len < 7u || (proj_buf[0] != 2u && proj_buf[0] != 3u) || proj_buf[4] > MAXCH) return;
    isz = proj_buf[0] == 2u ? 14u : PROJ_INST;
    *nch = proj_buf[4];
    for (c = 0; c < *nch; c++) chips[c] = proj_buf[5u + c * (1u + isz)] < KINDS ? KIND_CHIP[proj_buf[5u + c * (1u + isz)]] : 0u;
    n = 5u + *nch * (1u + isz);
    if (h.len < n + 2u) { *nch = 0; return; }
    ev = (uint32_t)proj_buf[n] | (uint32_t)proj_buf[n + 1u] << 8;
    n += 2u;
    for (i = 0; i < ev && n + (i + 1u) * PROJ_EV <= h.len; i++) {
        const uint8_t *e = proj_buf + n + i * PROJ_EV;
        if (e[0] == 0u && e[1] < *nch && e[2] < SEQ_LEN) rows[e[1]] |= (uint16_t)(1u << e[2]);
    }
}

/* slots 0 and 1 are the demos (Korobeiniki, the console symphony): made again at every boot if they are not exactly what the firmware makes, so an update or a damaged slot never leaves two */
#include "demo2.h"
static int proj_copy(uint32_t from, uint32_t to);
static void seq_symphony(void) { proj_unpack(DEMO2, sizeof DEMO2); }
static void proj_demo_sync(void)
{
    proj_hdr_t h;
    uint32_t len, s, f;
    seq_stop();
    seq_tetris();                               /* what was in memory is gone: this runs before the first project is opened */
    len = proj_pack(proj_buf);
    if (proj_newest(0, &h) < 0 || h.len != len || h.crc != proj_crc(proj_buf, len)) proj_write_buf(0, len);
    if (proj_newest(1, &h) >= 0 && !(h.seq & 0x40000000u)) {         /* a project of an older firmware sits in slot 1: it moves to a free slot at the end */
        for (f = PROJ_SLOTS - 1u; f > 1u && proj_exists(f); f--) {}
        if (f > 1u) proj_copy(1, f);
    }
    seq_stop();
    seq_symphony();
    len = proj_pack(proj_buf);
    if (proj_newest(1, &h) >= 0 && (h.seq & 0x40000000u) && h.len == len && h.crc == proj_crc(proj_buf, len)) return;
    proj_mark = 0x40000000u;
    proj_write_buf(1, len);
    proj_mark = 0;
}
static int proj_copy(uint32_t from, uint32_t to)    /* a project into another slot (the open one as it is in memory) */
{
    proj_hdr_t h;
    int b;
    uint32_t len;
    if (from == proj_cur && from >= PROJ_DEMOS) { len = proj_pack(proj_buf); return proj_write_buf(to, len); }
    b = proj_newest(from, &h);
    if (b < 0 || !proj_bank_ok(from, (uint32_t)b, &h)) return -1;
    return proj_write_buf(to, h.len);
}
