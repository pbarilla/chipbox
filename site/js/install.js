// SPDX-License-Identifier: GPL-3.0-only
// The installer page's script, after the installer page of Felucca (hugelton/Felucca, web/index_pkg.html,
// Copyright (C) 2026 Leo Kuroshita, Hügelton Instruments). The protocol is fm1ota.js and fm1pkg.js, unchanged.
import { Updater } from "./fm1ota.js";
import { productOf, logicalImage, sha256hex } from "./fm1pkg.js";

const PRODUCT = "FM-1_337";                     // the identity Chipbox reports (tools/build.py)
const $ = (id) => document.getElementById(id);

const TEXT = {
  nomidi: "This browser can't do it. Open the page in Chrome or Edge.",
  norelease: "No release to install yet.",
  loadfail: "Could not load the firmware: ",
  badpkg: "The firmware file is damaged. Reload the page.",
  start: "Starting", verify: "Checking", loader: "Switching to update mode", write: "Writing",
  reboot: "Restarting", done: "Done. Press GLO on the FM-1 for help.",
  stopped: "Stopped. Replug the USB cable and press Install again.",
  error: "Error: ",
  denied: "MIDI (SysEx) wasn't allowed. Allow MIDI in the site settings (address bar) and press Install again.",
  notfound: "No FM-1 found. Check the USB cable (a data cable) and close other apps using MIDI, then press Install again.",
  model: "The connected device isn't an FM-1.",
  lost: "The USB connection was lost. Replug it and press Install again.",
  noloader: "The FM-1 in update mode wasn't found. Replug the USB cable and press Install again (it stays in update mode until it's finished).",
  noreturn: "The FM-1 didn't come back. Power-cycle it.",
  mismatch: "Written, but the FM-1 reports another firmware: ",
  leave: "Writing. Don't close the page.",
};
const say = (k, arg = "") => { $("status").textContent = (TEXT[k] ?? k) + arg; };
const log = (s) => { $("log").textContent += s + "\n"; };

let image = null, writing = false;
const lock = (on) => { writing = on; $("go").disabled = on || !image; };
window.addEventListener("beforeunload", (e) => { if (writing) { e.preventDefault(); e.returnValue = TEXT.leave; } });

(async () => {
  if (!navigator.requestMIDIAccess) { say("nomidi"); return; }
  try {
    const m = await fetch("firmware/latest.json", { cache: "no-cache" });
    if (!m.ok) { say("norelease"); return; }
    const meta = await m.json();
    const r = await fetch("firmware/" + meta.file, { cache: "no-cache" });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    const pkg = new Uint8Array(await r.arrayBuffer());
    if (await sha256hex(pkg) !== meta.sha256 || productOf(pkg) !== PRODUCT) { say("badpkg"); return; }
    image = logicalImage(pkg);
    $("version").textContent = "Chipbox " + meta.version;
    say("");
    $("go").disabled = false;
  } catch (e) { say("loadfail", e.message); }
})();

$("go").addEventListener("click", async () => {
  if (writing || !image) return;
  lock(true);
  let midi;
  try { midi = await navigator.requestMIDIAccess({ sysex: true }); }
  catch (e) { say("denied"); lock(false); return; }
  try {
    const up = new Updater(midi, log);
    const total = 74 + Math.ceil(image.length / 512);        // a guess for the bar: the checking reads, then the writing reads
    const step = (k, a) => {
      log(`${k} ${a ?? ""}`);
      if (k === "verify") { say("verify"); $("bar").value = Math.min(99, (a / total) * 100); }
      else if (k === "write") { say("write"); $("bar").value = Math.min(99, ((74 + a) / total) * 100); }
      else if (k === "done") { $("bar").value = 100; say("done"); }
      else say(k);
    };
    const loader = await up.find((id) => /^ota-/i.test(id.model));     // an earlier install was cut off: finish it
    if (loader) {
      loader.link.close();
      if (await up.resume(image, step, PRODUCT)) say("done"); else say("stopped");
    } else {
      await up.install(image, PRODUCT, step);
    }
  } catch (e) {
    if (e && e.code && TEXT[e.code]) say(e.code, e.detail || "");
    else say("error", e && e.message);
    log(String((e && e.stack) || e));
  }
  lock(false);
});
