// SPDX-License-Identifier: GPL-3.0-only
// The Projects page: back up and restore projects, upload a file into a slot, send a sample.
import { Device, toFile, fromFile } from "./fm1proj.js";

const $ = (id) => document.getElementById(id);
const TEXT = {
  nomidi: "This browser can't talk to the FM-1.",
  denied: "MIDI was blocked. Allow it for this site and try again.",
  notfound: "No FM-1 found. Check the USB cable and close other MIDI apps. An older Chipbox needs an update first.",
  lost: "Lost the FM-1. Replug it and connect again.",
  read: "Couldn't read it.",
  w2: "That isn't a project this firmware can read.",
  w3: "That slot is the demo. Pick another.",
  w4: "Couldn't write it. Try again.",
  w5: "Something went out of order. Try again.",
  w6: "No such slot.",
  badfile: "That file isn't a Chipbox project.",
};
const say = (t) => { $("status").textContent = t; };
const prog = (f) => { const b = $("bar"); b.style.display = f == null ? "none" : "block"; if (f != null) b.value = Math.round(f * 100); };
const fail = (e) => {
  prog(null);
  if (e && e.code === "write") say(TEXT["w" + e.status] || TEXT.w4);
  else if (e && TEXT[e.code]) say(TEXT[e.code]);
  else if (e && e.name === "SecurityError") say(TEXT.denied);
  else say(e && e.message ? e.message : String(e));
};

let dev = null, info = null, samples = [], pending = null;     // pending: { slot, restore } for the file chooser

const save = (name, text) => {
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([text], { type: "application/json" }));
  a.download = name;
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 2000);
};
const size = (n) => (n < 1024 ? n + " B" : (n / 1024).toFixed(1) + " KB");
const firstFree = () => { for (let s = 1; s < info.slots.length; s++) if (!info.slots[s].used) return s; return 1; };
const mk = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text != null) e.textContent = text; return e; };
const btn = (text, cls, fn) => { const b = mk("button", cls, text); b.addEventListener("click", fn); return b; };

async function refresh() {
  info = await dev.info();
  $("mine").hidden = false;
  const box = $("slots");
  box.textContent = "";
  info.slots.forEach((s, i) => {
    const row = mk("div", "row" + (i === info.open ? " open" : ""));
    const what = mk("div", "what", i === 0 ? "Demo" : "Project " + i);
    what.append(mk("small", "", (s.used ? size(s.len) : "empty") + (i === info.open ? (info.dirty ? ", open, unsaved" : ", open") : "") + (i === 0 ? ", locked" : "")));
    row.append(mk("span", "n", String(i)), what);
    const b = mk("div", "btns");
    if (s.used) b.append(btn("Download", "alt", () => download(i)));
    if (i > 0) b.append(btn(s.used ? "Replace…" : "Upload…", "alt", () => chooseFor(i, b)));
    row.append(b);
    box.append(row);
  });
  renderSamples();
}

async function download(i) {
  try {
    say("Reading…");
    const bytes = await dev.read(i, prog);
    prog(null);
    save(i === 0 ? "demo.chipbox" : `project-${i}.chipbox`, toFile([{ name: i === 0 ? "Demo" : "Project " + i, bytes }]));
    say("Saved.");
  } catch (e) { fail(e); }
}

function chooseFor(slot, b) {
  const open = () => { pending = { slot }; $("pick").value = ""; $("pick").click(); };
  if (!info.slots[slot].used) { open(); return; }
  const was = [...b.childNodes];
  b.textContent = "";
  b.append(mk("span", "small", `Replaces slot ${slot}.`), btn("Pick a file", "warn", open), btn("Cancel", "alt", () => { b.textContent = ""; b.append(...was); }));
}

$("pick").addEventListener("change", async (ev) => {
  const f = ev.target.files[0];
  if (!f || !pending) return;
  const { slot, restore } = pending;
  pending = null;
  let list;
  try { list = fromFile(await f.text()); } catch (_) { fail({ code: "badfile" }); return; }
  try {
    if (restore) { await putAll(list); return; }
    await put(slot, list[0].bytes);
    say(`Slot ${slot} has it now, and it's open on the FM-1.`);
    await refresh();
  } catch (e) { fail(e); }
});

async function put(slot, bytes) {
  say(`Writing slot ${slot}…`);
  await dev.write(slot, bytes, prog);
  prog(null);
}

async function putAll(list) {
  const n = Math.min(list.length, info.slots.length - 1);
  for (let i = 0; i < n; i++) await put(i + 1, list[i].bytes);
  say(`Restored ${n} project${n === 1 ? "" : "s"} into slots 1 to ${n}.`);
  await refresh();
}

$("backup").addEventListener("click", async () => {
  try {
    const out = [];
    for (let i = 1; i < info.slots.length; i++) {
      if (!info.slots[i].used) continue;
      say(`Reading slot ${i}…`);
      out.push({ name: "Project " + i, bytes: await dev.read(i, prog) });
    }
    prog(null);
    if (!out.length) { say("Nothing to back up."); return; }
    save(`chipbox-backup-${new Date().toISOString().slice(0, 10)}.chipbox`, toFile(out));
    say(`Saved ${out.length} project${out.length === 1 ? "" : "s"}.`);
  } catch (e) { fail(e); }
});

let restoreArmed = false;
$("restore").addEventListener("click", () => {
  const b = $("restore");
  if (!restoreArmed) {
    restoreArmed = true;
    b.textContent = "This replaces your projects. Pick a file";
    b.className = "warn";
    return;
  }
  restoreArmed = false;
  b.textContent = "Restore a backup";
  b.className = "alt";
  pending = { slot: 0, restore: true };
  $("pick").value = "";
  $("pick").click();
});

function renderSamples() {
  const box = $("samples");
  box.textContent = "";
  for (const s of samples) {
    const row = mk("div", "row");
    const what = mk("div", "what", s.name);
    what.append(mk("small", "", s.text));
    const b = mk("div", "btns");
    const a = mk("a", "btn alt", "Download");
    a.href = s.file;
    a.download = s.file.split("/").pop();
    b.append(a);
    if (dev && info) {
      const sel = mk("select");
      for (let i = 1; i < info.slots.length; i++) sel.append(new Option(`Slot ${i}${info.slots[i].used ? " (replace)" : ""}`, String(i)));
      sel.value = String(firstFree());
      const send = btn("Send", "", () => sendSample(s, sel, send));
      sel.addEventListener("change", () => { send.textContent = "Send"; send.className = ""; send.armed = false; });
      b.append(sel, send);
    }
    row.append(what, b);
    box.append(row);
  }
}

async function sendSample(s, sel, send) {
  const slot = Number(sel.value);
  if (info.slots[slot].used && !send.armed) { send.armed = true; send.textContent = `Replace slot ${slot}?`; send.className = "warn"; return; }
  try {
    const r = await fetch(s.file);
    const list = fromFile(await r.text());
    await put(slot, list[0].bytes);
    say(`${s.name} is in slot ${slot} and open on the FM-1.`);
    await refresh();
  } catch (e) { fail(e); }
}

$("connect").addEventListener("click", async () => {
  if (!navigator.requestMIDIAccess) { say(TEXT.nomidi); return; }
  try {
    say("Looking…");
    dev = await Device.connect();
    await refresh();
    say("Connected.");
    $("connect").textContent = "Refresh";
  } catch (e) { dev = null; fail(e); }
});

(async () => {
  try { samples = await (await fetch("samples.json")).json(); } catch (_) { samples = []; }
  renderSamples();
})();
