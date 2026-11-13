#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Projects over USB-MIDI from a terminal (firmware/src/projio.c; the Projects page does the same in a browser).
#   fm1_projects.py info                      slots, their sizes, the open one
#   fm1_projects.py read SLOT FILE.bin        a slot's bytes
#   fm1_projects.py write SLOT FILE.chipbox   a .chipbox file's first project (or a raw .bin) into SLOT (never slots 0 / 1: the demos; refuses a used slot unless --force)
import base64, json, os, queue, sys, time
import mido

CHUNK = 200


def pack7(data):
    out, acc, nb = [], 0, 0
    for b in data:
        acc |= b << nb
        nb += 8
        while nb >= 7:
            out.append(acc & 0x7F)
            acc >>= 7
            nb -= 7
    if nb:
        out.append(acc & 0x7F)
    return bytes(out)


def unpack7(s):
    out, acc, nb = [], 0, 0
    for b in s:
        acc |= (b & 0x7F) << nb
        nb += 7
        while nb >= 8:
            out.append(acc & 0xFF)
            acc >>= 8
            nb -= 8
    return bytes(out)


class Dev:
    def __init__(self):
        names = [n for n in mido.get_input_names() if "FM-1" in n or "Felucca" in n or "X0X" in n]
        if os.environ.get("FM1_PORT"): names = [n for n in names if n == os.environ["FM1_PORT"]]       # two FM-1s: FM1_PORT="FM-1 Chipbox" picks one (check its identity with fm1_install.py --info --port first)
        if not names:
            raise SystemExit("FM-1 not found")
        self.out = mido.open_output(names[0])
        self.inp = mido.open_input(names[0])

    def ask(self, cmd, body, secs=3.0):
        for _ in range(3):
            for _m in self.inp.iter_pending():
                pass
            m = [0, 0x59, cmd, len(body) & 255, len(body) >> 8, 0] + list(body)
            m.append(~sum(body) & 255)
            self.out.send(mido.Message("sysex", data=pack7(m)))
            end = time.time() + secs
            while time.time() < end:
                for msg in self.inp.iter_pending():
                    if msg.type == "sysex":
                        u = unpack7(bytes(msg.data))
                        if len(u) >= 7 and u[0] == 0 and u[1] == 0x59 and u[2] == cmd:
                            return u[6:-1]
                time.sleep(0.005)
        raise SystemExit("no answer from the FM-1")

    def info(self):
        r = self.ask(0x50, [0])
        return {"open": r[2], "dirty": r[3], "slots": [(r[4 + s * 3], r[5 + s * 3] | r[6 + s * 3] << 8) for s in range(r[1])]}

    def read(self, slot):
        out, off, total = bytearray(), 0, 1
        while off < total:
            r = self.ask(0x51, [slot, off & 255, off >> 8])
            if r[0] == 7:
                return None
            if r[0] != 0:
                raise SystemExit("read status %d" % r[0])
            total = r[2] | r[3] << 8
            out += r[6:]
            off += len(r[6:])
            if not r[6:]:
                break
        return bytes(out)

    def write(self, slot, data):
        off, st = 0, 0
        while off < len(data):
            part = list(data[off:off + CHUNK])
            r = self.ask(0x52, [slot, len(data) & 255, len(data) >> 8, off & 255, off >> 8] + part, 6.0)
            st = r[0]
            if st > 1:
                raise SystemExit("write status %d" % st)
            off += len(part)
        return st


def load(path):
    if path.endswith(".bin"):
        return open(path, "rb").read()
    return base64.b64decode(json.load(open(path))["projects"][0]["data"])


if __name__ == "__main__":
    a = sys.argv[1:]
    d = Dev()
    if a[:1] == ["info"]:
        i = d.info()
        print("open", i["open"], "unsaved", i["dirty"])
        for s, (used, n) in enumerate(i["slots"]):
            print(s, "used %d B" % n if used else "empty")
    elif a[:1] == ["read"]:
        b = d.read(int(a[1]))
        open(a[2], "wb").write(b or b"")
        print(len(b or b""), "bytes")
    elif a[:1] == ["write"]:
        slot = int(a[1])
        if slot < 2:
            raise SystemExit("slots 0 and 1 are the demos")
        if d.info()["slots"][slot][0] and "--force" not in a:
            raise SystemExit("slot %d is used" % slot)
        print("status", d.write(slot, load(a[2])))
    else:
        print(__doc__)
