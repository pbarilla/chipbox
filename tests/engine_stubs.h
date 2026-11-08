/* SPDX-License-Identifier: GPL-3.0-only */
/* What the app gives the engine (chips.c / seq.c) and a standalone test does not have: MIDI out and the Bluetooth link, as no-ops. */
#pragma once
#include <stdint.h>
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2) { (void)st; (void)d1; (void)d2; }
static void midi_isr_send(uint32_t st, uint32_t d1, uint32_t d2) { (void)st; (void)d1; (void)d2; }
static uint8_t brothers_mode;
static void brothers_lead_tick(uint32_t off, uint32_t per) { (void)off; (void)per; }
static void brothers_lead_run(uint32_t run) { (void)run; }
static uint32_t brothers_snap(uint32_t note, uint32_t scale) { (void)scale; return note; }
static uint32_t brothers_duck(uint32_t depth) { (void)depth; return 256u; }
