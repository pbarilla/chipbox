#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Plays a note on each chip over USB-MIDI and records the FM-1's USB audio (the "Felucca" audio device) with ffmpeg.
Prints the CPU load the firmware reports (control change 100). Needs the terminal to have macOS microphone permission.
    python3 tests/device_capture.py OUT_DIR          needs mido, python-rtmidi, ffmpeg; FM-1 running Chipbox"""
import subprocess, sys, time
import mido

out_dir = sys.argv[1]
names = mido.get_output_names()
port = [n for n in names if "FM" in n or "X0X" in n or "Felucca" in n][0]
out = mido.open_output(port)
inp = mido.open_input([n for n in mido.get_input_names() if port.split()[0] in n][0])
# (chip, midi channel, note, label): program change picks the chip, the MIDI channel picks the chip channel
plan = [(0, 0, 60, "nes_pulse"), (0, 2, 48, "nes_tri"), (1, 0, 60, "gb_pulse"), (1, 1, 60, "gb_pulse2"),
        (1, 2, 48, "gb_wave"), (2, 0, 60, "sms_tone")]
total = len(plan) * 2.5 + 4
rec = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "avfoundation", "-i", ":Felucca", "-t", str(total), "-ar", "44100", "-ac", "2",
                        f"{out_dir}/device.wav"])
time.sleep(1.5)                                  # let the capture start before the first note
t0 = time.time(); marks = []; cc = []; cur = -1
for chip, ch, note, label in plan:
    if chip != cur:
        out.send(mido.Message("program_change", program=chip, channel=0)); cur = chip; time.sleep(0.4)
    marks.append((label, round(time.time() - t0, 2)))
    out.send(mido.Message("note_on", note=note, velocity=127, channel=ch)); time.sleep(1.6)
    out.send(mido.Message("note_off", note=note, channel=ch)); time.sleep(0.9)
    cc += [m.value for m in inp.iter_pending() if m.type == "control_change" and m.control == 100]
try:
    rec.wait(timeout=total + 15)
except subprocess.TimeoutExpired:
    rec.kill(); print("ffmpeg produced no audio in time: microphone permission, or the device is not streaming")
print("marks (label, seconds from start of notes; capture started 1.5 s earlier):", marks)
print("CPU % reports:", cc)
