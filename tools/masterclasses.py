#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The masterclasses: long lessons that teach WHY a sound behaves as it does. Pages to read (a window: OCT+ goes on) alternate with things to do on the instrument.
# This file is the source: it writes firmware/src/lessons_mc_*.h; tests/lesson_test.py reads the same list to do each one.  python3 tools/masterclasses.py
# A page: R(title, body, patch) - title 20 characters, body wrapped to 26 characters a line (long ones are split into several pages); patch: the sound the chapter starts from (lessons.c les_patch), 0 keeps yours.
# A step: D(text, pill, value, op, number, aux, action)  (text 26 characters at most).
import os, sys, textwrap

class R:
    def __init__(s, title, body, patch=0): s.title, s.body, s.patch = title, " ".join(body.split()), patch
def D(text, pill, v, op, n, a, act): return (text, pill, v, op, n, a, act)
def KEY(text="Press a key to hear it"): return D(text, "KEYS", "V_KEYS", "O_EQ", 1, 0, ("key",))
def KN(n, text, v, op, num, a, d): return D(text, f"KNOB {n}", v, op, num, a, ("knobs", n, d))
def SEL(text, foc, clicks=1): return D(text, "SEL", "V_FMFOC", "O_EQ", foc, 0, ("sel", clicks))
def ALG(page, text, mod=0, last=False): return D(text, "ALGORITHM", "V_LASTPG" if last else "V_CHIPPG", "O_EQ", 1 if last else page, 0, ("alg", page, mod))

CF_ALG, CF_FB, CF_PAN, CF_PMS, CF_AMS, CF_LFS, CF_LFW, CF_PMD, CF_AMD = 0, 1, 2, 3, 4, 5, 6, 7, 8
def OPF(op, f): return op * 32 + f                      # V_FMOP's aux
OF_TL, OF_MUL, OF_DT1, OF_DT2, OF_AR, OF_D1R = 0, 1, 2, 3, 5, 6
SP_WAVE, SP_MODEL, SP_PW, SP_PWM, SP_SYNC, SP_RING, SP_CUT, SP_RES, SP_MODE, SP_FENV = range(10)
IF_WAVE, IF_DUTY, IF_WF, IF_WC, IF_WT = range(5)

MC = []                                          # (id, name, kind, items)
def masterclass(mid, name, kd, *items): MC.append((mid, name, kd, list(items)))

# ---------------------------------------------------------------------------------------------------------------------------------------------- FM
masterclass(1, "Masterclass: FM", "KD_OPM",
  R("Why FM?", """FM builds sound out of sine waves. A sine is the purest tone there is: one pitch, no overtones, like a whistle.
Each OPERATOR is one sine wave with its own volume envelope.
Right now only operator 1 is on. Listen to it.""", 1),
  KEY("Press a key: a pure sine"),
  R("Operators talk", """Now operator 1 feeds operator 2. It no longer adds sound: it BENDS the pitch of operator 2, many thousand times a second.
The bender is the MODULATOR. The one you hear is the CARRIER.
A bent sine contains new tones. At the moment the bend is tiny.""", 2),
  KEY("Press a key: nearly pure"),
  SEL("Press SEL to pick OP 1", 1),
  KN(1, "Turn KNOB 1 down: more FM", "V_FMOP", "O_DOWN", 0, OPF(0, OF_TL), -8),
  KEY("Press a key: it brightens"),
  R("The index", """How hard the modulator pushes is the INDEX. At zero you hear the plain sine. As it grows, new tones appear above and below the carrier, called SIDEBANDS: the sound gets brighter, then rough.
The modulator's TL is the index. TL counts the volume DOWN, so a LOW TL pushes HARD."""),
  KN(1, "Turn KNOB 1 up: less FM", "V_FMOP", "O_UP", 0, OPF(0, OF_TL), 8),
  KEY("Press a key: a sine again"),
  R("Ratio", """The modulator's MUL says how fast it wobbles, as a multiple of the note. The sidebands sit at the carrier plus and minus whole steps of that speed.
MUL 1: they land on every harmonic: bright, saw-like.
MUL 2: only on the odd harmonics: hollow, like a clarinet or a square wave.""", 3),
  SEL("Press SEL to pick OP 1", 1),
  KN(2, "Turn KNOB 2 up to MUL 2", "V_FMOP", "O_EQ", 2, OPF(0, OF_MUL), 1),
  KEY("Press a key: hollow"),
  R("Bells", """Whole-number ratios give a clean pitch. Odd ones do not: the sidebands fall BETWEEN the harmonics and the pitch smears into a clang or a bell.
DT2 pushes an operator by a fixed odd interval (about x1.41, x1.57 or x1.73). It is the quickest road to a bell."""),
  KN(4, "Turn KNOB 4 up for DT2", "V_FMOP", "O_GE", 1, OPF(0, OF_DT2), 2),
  KEY("Press a key: a clang"),
  R("Envelopes are colour", """Every operator has its own envelope. The carrier's is the LOUDNESS. The modulator's is the BRIGHTNESS: while it is strong the sound is bright; as it fades the sound mellows to a plain sine.
Strong, then fading fast: a pluck, an electric piano, a bell. Fading in slowly: a brass swell.""", 4),
  KEY("Press a key: bright, soft"),
  SEL("Press SEL to pick OP 1", 1),
  ALG(1, "Turn ALGORITHM to AR", 3),
  KN(2, "Turn KNOB 2 for slow fade", "V_FMOP", "O_DOWN", 0, OPF(0, OF_D1R), -6),
  KEY("Press a key: stays bright"),
  KN(1, "Turn KNOB 1 for slow AR", "V_FMOP", "O_DOWN", 0, OPF(0, OF_AR), -12),
  KEY("Press a key: it swells in"),
  R("Feedback", """FEEDBACK (FB) sends operator 1 back into itself, so it bends itself with its own past. A sine fed back leans toward a saw: more FB is brighter, then rough, then noisy.
It is the cheapest brightness in FM: one operator, no partner.""", 5),
  KN(2, "Turn KNOB 2 up for FB", "V_FMCH", "O_GE", 4, CF_FB, 4),
  KEY("Press a key: a buzzy saw"),
  KN(2, "Turn KNOB 2 to the top", "V_FMCH", "O_GE", 7, CF_FB, 3),
  KEY("Press a key: nearly noise"),
  R("Algorithms", """An ALGORITHM is a wiring diagram. It says which operators bend which, and which reach the output: the CARRIERS.
Look at the picture on the screen. Boxes are operators, arrows are bending, and the boxes joined to OUT are the ones you hear.
Now all four operators are working.""", 6),
  R("Chains and organs", """ALGO 0 is a chain: 1 bends 2, 2 bends 3, 3 bends 4, and only 4 is heard. Each link multiplies the complexity: the richest, harshest sounds.
ALGO 7 is the opposite: four carriers side by side and nobody bends anybody. They simply ADD, like four oscillators or the drawbars of an organ."""),
  KN(1, "Turn KNOB 1 up to ALGO 7", "V_FMCH", "O_GE", 7, CF_ALG, 7),
  KEY("Press a key: four add up"),
  R("In between", """ALGO 4 is two pairs side by side, 1 bends 2 and 3 bends 4: two small FM voices layered.
ALGO 5: operator 1 bends 2, 3 and 4 at once: one modulator shaping three carriers.
ALGOs 1 to 3 merge two modulators into one chain, for more varied bends.
Choose the wiring first, then dial it in with TL and MUL."""),
  KN(1, "Turn KNOB 1 down to ALGO 4", "V_FMCH", "O_EQ", 4, CF_ALG, -3),
  KEY("Press a key: two voices"),
  KN(1, "Turn KNOB 1 down to ALGO 0", "V_FMCH", "O_LE", 0, CF_ALG, -4),
  KEY("Press a key: the chain"),
  R("Detune", """Two carriers at the same pitch simply add. Detune one by a few cents and the two drift in and out of step: the sound beats slowly. An instant chorus.
DT1 is the fine detune of one operator. Pick operator 2 and nudge it.""", 7),
  SEL("Press SEL twice for OP 2", 2, 2),
  KN(3, "Turn KNOB 3 for DT1", "V_FMOP", "O_CHG", 0, OPF(1, OF_DT1), 4),
  KEY("Press a key: it beats"),
  R("The LFO", """The LFO is a slow wobble you can aim. PMS aims it at the PITCH: vibrato. AMS aims it at the LOUDNESS: tremolo. SPEED and PMD set how fast and how deep.
Aim it at a modulator and it wobbles the brightness instead: a growl.""", 8),
  ALG(1, "Turn ALGORITHM to SPEED", 4),
  KN(1, "Turn KNOB 1 up for SPEED", "V_FMCH", "O_UP", 0, CF_LFS, 4),
  KN(3, "Turn KNOB 3 up for PMD", "V_FMCH", "O_UP", 0, CF_PMD, 4),
  ALG(2, "Turn ALGORITHM to PMS", 4),
  KN(1, "Turn KNOB 1 up for PMS", "V_FMCH", "O_GE", 3, CF_PMS, 3),
  KEY("Press a key: vibrato"),
  R("Recipes", """Electric piano: ALGO 4, modulator MUL 1, TL near 30, a quick D1R.
Bell: modulator MUL 7 plus DT2, a long carrier release.
Bass: ALGO 0, modulator TL 20, short D1R, a little FB.
Brass: a modulator attack slower than the carrier's.
Organ: ALGO 7, MULs 1, 2, 3, 4.
Play with what you have learned.""", 9),
  KEY("Press a key to play it"),
)

# ---------------------------------------------------------------------------------------------------------------------------------------------- SID
masterclass(2, "Masterclass: SID", "KD_SID",
  R("Taking sound away", """The SID works the opposite way round from FM. It starts with an oscillator that is full of harmonics and carves them away.
Three parts: the OSCILLATOR makes the raw wave, the FILTER removes harmonics, the ENVELOPE shapes loudness over time. Everything else moves one of the three.""", 1),
  KEY("Press a key: soft triangle"),
  R("Waves", """Triangle: soft, mostly the pitch alone. Saw: every harmonic, bright and buzzy. Pulse: hollow, its colour set by the width. Noise: every frequency at once.
Between them sit the combined waves: two shapes ANDed together in the chip, thinner and stranger.
Turn through them all."""),
  KN(1, "Turn KNOB 1 up to the saw", "V_SID", "O_EQ", 1, SP_WAVE, 1),
  KEY("Press a key: bright"),
  KN(1, "Turn KNOB 1 up to noise", "V_SID", "O_GE", 7, SP_WAVE, 6),
  KEY("Press a key: a hiss"),
  R("Pulse width", """A pulse wave is high for part of each cycle and low for the rest. The PW knob sets the high part. At 50% it is a square: only odd harmonics, hollow.
Squeeze it and the sound turns thin and nasal. Move it slowly and the sound seems to wander: the classic SID chorus. PWM does that moving for you.""", 2),
  ALG(3, "Turn ALGORITHM to PW", 0),
  KN(1, "Turn KNOB 1 down: thinner", "V_SID", "O_LE", 2, SP_PW, -6),
  KEY("Press a key: thin, nasal"),
  KN(2, "Turn KNOB 2 up for PWM", "V_SID", "O_GE", 3, SP_PWM, 3),
  KEY("Press a key: it wanders"),
  R("The filter", """A filter lets some frequencies through and holds others back. The CUT knob is the cutoff. A LOW PASS filter keeps what is below it: close it and the saw loses its top and goes dull.
Open it and the buzz comes back. Sweeping the cutoff is the oldest move in synthesis.""", 3),
  ALG(4, "Turn ALGORITHM to CUT", 0),
  KN(1, "Turn KNOB 1 up: open it", "V_SID", "O_GE", 12, SP_CUT, 9),
  KEY("Press a key: wide open"),
  R("Resonance and modes", """RES feeds the filter back on itself, so the frequencies right at the cutoff get louder and ring: a whistling edge that follows your sweep.
MODE picks what passes: L low, B band, H high, or a mix. LOW keeps the bottom, HIGH keeps the top, BAND keeps a slice around the cutoff."""),
  KN(2, "Turn KNOB 2 up for RES", "V_SID", "O_GE", 8, SP_RES, 8),
  KEY("Press a key: it sings"),
  KN(3, "Turn KNOB 3 to H (high)", "V_SID", "O_EQ", 4, SP_MODE, 3),
  KEY("Press a key: all top"),
  R("Filter envelope", """FENV lets the note's own envelope open the filter: the louder the note, the more the cutoff rises. Close the filter, give the note a short decay and add FENV: each note starts bright and closes: the pluck, the wow, the classic SID bass.""", 4),
  ALG(4, "Turn ALGORITHM to CUT", 0),
  KN(4, "Turn KNOB 4 up for FENV", "V_SID", "O_GE", 8, SP_FENV, 8),
  KEY("Press a key: the wow"),
  R("Hard sync", """Two oscillators run together. Every time the first finishes a cycle it forces the second back to the start, mid-wave. The pitch you hear stays the note, but the cut-off wave has a sharp, ringing character.
Change the second one's speed (SYNC, 1.25x to 12x) and the colour sweeps: the sync lead.""", 5),
  ALG(3, "Turn ALGORITHM to SYNC", 0),
  KN(3, "Turn KNOB 3 up for SYNC", "V_SID", "O_GE", 3, SP_SYNC, 3),
  KEY("Press a key: tearing lead"),
  KN(3, "Turn KNOB 3 up further", "V_SID", "O_GE", 10, SP_SYNC, 7),
  KEY("Press a key: it screams"),
  R("Ring modulation", """Ring modulation multiplies the triangle by a second oscillator. The result has the SUM and the DIFFERENCE of the two pitches, and neither is a harmonic of the note. Simple ratios such as 1 to 1 or 1 to 2 stay musical; others give clang, metal, bells.""", 6),
  ALG(3, "Turn ALGORITHM to PW", 0),
  KN(4, "Turn KNOB 4 up for RING", "V_SID", "O_GE", 6, SP_RING, 6),
  KEY("Press a key: metal"),
  R("6581 and 8580", """There are two SID chips. The 6581 (1982) has a dark, uneven filter that distorts as it closes, and combined waves that thin out. The 8580 (1986) is clean and bright, with a more even filter.
The same patch sounds different on each. CHIP switches between them.""", 7),
  KN(4, "Turn KNOB 4 to 8580", "V_SID", "O_EQ", 1, SP_MODEL, 1),
  KEY("Press a key: clean"),
  KN(4, "Turn KNOB 4 back to 6581", "V_SID", "O_EQ", 0, SP_MODEL, -1),
  KEY("Press a key: dark, dirty"),
  R("Recipes", """Bass: saw, low cutoff, FENV up, short decay.
Lead: pulse with PWM, or saw with a little resonance.
Pluck: triangle or pulse, fast decay, no sustain.
Metal bell: triangle with RING.
Drum: noise, closed filter, FENV up, a very short decay.
Now make your own.""", 9),
  KEY("Press a key to play it"),
)

# ---------------------------------------------------------------------------------------------------------------------------------------------- waves
masterclass(3, "Masterclass: Waves", "KD_PCE",
  R("A wave is a recipe", """Every repeating wave is a stack of sine waves at whole-number multiples of the pitch, the HARMONICS. The shape of the wave only decides how loud each harmonic is.
These chips play a table of 32 samples over and over. Change the table and you change the recipe.
This one is a pure sine.""", 1),
  KEY("Press a key: one harmonic"),
  R("The shape family", """SHAPE runs from pulse (0) through saw (5) and triangle (10) to sine (15).
Saw: every harmonic, falling as 1 over n. Square: only the odd ones, same slope. Triangle: odd harmonics falling much faster, as 1 over n squared: soft. Sine: just one."""),
  KN(1, "Turn KNOB 1 down to saw", "V_INF", "O_LE", 5, IF_WAVE, -10),
  KEY("Press a key: all harmonics"),
  KN(1, "Turn KNOB 1 up to triangle", "V_INF", "O_GE", 10, IF_WAVE, 5),
  KEY("Press a key: soft and odd"),
  KN(1, "Turn KNOB 1 down to pulse", "V_INF", "O_LE", 0, IF_WAVE, -10),
  KEY("Press a key: hollow"),
  R("Folding", """FOLD turns the wave up beyond its ceiling and bounces what sticks out back down, like a ball off a floor. A sine has no overtones, and folding it makes some from nothing.
A little fold: a gentle edge. A lot: many bounces and a dense, metallic spectrum.""", 2),
  ALG(3, "Turn ALGORITHM to FOLD", 0),
  KN(2, "Turn KNOB 2 up for FOLD", "V_INF", "O_GE", 6, IF_WF, 6),
  KEY("Press a key: an edge"),
  KN(2, "Turn KNOB 2 to the top", "V_INF", "O_GE", 15, IF_WF, 9),
  KEY("Press a key: metallic"),
  R("Steps", """STEP lowers how many heights the wave may use. With fewer levels the smooth curve turns into stairs, and every stair is a sharp edge, full of high harmonics.
It is bit crushing: the lower the resolution, the more grit. The old chips had few bits, and that grit is part of their sound.""", 3),
  ALG(3, "Turn ALGORITHM to FOLD", 0),
  KN(3, "Turn KNOB 3 up for STEP", "V_INF", "O_GE", 10, IF_WC, 10),
  KEY("Press a key: grit"),
  R("Twist", """TWIST bends WHEN things happen inside each cycle, with a slow wobble along the wave. The pitch does not change, but the shape does: energy moves between harmonics, like a vowel changing.
On a saw it adds a hollow, vocal movement.""", 4),
  ALG(3, "Turn ALGORITHM to FOLD", 0),
  KN(4, "Turn KNOB 4 up for TWIST", "V_INF", "O_GE", 8, IF_WT, 8),
  KEY("Press a key: vocal"),
  R("Draw your own", """On the DRAW page you set the 32 samples yourself. BRUSH paints several at once. MIRROR fills the other half for you: mirrored makes the shape symmetrical, flipped makes it the reverse and upside down, repeat copies the half twice.
A repeated half has half the period, so it sounds an octave higher.""", 0),
  ALG(5, "Turn ALGORITHM to BRUSH", 0, True),
  KN(1, "Turn KNOB 1 to draw", "V_WCUS", "O_EQ", 1, 0, 3),
  KEY("Press a key: your wave"),
  R("Why they sound old", """A PC Engine wave has 32 samples at 5 bits. The Game Boy has 4 bits, the SCC has 8. The coarser the table, the more grit and aliasing: tones that are not quite where you expect.
Those limits are the sound of the chips. Choose a chip for what its limits do to your wave."""),
  KEY("Press a key to play it"),
)

# ---------------------------------------------------------------------------------------------------------------------------------------------- drums
def PY(code): return ("py", code)
masterclass(4, "Masterclass: Drums", "KD_DRUM0 + 7",
  R("How drums are made", """A kick is a low sine that starts high and falls fast. That fall is the thump. In this kit it drops from 160 Hz to 48 Hz in about 30 milliseconds.
A snare is two things: a short tone near 200 Hz plus a burst of noise. Hats and cymbals are only noise, filtered to the top: closed is short, open rings, a crash rings for a second.""", 1),
  D("Press SEQ for the lanes", "SEQ", "V_VIEW", "O_EQ", 3, 0, PY("r.btn(SEQ)")),
  D("Press black key 3: a tom", "KEYS", "V_LANE", "O_EQ", 2, 0, PY("r.black(3)")),
  D("Press black key 1: kick", "KEYS", "V_LANE", "O_EQ", 0, 0, PY("r.black(1)")),
  R("Beats and steps", """The page is 16 steps: four beats of four. White key 1 is step 1, white key 5 is beat 2, key 9 beat 3, key 13 beat 4.
The most common rock beat is the BACKBEAT: kick on beats 1 and 3, snare on 2 and 4. The kick is the floor you stand on, the snare is the clap you move to."""),
  D("White key 1: kick on 1", "KEYS", "V_DHIT", "O_EQ", 1, 0, PY("r.white(1)")),
  D("White key 9: kick on 3", "KEYS", "V_DHIT", "O_EQ", 1, 8 << 4, PY("r.white(9)")),
  D("Press black key 2: snare", "KEYS", "V_LANE", "O_EQ", 1, 0, PY("r.black(2)")),
  D("White key 5: snare on 2", "KEYS", "V_DHIT", "O_EQ", 1, (4 << 4) | 1, PY("r.white(5)")),
  D("White key 13: snare on 4", "KEYS", "V_DHIT", "O_EQ", 1, (12 << 4) | 1, PY("r.white(13)")),
  D("Press PLAY: hear the beat", "PLAY", "V_RUN", "O_EQ", 1, 0, PY("r.btn(PLAY)")),
  R("The hat is the clock", """The closed hat does not carry the weight. It marks time. Hits on every other step, the eighth notes, give the beat a grid to sit on: it is the metronome you can feel.
Place them with the music playing, with your hands: the page loops and you hear each hit land.""", 0),
  D("Press black key 8: hat", "KEYS", "V_LANE", "O_EQ", 7, 0, PY("r.black(8)")),
  D("Add hats every 2nd step", "KEYS", "V_DLANE", "O_GE", 8, 7, PY("[r.white(n) for n in (1, 3, 5, 7, 9, 11, 13, 15)]")),
  R("Pushing the beat", """A hit that lands just BEFORE a beat leans into it and pushes the groove forward. That is syncopation. A kick on step 11, one step before beat 4 would be, makes the whole bar surge.
Rhythm is made as much by where you do not play as where you do.""", 0),
  D("Press black key 1: kick", "KEYS", "V_LANE", "O_EQ", 0, 0, PY("r.black(1)")),
  D("White key 11: kick push", "KEYS", "V_DHIT", "O_EQ", 1, (10 << 4), PY("r.white(11)")),
  R("Accents", """Every hit is either normal or ACCENTED: louder, and the bit of energy that makes a pattern breathe. A drummer leans on the backbeat and lets the rest fall away.
Hold ENV and press a key to accent a hit. Accent the snares, leave the hats quiet, and the pattern stands up."""),
  D("Press black key 2: snare", "KEYS", "V_LANE", "O_EQ", 1, 0, PY("r.black(2)")),
  D("Hold ENV, press white key", "ENV", "V_ACC", "O_UP", 0, 0, PY("r.ev('held 0x10', 100); r.white(5); r.ev('held 0', 100)")),
  R("Open and closed hats", """A real hi-hat has a pedal. The closed hat is the cymbals clamped together, the open hat is them apart and ringing. When the pedal closes, the ring stops dead.
Here too: a closed hat CHOKES an open one. Put an open hat before a closed hat and it is cut short.""", 0),
  D("Press black key 9: open", "KEYS", "V_LANE", "O_EQ", 8, 0, PY("r.black(9)")),
  D("White key 8: open hat", "KEYS", "V_DHIT", "O_EQ", 1, (7 << 4) | 8, PY("r.white(8)")),
  R("Odd groupings", """Not every groove is four beats. In 7/8 the bar is seven eighths, felt as 2 + 2 + 3: one long step at the end that pulls you back to the start.
KNOB 3 changes the meter. Pick 7/8 and move your snare to land on the second group.""", 0),
  D("Turn KNOB 3 to 7/8", "KNOB 3", "V_SIG", "O_EQ", 5, 0, ("knobs", 3, 3)),
  R("The kit is colour", """The pattern is only a score. The sound comes from the kit, and every console has its own: sines, pulses, poly counters, FM squares. The same beat on another console is a different record.
Tap EDIT on a channel to give it another console, and the notes stay.""", 0),
  D("Press PLAY to stop", "PLAY", "V_RUN", "O_EQ", 0, 0, PY("r.btn(PLAY)")),
)

import re
MAXL = 11                                         # lines a page holds (lessons.c les_read_draw: pitch 13)
def expand(items):                                # a long page becomes two or three: split at sentences, 11 lines each
    out = []
    for it in items:
        if not isinstance(it, R): out.append(it); continue
        sents = re.split(r"(?<=[.!?:])\s+", it.body)
        chunks, cur = [], []
        for sn in sents:
            if cur and len(textwrap.wrap(" ".join(cur + [sn]), 26)) > MAXL: chunks.append(" ".join(cur)); cur = []
            cur.append(sn)
        chunks.append(" ".join(cur))
        for k, ch in enumerate(chunks):
            r = R(it.title if k == 0 else ("%s %d" % (it.title[:17], k + 1)), ch, it.patch if k == 0 else 0)
            out.append(r)
    return out

def wrap(body):
    out = []
    for para in body.split("\n"):
        out += textwrap.wrap(para, 26) or [""]
    return out

def c_str(s): return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'

def entry(mid, name, kd, items):
    items = expand(items)
    parts = []
    for it in items:
        if isinstance(it, R):
            lines = wrap(it.body)
            assert len(lines) <= MAXL, (name, it.title, len(lines))
            assert len(it.title) <= 20, (name, it.title)
            parts.append('RD(%s, %s, %d)' % (c_str(it.title), c_str("\n".join(lines)), it.patch))
        else:
            t, p, v, op, n, a, _ = it
            assert len(t) <= 26, (name, t)
            parts.append('STA(%s, %s, %s, %s, %d, %d)' % (c_str(t), c_str(p), v, op, n, a))
    return '    {%s, S_MC, %d, (const lstep_t[]){%s}, %s, %d},\n' % (c_str(name), len(items), ", ".join(parts), kd, mid)

def actions(items):
    items = expand(items)
    out, cur = [], 0
    for it in items:
        if isinstance(it, R):
            if it.patch: cur = 0                                 # a patch puts the CHIP screen back on its first page
            out.append(("read",)); continue
        act = it[6]
        if act[0] == "alg":
            _, page, mod = act
            d = (page - cur) % mod if mod else page - cur
            out.append(("alg", d)); cur = page
        else:
            if act[0] == "sel": cur = cur                    # (the page stays)
            out.append(act)
    return out

FILES = {1: "fm", 2: "sid", 3: "waves", 4: "drums"}
def emit(root):
    for mid, name, kd, items in MC:
        with open(os.path.join(root, "firmware/src/lessons_mc_%s.h" % FILES[mid]), "w") as f:
            f.write("/* SPDX-License-Identifier: GPL-3.0-only */\n/* A masterclass, made by tools/masterclasses.py (edit that, not this). */\n")
            f.write(entry(mid, name, kd, items))

if __name__ == "__main__":
    emit(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print(len(MC), "masterclasses;", [len(m[3]) for m in MC], "items")
