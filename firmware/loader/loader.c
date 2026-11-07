/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca update loader (device side). Runs from RAM, started by the SPL from
 * an update record, with interrupts off: USB is polled from the main loop.
 * It is a USB-MIDI device "Felucca Update" (1209:0002) that answers the
 * M-UPGRADE update protocol as "ota-FM-1_900" (hosts look for "ota-"), and
 * writes the app area only (ldr_core.c). The UBOOT soft key works here too.
 * Single compilation unit. */
#include <stdint.h>
#define FELUCCA_LOADER 1
#define FELUCCA_CDC 0
#define FELUCCA_OTA 1
#define FELUCCA_OTA_DRYRUN 0
#define FELUCCA_USB_PID 0x0002
#define FELUCCA_ID "ota-FM-1_900"
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include "fm1_time.h"
#include "fm1_sys.h"
/* the loader may write the app area and erase update records; never the head */
#define FL_RANGE_OK(off, n) (FL_IN(off, n, 0x4000u, 0x93000u) || FL_IN(off, n, 0x93000u, 0xFC000u))
#include "fm1_flash.h"
#include "../src/libc.c"
#include "../src/usb.c"

static uint32_t ldr_ms(void)   /* monotonic ms; survives the TIMER4 wrap (178.9 s) */
{
    static uint32_t last, acc, ms;
    uint32_t now = fm1_ticks();
    acc += now - last;
    last = now;
    ms += acc / (1000u * FM1_TICKS_PER_US);
    acc %= 1000u * FM1_TICKS_PER_US;
    return ms;
}

static void ldr_poll(void)                          /* USB at ~2 kHz, watchdog */
{
    static uint32_t last;
    uint32_t now = fm1_ticks();
    fm1_wdt_feed();
    if (now - last >= 500u * FM1_TICKS_PER_US) {
        last = now;
        usb_poll();
    }
}

/* ---- ota.c hooks (the SysEx frame hooks are in usb.c) ---- */
static uint32_t ota_now_ms(void) { return ldr_ms(); }
static void ota_idle(void) { ldr_poll(); }
/* step-1 hooks of ota.c: not used by the loader */
static int ota_erase(uint32_t off) { (void)off; return -1; }
static int ota_prog(uint32_t off, const void *p, uint32_t n) { (void)off; (void)p; (void)n; return -1; }
static int ota_fread(uint32_t off, void *p, uint32_t n) { return fl_read_ram(off, p, n); }
static void ota_show(uint32_t step, int32_t code) { (void)step; (void)code; }
static void ota_commit(const uint8_t *parm) { (void)parm; }
#include "../src/ota.c"

/* ---- the screen: the panel is started again here (the app that ran before is gone, and after a power cut
 * nothing ever started it), a progress bar follows the write. If the panel's SPI does not answer there is
 * no screen at all and the update goes on as before. 5x7 letters, scaled with plain rectangles. */
#include "fm1_lcd_hw.h"
#include "../src/lcd.c"
static const uint8_t LDR_FONT[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}};
#define LDR_BAR_X 20u
#define LDR_BAR_Y 170u
#define LDR_BAR_W 200u
static uint8_t ldr_lcd, ldr_stopped;
static uint32_t ldr_bar;

static void ldr_text(uint32_t y, uint32_t s, const char *t)       /* centred, white */
{
    uint32_t n = 0, x, c, r, e;
    while (t[n])
        n++;
    x = (240u - (n * 6u - 1u) * s) / 2u;
    for (; *t; t++, x += 6u * s) {
        if (*t < 'A' || *t > 'Z')
            continue;
        for (c = 0; c < 5u; c++) {
            uint32_t bits = LDR_FONT[*t - 'A'][c];
            for (r = 0; r < 7u; r++) {
                if (!(bits >> r & 1u))
                    continue;
                for (e = r; e < 7u && (bits >> e & 1u); e++)
                    ;
                lcd_fill(x + c * s, y + r * s, s, (e - r) * s, 0xFFFFu);
                r = e;
            }
        }
    }
}

static void ldr_status(const char *t)
{
    lcd_fill(0, 196, 240, 14, 0);
    ldr_text(196, 2, t);
}

static void ldr_screen(uint32_t mode)                             /* 0 waiting, 1 stopped, 2 done */
{
    if (!ldr_lcd)
        return;
    lcd_fill(0, 0, 240, 240, 0);
    ldr_stopped = mode == 1u;
    ldr_bar = 0;
    if (mode == 1u) {
        ldr_text(60, 4, "STOPPED");
        ldr_text(116, 2, "TRY AGAIN");
        ldr_text(140, 2, "FROM THE COMPUTER");
        return;
    }
    ldr_text(50, 4, mode == 2u ? "DONE" : "UPDATING");
    if (mode == 2u) {
        ldr_text(110, 2, "RESTARTING");
        return;
    }
    ldr_text(100, 2, "PLEASE WAIT");
    ldr_text(124, 2, "DO NOT UNPLUG");
    lcd_fill(LDR_BAR_X, LDR_BAR_Y, LDR_BAR_W, 10, 0x2104u);
    ldr_status("WAITING");
}

static void ldr_lcd_start(void)
{
    fm1_lcd_hw_init();
    lcd_cmd(0x00);                                  /* a NOP: with no answer from the SPI there is no screen work at all */
    if (fm1_lcd_timeouts)
        return;
    lcd_init();
    fm1_wdt_feed();
    ldr_lcd = 1;
    ldr_screen(0);
}

/* ---- ldr_core.c hooks: interrupts are off for the whole loader ---- */
static int ldr_fread(uint32_t off, void *p, uint32_t n) { return fl_read_ram(off, p, n); }
static int ldr_erase(uint32_t off)
{
    uint32_t took;
    return fl_erase4k_ram(off, &took);
}
static int ldr_prog(uint32_t off, const void *p, uint32_t n)
{
    uint32_t took;
    return fl_prog_ram(off, p, n, &took);
}
static void ldr_record_clear(void)
{
    fm1_updata_parm_clear();                        /* CRC 0: the SPL ignores it on a warm reset */
}
static int ldr_flash_known(void) { return FL_JEDEC_OK(fl_jedec_ram()); }   /* as the app (project.c) */
static void ldr_progress(uint32_t done, uint32_t total)
{
    if (ldr_lcd && !fm1_lcd_timeouts) {
        uint32_t w = (done >> 12) * LDR_BAR_W / (total >> 12);
        if (!ldr_bar)
            ldr_status("WRITING");
        if (w < ldr_bar) {                          /* another pass */
            lcd_fill(LDR_BAR_X, LDR_BAR_Y, LDR_BAR_W, 10, 0x2104u);
            ldr_bar = 0;
        }
        if (w > ldr_bar) {
            lcd_fill(LDR_BAR_X + ldr_bar, LDR_BAR_Y, w - ldr_bar, 10, 0xFFFFu);
            ldr_bar = w;
        }
    }
    ldr_poll();
}
#include "ldr_core.c"

extern uint32_t _bss_start[], _bss_end[];

void ldr_main(void)
{
    fm1_wdt_arm(0x0D);                              /* ~8 s, fed by ldr_poll */
    fm1_time_init();
    ldr_lcd_start();
    usb_start();
    for (;;) {
        ldr_poll();
        usb_retry(ldr_ms());
        if (usb.uboot_req) {                        /* soft key: mask-ROM UBOOT, as in Felucca */
            usb_detach();
            fm1_delay_ms(30);
            fm1_enter_uboot();
        }
        ota_service();                              /* handshake */
        if (usb.ota_req) {
            int rc;
            usb.ota_req = 0;
            if (ldr_stopped)
                ldr_screen(0);
            rc = ldr_session();
            if (rc == 0 || rc == LDR_EFLASH) {      /* done, or another flash chip: the old firmware again */
                fm1_delay_ms(200);                  /* the "success" reply leaves */
                ldr_screen(rc == 0 ? 2u : 1u);
                usb_detach();
                fm1_delay_ms(30);
                fm1_core_reset();                   /* the SPL boots the new app */
            }
            ldr_screen(1);
        }
    }
}

void ldr_cstart(void)
{
    uint32_t *p;
    ldr_record_clear();                             /* first thing: a crash below cannot loop through a
                                                     * warm reset; resume after power loss uses the flash record */
    for (p = _bss_start; p < _bss_end; p++)
        *p = 0;
    ldr_main();
}
