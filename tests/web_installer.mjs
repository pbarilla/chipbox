// SPDX-License-Identifier: GPL-3.0-only
// Node check of the site's installer (site/js/fm1ota.js, fm1pkg.js) against a simulated FM-1: no browser, no hardware.
//   node tests/web_installer.mjs [package.fwsc]       (default: build/chipbox.fwsc)
// The simulated device (the update loader is "Felucca Update", identity ota-FM-1_900, as firmware/loader/loader.c) is after Felucca's web/test_web.mjs (Copyright (C) 2026 Leo Kuroshita, Hügelton Instruments, GPL-3.0-only).
import { existsSync, readFileSync } from "node:fs";
import { logicalImage, productOf, sha256hex } from "../site/js/fm1pkg.js";
import { Updater, pack7, unpack7 } from "../site/js/fm1ota.js";

let failed = 0;
const ok = (cond, what) => { console.log(`${what.padEnd(70)} ${cond ? "ok" : "FAIL"}`); if (!cond) failed++; };
const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];

/* an FM-1 on WebMIDI: identity, then "device asks, host answers" reads of the image */
class FakeFM1 {
  constructor(image, product, { unplugAfter = Infinity } = {}) {
    this.image = image; this.product = product; this.unplugAfter = unplugAfter; this.served = 0; this.bad = 0;
    this.access = { inputs: new Map(), outputs: new Map() };
    this.boot("FM-1_015", "FM-1");
  }
  boot(identity, name) {
    this.identity = identity; this.waiting = null; this.queue = [];
    for (const m of [this.access.inputs, this.access.outputs]) { for (const p of m.values()) p.state = "disconnected"; m.clear(); }
    const id = Math.random().toString(36).slice(2);
    this.input = { id: "i" + id, name, state: "connected", onmidimessage: null, open: async () => {} };
    this.output = { id: "o" + id, name, state: "connected", open: async () => {}, send: (d) => {
      if (this.output.state !== "connected") throw new Error("InvalidStateError");
      setTimeout(() => this.rx(Array.from(d)), 1);
    } };
    this.access.inputs.set(this.input.id, this.input);
    this.access.outputs.set(this.output.id, this.output);
  }
  tx(bytes) { const i = this.input; setTimeout(() => { if (i.state === "connected" && i.onmidimessage) i.onmidimessage({ data: Uint8Array.from(bytes) }); }, 1); }
  rx(d) {
    if (eq(d, HS)) {
      const t = [...new TextEncoder().encode(this.identity)];
      this.tx([0xF0, ...pack7([0, 0x59, 0x11, 0, 0, 0, ...t, ...new Array(28 - t.length).fill(0)]), 0xF7]);
    } else if (eq(d, UPGRADE)) {
      this.queue = this.identity.startsWith("ota-")
        ? [...Array.from({ length: 6 }, (_, k) => [k * 512, 512]), [0xF0000000, 8]]
        : [[0, 64], [0x40, 160], [0x1000, 512], [0xE0000000, 8]];
      this.next();
    } else if (this.waiting) {
      const u = unpack7(d.slice(1, -1));
      const [addr, len] = this.waiting;
      const got = u.slice(14, 14 + (addr >= 0xE0000000 ? 8 : len));
      const want = addr >= 0xE0000000 ? [...new TextEncoder().encode("success"), 0] : Array.from(this.image.subarray(addr, addr + len));
      if (!eq(got, want)) this.bad++;
      this.waiting = null;
      this.served++;
      if (this.served >= this.unplugAfter) { this.input.state = this.output.state = "disconnected"; return; }
      if (addr === 0xE0000000) setTimeout(() => this.boot("ota-FM-1_900", "Felucca Update"), 300);
      else if (addr === 0xF0000000) setTimeout(() => this.boot(this.product, "FM-1"), 300);
      else this.next();
    }
  }
  next() {
    const r = this.queue.shift();
    if (!r) return;
    this.waiting = r;
    const [addr, len] = r;
    const u = [0, 0x59, 0x30, 0, 0, 0, 0, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF, len & 0xFF, len >> 8, 0];
    let s = 0;
    for (let i = 6; i < 14; i++) s += u[i];
    u.push(~s & 0xFF);
    this.tx([0xF0, ...pack7(u), 0xF7]);
  }
}

const path = process.argv[2] || new URL("../build/chipbox.fwsc", import.meta.url).pathname;
if (!existsSync(path)) { console.log(`skipped: no package at ${path} (run ./build.sh first)`); process.exit(0); }
const raw = new Uint8Array(readFileSync(path));
const product = productOf(raw);
const image = logicalImage(raw);
ok(product === "FM-1_337", `the package says ${product}`);
ok(image.length === raw.length - 20, "logicalImage drops the 20 marker bytes");
ok((await sha256hex(raw)).length === 64, "sha256hex");

const dev = new FakeFM1(image, product);
const steps = [];
const got = await new Updater(dev.access).install(image, product, (k) => steps.push(k));
ok(got === product && dev.bad === 0 && steps.includes("write") && steps.at(-1) === "done", `full install: running firmware -> loader -> ${product} (${dev.served} reads)`);

const dev2 = new FakeFM1(image, product, { unplugAfter: 3 });
dev2.boot("ota-FM-1_900", "Felucca Update");
const t0 = Date.now();
ok((await new Updater(dev2.access).resume(image)) === false && Date.now() - t0 < 6000, "unplugged during the write: stops at once");

const dev3 = new FakeFM1(image, product, { unplugAfter: 2 });
const e = await new Updater(dev3.access).install(image, product).then(() => null, (x) => x);
ok(e && e.code === "lost", "unplugged while checking: error 'lost', nothing written");

const e2 = await new Updater({ inputs: new Map(), outputs: new Map() }).install(image, product).then(() => null, (x) => x);
ok(e2 && e2.code === "notfound", "no device: error 'notfound'");

const dev4 = new FakeFM1(image, product);
dev4.identity = "SMK_037";
const e3 = await new Updater(dev4.access).install(image, product).then(() => null, (x) => x);
ok(e3 && e3.code === "model", "another model on USB: error 'model', nothing written");

console.log(failed ? `WEB INSTALLER TESTS FAILED (${failed})` : "web installer tests passed");
process.exit(failed ? 1 : 0);
