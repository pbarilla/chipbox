#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run firmware/src/chipbox.c on this computer (tests/host.c): build/host/chipbox.wav and the screens as PNG.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/host
cc -O2 -Wall -Wno-unused-function -DCHIPBOX_HOST $HOST_CFLAGS -DAPP_VER="\"${APP_VER:-dev}\"" -Ifirmware/src -Ifirmware/gen -o build/host/chipbox firmware/src/chipbox.c
build/host/chipbox build/host/chipbox ${1:+"$1"} ${2:+"$2"}
python3 - <<'PY'
from pathlib import Path
from PIL import Image
for p in sorted(Path("build/host").glob("chipbox-*.ppm")):
    Image.open(p).resize((480, 480), Image.NEAREST).save(p.with_suffix(".png"))
    p.unlink()
    print("screen ->", p.with_suffix(".png"))
PY
