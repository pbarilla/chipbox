/* SPDX-License-Identifier: GPL-3.0-only */
/* Projects over USB-MIDI, for the website's Projects page: read a slot's bytes, write a slot. The same SysEx frames as the
 * updater (F0 pack7(00 59 cmd len24 body sum) F7, ota.c), commands 0x50..0x52. A project is its payload as proj_pack makes it.
 *   0x50 info     (one byte, ignored) -> 0x50: version 1, slots, open slot, 1 if the open one is unsaved, then per slot: used, length (2)
 *   0x51 read     slot, offset (2) -> 0x51: status, slot, total (2), offset (2), up to 200 bytes (offset 0 takes a snapshot;
 *                 the open slot is read as it is now, unsaved edits included)
 *   0x52 write    slot, total (2), offset (2), bytes -> 0x52: status, slot, bytes so far (2). In order, from offset 0; the last chunk
 *                 checks the whole project and writes the slot. Slot 0 (the demo) is locked. Written slots are opened.
 * status: 0 ok, 1 stored, 2 not a project, 3 locked, 4 flash failed, 5 out of order, 6 no such slot, 7 empty slot
 * Included by chipbox.c after proj.c, on the device only (the fake FM-1 has no SysEx). */
#ifndef CHIPBOX_HOST
#define PX_CHUNK 200u
static uint8_t px[PROJ_BANK] POOL;                      /* the project in transit */
static uint32_t px_len, px_got, px_slot, px_open_req;   /* px_open_req: slot + 1 to open in the main loop */
static uint8_t px_msg[8u + 8u + PX_CHUNK + 8u], px_wire[2u + 8u * (8u + 8u + PX_CHUNK + 8u) / 7u + 4u];

static void px_reply(uint32_t cmd, const uint8_t *body, uint32_t n)
{
    uint32_t i, s = 0, w;
    px_msg[0] = 0; px_msg[1] = 0x59; px_msg[2] = (uint8_t)cmd;
    px_msg[3] = (uint8_t)n; px_msg[4] = (uint8_t)(n >> 8); px_msg[5] = 0;
    for (i = 0; i < n; i++) { px_msg[6u + i] = body[i]; s += body[i]; }
    px_msg[6u + n] = (uint8_t)~s;
    px_wire[0] = 0xF0;
    w = 1u + ota_pack7(px_msg, 7u + n, px_wire + 1);
    px_wire[w++] = 0xF7;
    ota_wire_send(px_wire, w);
}
static uint32_t px_snapshot(uint32_t slot)              /* the slot's project bytes into px: the length, 0 when there is none */
{
    proj_hdr_t h;
    int b;
    uint32_t tries;
    if (slot == proj_cur) return proj_pack(px);
    b = proj_newest(slot, &h);
    for (tries = 0; b >= 0 && tries < 2u; tries++, b ^= 1) {
        if (tries && !proj_header(slot, (uint32_t)b, &h)) continue;
        if (proj_bank_ok(slot, (uint32_t)b, &h)) { memcpy(px, proj_buf, h.len); return h.len; }
    }
    return 0;
}
static void proj_io(uint32_t cmd, const uint8_t *b, uint32_t n)
{
    uint8_t r[8u + PX_CHUNK];
    uint32_t s, i;
    if (cmd == 0x50u) {
        r[0] = 1; r[1] = (uint8_t)PROJ_SLOTS; r[2] = proj_cur; r[3] = proj_dirty ? 1u : 0u;
        for (s = 0; s < PROJ_SLOTS; s++) {
            proj_hdr_t h;
            int ok = proj_newest(s, &h) >= 0;
            r[4u + s * 3u] = (uint8_t)ok; r[5u + s * 3u] = ok ? (uint8_t)h.len : 0u; r[6u + s * 3u] = ok ? (uint8_t)(h.len >> 8) : 0u;
        }
        px_reply(0x50u, r, 4u + PROJ_SLOTS * 3u);
    } else if (cmd == 0x51u && n == 3u) {
        uint32_t slot = b[0], off = (uint32_t)b[1] | (uint32_t)b[2] << 8, k;
        r[1] = (uint8_t)slot; r[2] = r[3] = r[4] = r[5] = 0;
        if (slot >= PROJ_SLOTS) { r[0] = 6; px_reply(0x51u, r, 6u); return; }
        if (off == 0u) { px_len = px_snapshot(slot); px_slot = slot; }
        if (!px_len || slot != px_slot || off > px_len) { r[0] = px_len ? 5u : 7u; px_reply(0x51u, r, 6u); return; }
        k = px_len - off < PX_CHUNK ? px_len - off : PX_CHUNK;
        r[0] = 0; r[2] = (uint8_t)px_len; r[3] = (uint8_t)(px_len >> 8); r[4] = (uint8_t)off; r[5] = (uint8_t)(off >> 8);
        for (i = 0; i < k; i++) r[6u + i] = px[off + i];
        px_reply(0x51u, r, 6u + k);
    } else if (cmd == 0x52u && n >= 6u) {
        uint32_t slot = b[0], total = (uint32_t)b[1] | (uint32_t)b[2] << 8, off = (uint32_t)b[3] | (uint32_t)b[4] << 8, dl = n - 5u;
        uint32_t st = 0;
        if (slot >= PROJ_SLOTS) st = 6u;
        else if (slot < PROJ_DEMOS) st = 3u;
        else if (off == 0u) {
            if (!total || total > PROJ_BANK - PROJ_HDR) st = 2u;
            else { px_len = total; px_got = 0; px_slot = slot; }
        }
        if (!st && (slot != px_slot || off != px_got || off + dl > px_len || !px_len)) st = 5u;
        if (!st) {
            for (i = 0; i < dl; i++) px[off + i] = b[5u + i];
            px_got += dl;
            if (px_got == px_len) {
                if (proj_check(px, px_len)) st = 2u;
                else {
                    seq_stop();
                    memcpy(proj_buf, px, px_len);
                    if (proj_write_buf(slot, px_len)) st = 4u;
                    else { st = 1u; px_open_req = slot + 1u; }
                    px_len = 0;
                }
            }
        }
        r[0] = (uint8_t)st; r[1] = (uint8_t)slot; r[2] = (uint8_t)px_got; r[3] = (uint8_t)(px_got >> 8);
        if (st >= 2u) px_len = 0;
        px_reply(0x52u, r, 4u);
    }
}
#endif
