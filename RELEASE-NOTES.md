# Release notes (drafts, 2026-11-13)

## Chipbox v1.7.0

**New**
- 74 guided lessons (four of them masterclasses) for the whole workflow (get around, first notes, pages, song, sound, drums, mixing, FX, undo, meters, tempo, glide, a first song), in the first line of the GLO + HOME menu.
- Every page has its own meter: 2/4, 3/4, 4/4, 5/4, 6/8 or 7/8. In step entry, KNOB 3 picks it.
- Every page has its own length, up to 64 steps. KNOB 4 adds bars. A long page shows 16 steps at a time, and a line under the top bar shows which screen you're on.
- Steps past a page's end are crossed out and skipped. Notes on them are kept, so nothing is lost when you shorten a page.
- Every page can have its own tempo. In step entry, KNOB 1 gives the page a tempo of its own (amber). Turn it back to let go.
- A page can glide into the next page's tempo: tap SEL in step entry. Straight, ease in or ease out, then off. A small graph by the tempo shows it.
- The beat dots under the tempo follow the meter.
- GLO + HOME: Dancing Animations and Show Battery. The battery is in the top bar.
- Four new tutorials (Odd Meters, Long Pages, Tempo Changes, Glide), a Meter and tempo section in the manual, new GLO pages, and two samples: Meters waltz and Test: meters and a waltz.

**Under the hood**
- Steps are stored in blocks, only where there are notes. Undo is a stack of small changes instead of whole copies, so it goes about 30 steps back instead of 6, and 88 KB of memory is free again.
- Projects that don't use the new things are saved exactly as before. Old projects load unchanged.
- Brothers Link: the leader sends no kick pattern for pages that aren't 16 steps. Followers skip a page they have nothing in. A leader on a page above 15 sends none.

**Fixed**
- A speck on the top bar (or the rows under it) after a stalled screen transfer heals within a second.

## Smileyface v1.2.2
- A follower skips a leader's pattern that has nothing in it. Before, it played a blank pattern.
- 13 guided lessons in the first line of the GLO + HOME menu: a card says what to do and goes green when you have. GLO hides it. Your project comes back when it ends.

## Drumlab v0.1.5
- A follower skips a leader's pattern that has nothing in it, and a pattern number beyond its sixteen no longer reads outside its patterns.
- The GLO + HOME menu has more room between the lines and scrolls.
- 17 guided lessons: a card says what to do with your hands and goes green when you have done it (first beat, accents, tempo, patterns, chains, mute and solo, punch-in FX, undo, the sound lab, patches A and B, kits, the PO-32 send window). GLO hides the card.
