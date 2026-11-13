# Plan (2026-11-13)

## 0. Now: Drumlab on the link
Chipbox (FM-1_337, port "FM-1 Smileyface") leads the Meters waltz, Drumlab (FM-1_032, port "FM-1 Chipbox") follows.
- Sync: Chipbox Lead, Drumlab Follow, Patterns On.
- Check: it starts at the bar line, keeps the tempo through every page tempo and glide (no drift over a loop), changes
  pattern when the leader does, never plays garbage when the leader is on a page it has nothing for, no crash.
- Then Smileyface again (the follower rule), if the user wants the unit back.
- A long soak (an hour or more, with a pattern change every few minutes) is optional; ask for a reminder.

## 1a. Documentation first (2026-11-13: the user's call, before the release)
Done: GLO pages (Step Entry: Meter, Bars, Long pages, Page tempo, Glide; Rack and Song: Page meter / tempo; Menu: Look; Link
options), controls.json and the cheat sheet, the guide's Meter and tempo section, the menu rows and the numbers table, four
tutorials (Odd Meters, Long Pages, Tempo Changes, Glide) with box art, the READMEs (Chipbox, Smileyface, Drumlab), the Brothers
Link spec and agent file (three identical copies), the hub card (`fmhub.json` what), release note drafts (`RELEASE-NOTES.md`).
To do: the on-device lessons (below), a capstone tutorial that builds a short waltz from nothing (optional), flash the
manual changes (they are built into the firmware: rebuild and flash Chipbox, Smileyface, Drumlab before the release), look
through the site pages once in a browser (`site/manual/*/index.html`, `site/guide/`).

### On-device lessons for Chipbox (design, waiting for a yes; sketch in the mockups page)
- Where: GLO + HOME menu, a "Lessons" row (KNOB 1 picks the lesson, OCT+ starts it), as Drumlab does.
- What it is: a card over the bottom of the screen (where the keyboard is in step entry, empty in the rack) with the lesson's
  name and step count, one instruction in verb-first words, the control to use in a pill. A step finishes by itself when the
  state shows it happened (the page is 3/4, the page has a tempo of its own, a glide is set, the song plays ...), the card
  goes green, and the next step shows. The last step says nice and offers the next lesson.
- Show and hide: GLO taps the card away and back while a lesson runs (a small "LESSON 3/5" tab stays). The manual is still
  one GLO + HOME away.
- The project: a lesson works on a scratch copy of a tiny project (one channel, a few notes); leaving puts the user's project
  back exactly (the packed project is kept in the pool, 88 KB are free). Nothing the lesson does is saved.
- First lessons: the four tutorials. Later: Tetris and Amen Break as lessons, and one for each audit item as it lands.
- No auto popups, ever; the user starts it and ends it.
- Size guess: a state machine of steps (text, control to name, a predicate), a card drawer, the menu row: about 400 lines.

## 1. Release (the user commits, tags and pushes)
Versions now: Chipbox v1.6.1, Smileyface v1.2.1, Drumlab v0.1.4 (nothing committed since).
- Chipbox v1.7.0 (something new you can do): a meter and a length for every page (2/4 to 7/8, up to 64 steps), tempo
  per page, glide with three shapes, the battery and "Dancing Animations" switches, a faster, deeper undo, 88 KB more
  free memory, four lessons, a guide section, new test songs (`Meters waltz`).
- Smileyface v1.2.2 and Drumlab v0.1.5 (fixes): a follower ignores a leader's pattern it has nothing for (Drumlab also
  range-checks the number).
- Before tagging: `./build.sh --release X.Y.Z` in each folder, check `loader: ota.bin 8544 B`, flash and try it, run
  `python3 tests/meter_seq.py`, `undo_delta.py`, `proj_fuzz.py`, the host scripts, check `fmhub.json` `what`, the site
  (`site/manual/`, `site/guide/`, the cheat sheet is built by the workflow), `docs/RELEASING.md` steps.
- Not yet checked by hand (the user, on the device): REC across screens of a long page, KNOB 3 slide on a long page,
  drum lanes on a 64 step page.

## 2. The audit items (after the release)
Rules for all of them: no new screens; the simple workflow stays as it is (a project that never touches a new control
sounds and looks exactly as before, and saves in the old format); one control, one job, drawn not written; every item
gets a GLO page and a lesson; old projects load; the free keys and knobs are the budget:
- free buttons: ARP (note tails; could move to the menu), ENV (accents on drums only), SEL (a tap is free in most views)
- free knobs: Song view KNOB 2, 3 and 4.

| # | Item | Where | Hands | Saved as |
|---|------|-------|-------|----------|
| 1 | Song slot repeats | Song view | KNOB 2 on a slot: repeats 1..8 (a pip on the tile) | a repeat per slot (trailer) |
| 2 | Swing | Song view | KNOB 3: swing, for the song (the Link already carries a swing number) | one byte |
| 3 | Mute a channel | Rack | SEL tap on a channel (SEL hold is still copy) | not saved (a performance thing) or one bit |
| 3b | Velocity | Step entry | ENV tap on the note under the cursor: soft / normal / accent | the step's velocity byte (already there) |
| 4 | Count-in, metronome | menu rows | "Count-in: Off / 1 bar", "Metronome: Off / On"; the Link leader waits a bar so followers can switch first | settings bits |
| 4b | Tap tempo | any view | needs a key: ARP if the tails move to the menu | no |
| 5 | Note slide (portamento) | Chip screen | a PORTA knob on an extra page (the `x_tab` mechanism) | the channel's extra bytes |
| 6 | Transpose per slot (key changes, chord progressions) | Song view | KNOB 4 on a slot: -12..+12 semitones; the Link's key follows | a transpose per slot |
| 7 | A length per channel (polymeter) | Chip screen | a STEPS knob on an extra page | the channel's extra bytes |
| 8 | Ratchets, step probability | Step entry | only if asked | later |
| 9 | Automation (the woovebox way) | needs its own design | record a knob's moves for a page | later |
| 10 | Lessons on the device | GLO | a lesson the user can show and hide over the screen (the guided overlay tried on 2026-10-28 was dropped for popping up by itself; this one is asked for) | none |
| 11 | An S-curve glide | Step entry | one more shape | needs a bit |

Suggested order: 1, 2 and 6 together (the Song view "arrangement" knobs: one release, one lesson), then 3 and 3b, then
4, then 5 and 7, then 10, then 9.

## 3. Smaller things waiting
See `TODO.md`: punch-in FX over Brothers Link, glide S-curve, followers' finer controls, the artifacts if they come back.
