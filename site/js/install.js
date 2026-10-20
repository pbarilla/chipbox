// SPDX-License-Identifier: GPL-3.0-only
// The installer page's script, after the installer page of Felucca (hugelton/Felucca, web/index_pkg.html,
// Copyright (C) 2026 Leo Kuroshita, Hügelton Instruments). The protocol is fm1ota.js and fm1pkg.js, unchanged.
import { Updater } from "./fm1ota.js";
import { productOf, logicalImage, sha256hex } from "./fm1pkg.js";

const PRODUCT = "FM-1_337";                     // the identity Chipbox reports (tools/build.py)
const $ = (id) => document.getElementById(id);

const TEXT = {
  nomidi: "Use Chrome or Edge.",
  norelease: "Nothing to install yet.",
  loadfail: "Couldn't load it: ",
  badpkg: "The file is damaged. Reload.",
  start: "Starting", verify: "Checking", loader: "Switching to update mode", write: "Writing",
  reboot: "Restarting", done: "Done. Press GLO on the FM-1 for help.",
  stopped: "Stopped. Replug it and try again.",
  error: "Error: ",
  denied: "MIDI was blocked. Allow it for this site and try again.",
  notfound: "No FM-1 found. Check the USB cable and close other MIDI apps.",
  model: "That isn't an FM-1.",
  lost: "Lost the USB connection. Replug it and try again.",
  noloader: "Lost the FM-1. Replug it and try again.",
  noreturn: "It didn't come back. Turn it off and on.",
  mismatch: "Done, but it reports: ",
  leave: "Writing. Keep this page open.",
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
