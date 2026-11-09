/* SPDX-License-Identifier: GPL-3.0-only */
/* The manual's chapter for a chip: GLO on the chip screen opens a chapter named for the channel's chip, with a page for every page of knobs (in the screen's own order) and what each knob does.
 * Written in the manual's voice: a verb and an object in every sentence, the reader left out. Included by manual.c after the page type. When a chip gets a knob, add it to its page here. */
#define DK (M_KNOBS | M_SMALL)
static const mpage_t DOC_ENV = {"Envelope", DK, "Turn KNOB 1 for the attack, KNOB 2 for the decay, KNOB 3 for the level it falls to and KNOB 4 for the release."};
static const mpage_t DOC_ARP = {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for the vibrato speed."};

/* ---- the FM chips: a page for the channel's pages, then each operator page (what the screen shows for them) */
static const mpage_t DOC_OPM[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move through the channel and OP1 to OP4. The picture lights the one you edit. Turn ALGORITHM for its pages."},
    {"Channel", DK, "Turn KNOB 1 to pick the algorithm, how the four operators connect. Turn KNOB 2 for OP1's feedback, KNOB 3 to pan and KNOB 4 for the level."},
    {"LFO", DK, "Turn KNOB 1 for the LFO's speed and KNOB 2 for its wave: saw, square, triangle or noise. Turn KNOB 3 for the vibrato depth and KNOB 4 for the tremolo depth."},
    {"LFO reach", DK, "Turn KNOB 1 for how far the LFO bends the pitch and KNOB 2 for how far it dips the level. Only operators with AM take the level."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple of the note, KNOB 3 to detune a little and KNOB 4 to detune a lot."},
    {"Rates", DK, "Turn KNOB 1 for the attack and KNOB 2 for the first fall. Turn KNOB 3 for the level where it ends and KNOB 4 for the slow fall after it. Higher is faster."},
    {"Release", DK, "Turn KNOB 1 for the release. Turn KNOB 2 to speed up high notes and KNOB 3 to let the LFO shake this operator's level."},
};
static const mpage_t DOC_OPN[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move through the channel and OP1 to OP4. The picture lights the one you edit. Turn ALGORITHM for its pages."},
    {"Channel", DK, "Turn KNOB 1 to pick the algorithm, how the four operators connect. Turn KNOB 2 for OP1's feedback, KNOB 3 to pan and KNOB 4 for the level."},
    {"LFO", DK, "Turn KNOB 1 to pick one of eight LFO speeds, or none. Turn KNOB 2 for the vibrato depth and KNOB 3 for the tremolo depth."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple of the note, KNOB 3 to detune it and KNOB 4 to speed up high notes."},
    {"Rates", DK, "Turn KNOB 1 for the attack and KNOB 2 for the first fall. Turn KNOB 3 for the level where it ends and KNOB 4 for the slow fall after it. Higher is faster."},
    {"Release", DK, "Turn KNOB 1 for the release. Turn KNOB 2 to let the LFO shake this operator's level."},
};
static const mpage_t DOC_OPN3[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move through the channel and OP1 to OP4. The picture lights the one you edit. Turn ALGORITHM for its pages."},
    {"Channel", DK, "Turn KNOB 1 to pick the algorithm, how the four operators connect. Turn KNOB 2 for OP1's feedback and KNOB 4 for the level."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple of the note, KNOB 3 to detune it and KNOB 4 to speed up high notes."},
    {"Rates", DK, "Turn KNOB 1 for the attack and KNOB 2 for the first fall. Turn KNOB 3 for the level where it ends and KNOB 4 for the slow fall after it. Higher is faster."},
    {"Release", DK, "Turn KNOB 1 for the release, how fast it dies after you let go."},
};
static const mpage_t DOC_OPZ[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move through the channel and OP1 to OP4. The picture lights the one you edit. Turn ALGORITHM for its pages."},
    {"Channel", DK, "Turn KNOB 1 to pick the algorithm, how the four operators connect. Turn KNOB 2 for OP1's feedback and KNOB 4 for the level."},
    {"LFO", DK, "Turn KNOB 1 for the LFO's speed and KNOB 2 for its wave: saw, square, triangle or noise. Turn KNOB 3 for the vibrato depth and KNOB 4 for the tremolo depth."},
    {"LFO reach", DK, "Turn KNOB 1 for how far the LFO bends the pitch and KNOB 2 for how far it dips the level. Only operators with AM take the level."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple, KNOB 3 for a fine step between multiples and KNOB 4 to detune."},
    {"Rates", DK, "Turn KNOB 1 for the attack and KNOB 2 for the first fall. Turn KNOB 3 for the level where it ends and KNOB 4 for the slow fall after it. Higher is faster."},
    {"Release and wave", DK, "Turn KNOB 1 for the release and KNOB 2 to speed up high notes. Turn KNOB 3 for tremolo and KNOB 4 to pick this operator's wave."},
    {"Fixed pitch", DK, "Turn KNOB 1 to lift the pitch by a set interval and KNOB 2 to narrow the envelope. Turn KNOB 3 for a fixed pitch set by MUL, FINE and DT1."},
};
static const mpage_t DOC_OPL3[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move through the channel and OP1 to OP4. The picture lights the one you edit. Turn ALGORITHM for its pages."},
    {"Channel", DK, "Turn KNOB 1 to pick one of four connections. Turn KNOB 2 for OP1's feedback, KNOB 3 to pan and KNOB 4 for the level."},
    {"Depth", DK, "Turn KNOB 1 to deepen the tremolo and KNOB 2 to deepen the vibrato, for every operator that has them."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple, KNOB 3 for key scale level and KNOB 4 to pick one of eight waves."},
    {"Envelope", DK, "Turn KNOB 1 for the attack, KNOB 2 for the decay, KNOB 3 for the level it falls to and KNOB 4 for the release. Higher is faster."},
    {"Flags", DK, "Turn KNOB 1 to hold the level until you let go. Turn KNOB 2 to speed up high notes, KNOB 3 for tremolo and KNOB 4 for vibrato."},
};
static const mpage_t DOC_OPL2[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move between the channel and the two operators. OP1 modulates OP2. Turn ALGORITHM for the pages of each."},
    {"Channel", DK, "Turn KNOB 1 to switch between FM and both operators playing. Turn KNOB 2 for OP1's feedback and KNOB 4 for the level."},
    {"Depth", DK, "Turn KNOB 1 to deepen the tremolo and KNOB 2 to deepen the vibrato, for every operator that has them."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple, KNOB 3 for key scale level and KNOB 4 to pick one of four waves."},
    {"Envelope", DK, "Turn KNOB 1 for the attack, KNOB 2 for the decay, KNOB 3 for the level it falls to and KNOB 4 for the release. Higher is faster."},
    {"Flags", DK, "Turn KNOB 1 to hold the level until you let go. Turn KNOB 2 to speed up high notes, KNOB 3 for tremolo and KNOB 4 for vibrato."},
};
static const mpage_t DOC_OPL1[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move between the channel and the two operators. OP1 modulates OP2. Turn ALGORITHM for the pages of each."},
    {"Channel", DK, "Turn KNOB 1 to switch between FM and both operators playing. Turn KNOB 2 for OP1's feedback and KNOB 4 for the level."},
    {"Depth", DK, "Turn KNOB 1 to deepen the tremolo and KNOB 2 to deepen the vibrato, for every operator that has them."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Level", DK, "Turn KNOB 1 for TL, the level: higher is quieter. Turn KNOB 2 for the multiple and KNOB 3 for key scale level."},
    {"Envelope", DK, "Turn KNOB 1 for the attack, KNOB 2 for the decay, KNOB 3 for the level it falls to and KNOB 4 for the release. Higher is faster."},
    {"Flags", DK, "Turn KNOB 1 to hold the level until you let go. Turn KNOB 2 to speed up high notes, KNOB 3 for tremolo and KNOB 4 for vibrato."},
};
static const mpage_t DOC_OPLL[] = {
    {"Pick an operator", M_SEL | M_ALGO | M_SMALL, "Press SEL to move between the channel, the modulator and the carrier. The modulator is OP1. A mark by INST shows you changed the instrument."},
    {"Channel", DK, "Turn KNOB 1 to pick one of fifteen instruments, or 0 for your own. Turn KNOB 2 for feedback, KNOB 3 for the pedal and KNOB 4 for the level."},
    {"Arp", DK, "Turn KNOB 1 to pick a chord, KNOB 2 for the speed and KNOB 3 for the direction. Turn KNOB 4 for a slow vibrato."},
    {"Modulator", DK, "Turn KNOB 1 for TL, how hard it hits the carrier: higher is softer. Turn KNOB 2 for key scale level, KNOB 3 for the multiple and KNOB 4 to pick sine or half sine."},
    {"Carrier", DK, "Turn KNOB 1 for the multiple, KNOB 2 for key scale level and KNOB 3 to pick sine or half sine. The carrier's level is VOL."},
    {"Envelope", DK, "Turn KNOB 1 for the attack, KNOB 2 for the decay, KNOB 3 for the level it falls to and KNOB 4 for the release. Higher is faster."},
    {"Flags", DK, "Turn KNOB 1 for tremolo and KNOB 2 for vibrato. Turn KNOB 3 to hold the level until you let go and KNOB 4 to speed up high notes."},
};

/* ---- the others: tone page, then the pages in the screen's order */
#define TONE(t, s) {t, DK, s}
static const mpage_t DOC_NESP[] = {TONE("Tone", "Turn KNOB 1 to pick the pulse width: thin, medium, wide or square. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_MMC[] = {TONE("Tone", "Turn KNOB 1 to pick the pulse width. Turn KNOB 2 for the level and KNOB 3 for the vibrato. KNOB 4 does nothing: no sweep here.")};
static const mpage_t DOC_GBP[] = {TONE("Tone", "Turn KNOB 1 to pick the pulse width. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to pan: left, centre or right.")};
static const mpage_t DOC_TRI[] = {TONE("Tone", "The triangle has one shape and no level. Turn KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_NNES[] = {TONE("Noise", "Turn KNOB 1 for the long or short register, KNOB 2 for the level, KNOB 3 for vibrato and KNOB 4 to fix the period. 0 follows the note.")};
static const mpage_t DOC_NGB[] = {TONE("Noise", "Turn KNOB 1 for the long or short register, KNOB 2 for the level, KNOB 3 for vibrato and KNOB 4 to pan. The note picks the rate.")};
static const mpage_t DOC_NPCE[] = {TONE("Noise", "Turn KNOB 1 to fix the noise rate. At 0 it follows the note. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to pan.")};
static const mpage_t DOC_NVB[] = {TONE("Noise", "Turn KNOB 1 to pick the shift register's tap, 0 to 7. Turn KNOB 2 for the level, KNOB 3 for vibrato and KNOB 4 to pan.")};
static const mpage_t DOC_VRP[] = {TONE("Tone", "Turn KNOB 1 to pick one of eight pulse widths. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_SAWT[] = {TONE("Tone", "Turn KNOB 1 for OVER: past its usual top the saw wraps and turns rough. Turn KNOB 2 for the level, KNOB 3 for vibrato, KNOB 4 to slide.")};
static const mpage_t DOC_SMS[] = {TONE("Tone", "Turn KNOB 1 to pick tone or noise, on the note or at a fixed rate. Turn KNOB 2 for the level, KNOB 3 for vibrato, KNOB 4 to slide.")};
static const mpage_t DOC_NGP[] = {TONE("Tone", "Turn KNOB 1 to pick tone or noise, on the note or at a fixed rate. Turn KNOB 2 for the level, KNOB 3 for vibrato, KNOB 4 to pan.")};
static const mpage_t DOC_GBW[] = {TONE("Tone", "Turn KNOB 1 to start from pulse, saw, triangle or sine. Turn KNOB 2 for one of four levels, KNOB 3 for the vibrato and KNOB 4 to pan.")};
static const mpage_t DOC_WAVP[] = {TONE("Tone", "Turn KNOB 1 to start from pulse, saw, triangle or sine. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to pan.")};
static const mpage_t DOC_WAVS[] = {TONE("Tone", "Turn KNOB 1 to start from pulse, saw, triangle or sine. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_WAVE = {"Wave", DK, "Turn KNOB 1 for the shape between pulse, saw, triangle and sine. Turn KNOB 2 to fold it, KNOB 3 to crush it into steps and KNOB 4 to twist it."};
static const mpage_t DOC_FDS[] = {TONE("Tone", "Turn KNOB 1 to start from pulse, saw, triangle or sine. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 for the modulator's depth.")};
static const mpage_t DOC_FDSM = {"Modulator", DK, "Turn KNOB 1 for the modulator's speed as a multiple of the note. Turn KNOB 2 to fade its depth in or out over the note and KNOB 3 to pick its shape."};
static const mpage_t DOC_TIA[] = {TONE("Tone", "Turn KNOB 1 to pick one of eleven sound types, from buzz to noise. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_POK[] = {TONE("Tone", "Turn KNOB 1 to pick one of eight distortions. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_AY[] = {TONE("Tone", "Turn KNOB 1 to mix noise into the tone, up to noise alone. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_5B[] = {TONE("Tone", "Turn KNOB 1 to mix noise into the tone, up to noise alone. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to pick a buzz.")};
static const mpage_t DOC_5BP = {"Noise", DK, "Turn KNOB 1 to fix the noise period. At 0 it follows the note. Turn KNOB 2 for the buzz speed against the note and KNOB 3 to play the tone together with the buzz."};
static const mpage_t DOC_SAA[] = {TONE("Tone", "Turn KNOB 1 to mix noise into the tone, up to noise alone. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to pan.")};
static const mpage_t DOC_TED[] = {TONE("Tone", "Turn KNOB 1 to switch between tone and noise. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_LYN[] = {TONE("Tone", "Turn KNOB 1 to pick the sound: a square, then shift register tunings. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 to pan.")};
static const mpage_t DOC_SN7[] = {TONE("Tone", "Turn KNOB 1 to pick tone, tone with noise, noise or siren. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 for the slow oscillator's speed.")};
static const mpage_t DOC_MSM[] = {TONE("Tone", "Turn KNOB 1 to pick the footages that play: 16, 8, 4 and 2 feet. Turn KNOB 2 for the level, KNOB 3 for the vibrato and KNOB 4 for the two quints.")};
static const mpage_t DOC_SPK[] = {TONE("Tone", "The speaker has one square and no level. Turn KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_ZX[] = {TONE("Tone", "Turn KNOB 1 to pick one of eight pulse widths. The beeper has no level. Turn KNOB 3 for the vibrato and KNOB 4 to slide the pitch.")};
static const mpage_t DOC_SID[] = {TONE("Tone", "Turn KNOB 1 to pick the wave, KNOB 2 for the level and KNOB 3 for the vibrato. Turn KNOB 4 to pick the 6581 or the 8580.")};
static const mpage_t DOC_SIDE = {"Envelope", DK, "Turn KNOB 1 for the attack, KNOB 2 for the decay, KNOB 3 for the sustain level and KNOB 4 for the release. They are the chip's own ADSR steps."};
static const mpage_t DOC_SIDP = {"Pulse", DK, "Turn KNOB 1 for the pulse width and KNOB 2 to sweep it. Turn KNOB 3 to sync the oscillator and KNOB 4 for ring modulation."};
static const mpage_t DOC_SIDF = {"Filter", DK, "Turn KNOB 1 for the cutoff and KNOB 2 for the resonance. Turn KNOB 3 to pick low, band or high pass and KNOB 4 for how far the envelope opens the filter."};
static const mpage_t DOC_AYX = {"Noise and buzz", DK, "Turn KNOB 1 to fix the noise period. At 0 it follows the note. Turn KNOB 2 to pick a buzz: saws, a triangle or plucks. Turn KNOB 3 for its speed."};
static const mpage_t DOC_SAAX = {"Chip envelope", DK, "Turn KNOB 1 to pick one of the chip's envelope shapes: decay, triangle or attack, once or repeating. Turn KNOB 2 for coarser steps and KNOB 3 to fix the noise rate."};
static const mpage_t DOC_POKX = {"Filter", DK, "Turn KNOB 1 for the high-pass filter: higher cuts more of the lows. Turn KNOB 2 to pick the clock the noise runs from: 64 kHz, 15 kHz or 1.79 MHz."};
static const mpage_t DOC_LYNX = {"Shift register", DK, "Turn KNOB 1 to integrate the output: it ramps instead of jumping. Turn KNOB 2 to pick another short cycle. Turn KNOB 3 to set the balance, or 0 to use PAN."};
static const mpage_t DOC_SN7X = {"Oscillators", DK, "Turn KNOB 1 for the noise filter, KNOB 2 to fix the noise rate and KNOB 3 for how deep the slow oscillator swings. Turn KNOB 4 for the square's width."};
static const mpage_t DOC_N163X = {"Length", DK, "Turn KNOB 1 to pick how many steps the wave has: 4, 8, 16, 32 or 64. A shorter wave is coarser and rougher."};
static const mpage_t DOC_SWX = {"Sweep", DK, "Turn KNOB 1 to slide the pitch down or up on every note. Far from the middle it moves several notes at once."};
/* DRAW (the last page of a chip with a writable wave): one page for the chip's depth and length, one for the brush and the mirror */
#define DRW(t) {"Draw", M_SELECT | M_K1 | M_SMALL, t}
static const mpage_t DOC_DRGB = DRW("Turn SELECT to pick one of the 32 steps. Turn KNOB 1 to set its height, 0 to 15: four bits, like the chip's wave RAM.");
static const mpage_t DOC_DRPC = DRW("Turn SELECT to pick one of the 32 steps. Turn KNOB 1 to set its height, 0 to 31: five bits, like the chip's wave RAM.");
static const mpage_t DOC_DRVB = DRW("Turn SELECT to pick one of the 32 steps. Turn KNOB 1 to set its height, 0 to 63: six bits, like the chip's wave RAM.");
static const mpage_t DOC_DRSC = DRW("Turn SELECT to pick one of the 32 steps. Turn KNOB 1 to set its height. The chip has 256 heights: each click moves 4.");
static const mpage_t DOC_DRN1 = DRW("Turn SELECT to pick a step. The wave is as long as LENGTH says. Turn KNOB 1 to set its height, 0 to 15.");
static const mpage_t DOC_DRFD = DRW("Turn SELECT to pick one of the 64 steps. Turn KNOB 1 to set its height, 0 to 63: six bits, like the chip's wave RAM.");
static const mpage_t DOC_DRAW2 = {"Brush and mirror", M_K2 | M_K3 | M_SMALL, "Turn KNOB 2 to move 1, 2, 4 or 8 steps at once. Turn KNOB 3 to copy edits to the far side. Turn a WAVE knob to go back to the shape."};
static const mpage_t DOC_DRUM[] = {
    {"Kit", M_K2 | M_SMALL, "Turn KNOB 2 for the level of the whole kit. The picture shows the lanes: one for each drum."},
    {"Hits", M_SEQ | M_KEYS | M_SMALL, "Press SEQ to open the drum lanes. Press a black key to pick a drum and a white key to place it on a step."},
    {"Accents", M_ENV | M_EDIT | M_SMALL, "Hold ENV and press a white key to place an accent. Press EDIT to clear the drum you picked."},
};

#define DOCB(a) a, (uint8_t)(sizeof a / sizeof a[0])
static mpage_t man_cdoc[20];                                       /* the chapter being shown: the chip's pages, then the general ones */
static uint32_t man_pn;
static void man_add(const mpage_t *p) { if (man_pn < 20u) man_cdoc[man_pn++] = *p; }
static void man_addn(const mpage_t *p, uint32_t n) { uint32_t i; for (i = 0; i < n; i++) man_add(&p[i]); }
static uint32_t man_chip_pages(uint32_t kind, uint32_t *at, uint32_t chip_page, uint32_t foc)      /* builds man_cdoc for this kind; the page to open at (the knob page on the screen); the page count */
{
    const uint32_t fam = fm_fam(kind);
    man_pn = 0;
    if (fam) {
        const mpage_t *t; uint32_t n, nc, no;
        switch (fam) {
        case FAM_OPM: t = DOC_OPM; n = sizeof DOC_OPM / sizeof DOC_OPM[0]; nc = 4; break;
        case FAM_OPN: t = DOC_OPN; n = sizeof DOC_OPN / sizeof DOC_OPN[0]; nc = 3; break;
        case FAM_OPN3: t = DOC_OPN3; n = sizeof DOC_OPN3 / sizeof DOC_OPN3[0]; nc = 2; break;
        case FAM_OPZ: t = DOC_OPZ; n = sizeof DOC_OPZ / sizeof DOC_OPZ[0]; nc = 4; break;
        case FAM_OPL3: t = DOC_OPL3; n = sizeof DOC_OPL3 / sizeof DOC_OPL3[0]; nc = 3; break;
        case FAM_OPL2: t = DOC_OPL2; n = sizeof DOC_OPL2 / sizeof DOC_OPL2[0]; nc = 3; break;
        case FAM_OPL1: t = DOC_OPL1; n = sizeof DOC_OPL1 / sizeof DOC_OPL1[0]; nc = 3; break;
        default: t = DOC_OPLL; n = sizeof DOC_OPLL / sizeof DOC_OPLL[0]; nc = 2; break;
        }
        no = n - 1u - nc;
        man_addn(t, n);
        *at = foc ? 1u + nc + (chip_page < no ? chip_page : 0u) : 1u + (chip_page < nc ? chip_page : 0u);
    } else {
        const mpage_t *t = 0, *x1 = 0, *x2 = 0, *x3 = 0;
        switch (kind) {
        case KD_NES_PU: t = DOC_NESP; break;
        case KD_MMC5: t = DOC_MMC; break;
        case KD_NES_NOI: t = DOC_NNES; break;
        case KD_GB_NOI: t = DOC_NGB; break;
        case KD_PCE_NOI: t = DOC_NPCE; break;
        case KD_VB_NOI: t = DOC_NVB; break;
        case KD_GB_PU: t = DOC_GBP; x3 = &DOC_SWX; break;
        case KD_NES_TRI: t = DOC_TRI; break;
        case KD_VRC6_PU: t = DOC_VRP; break;
        case KD_VRC6_SAW: t = DOC_SAWT; break;
        case KD_SMS: t = DOC_SMS; break;
        case KD_NGP: t = DOC_NGP; x3 = &DOC_SWX; break;
        case KD_GB_WAV: t = DOC_GBW; x1 = &DOC_WAVE; x3 = &DOC_SWX; break;
        case KD_PCE: case KD_VB_WAV: t = DOC_WAVP; x1 = &DOC_WAVE; x3 = &DOC_SWX; break;
        case KD_WS: case KD_C30: t = DOC_WAVP; x1 = &DOC_WAVE; break;
        case KD_N163: t = DOC_WAVS; x1 = &DOC_WAVE; x3 = &DOC_N163X; break;
        case KD_WSG: case KD_SCC: t = DOC_WAVS; x1 = &DOC_WAVE; break;
        case KD_FDS: t = DOC_FDS; x1 = &DOC_WAVE; x2 = &DOC_FDSM; break;
        case KD_TIA: t = DOC_TIA; break;
        case KD_POKEY: t = DOC_POK; x3 = &DOC_POKX; break;
        case KD_AY: case KD_PC98_SSG: case KD_NEO_SSG: case KD_OPN_SSG: t = DOC_AY; x3 = &DOC_AYX; break;
        case KD_S5B: t = DOC_5B; x1 = &DOC_5BP; break;
        case KD_SAA: t = DOC_SAA; x3 = &DOC_SAAX; break;
        case KD_TED: case KD_VIC: t = DOC_TED; break;
        case KD_LYNX: t = DOC_LYN; x3 = &DOC_LYNX; break;
        case KD_SN77: t = DOC_SN7; x3 = &DOC_SN7X; break;
        case KD_MSM: t = DOC_MSM; break;
        case KD_SPK: t = DOC_SPK; break;
        case KD_ZX: t = DOC_ZX; break;
        case KD_SID: t = DOC_SID; x1 = &DOC_SIDP; x2 = &DOC_SIDF; break;
        default: t = 0; break;
        }
        if (IS_DRUM(kind)) { man_addn(DOC_DRUM, sizeof DOC_DRUM / sizeof DOC_DRUM[0]); *at = 0; }
        else {
            man_add(t ? &t[0] : &DOC_NESP[0]);
            man_add(kind == KD_SID ? &DOC_SIDE : &DOC_ENV); man_add(&DOC_ARP);
            if (x1) man_add(x1);
            if (x2) man_add(x2);
            if (x3) man_add(x3);
            switch (kind) {                                             /* DRAW: the last page */
            case KD_GB_WAV: case KD_WS: case KD_C30: man_add(&DOC_DRGB); man_add(&DOC_DRAW2); break;
            case KD_PCE: man_add(&DOC_DRPC); man_add(&DOC_DRAW2); break;
            case KD_VB_WAV: man_add(&DOC_DRVB); man_add(&DOC_DRAW2); break;
            case KD_SCC: man_add(&DOC_DRSC); man_add(&DOC_DRAW2); break;
            case KD_N163: man_add(&DOC_DRN1); man_add(&DOC_DRAW2); break;
            case KD_FDS: man_add(&DOC_DRFD); man_add(&DOC_DRAW2); break;
            default: break;
            }
            *at = chip_page < man_pn ? chip_page : 0u;
        }
    }
    man_addn(MP_CHIP, sizeof MP_CHIP / sizeof MP_CHIP[0]);
    return man_pn;
}
