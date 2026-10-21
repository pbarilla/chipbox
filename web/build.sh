#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The firmware's own sources built for the browser (site/play/chipbox.js + .wasm), with Emscripten in Docker.
#   web/build.sh
set -e
cd "$(dirname "$0")/.."
docker run --rm --platform linux/amd64 -v "$PWD":/w -w /w emscripten/emsdk:3.1.64 \
    emcc firmware/src/chipbox.c -O2 -DCHIPBOX_HOST -DCHIPBOX_WEB -DAPP_VER="\"${APP_VER:-web}\"" -Ifirmware/src -Ifirmware/gen \
    -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sINITIAL_MEMORY=33554432 -sMODULARIZE=1 -sEXPORT_NAME=Chipbox -sENVIRONMENT=web,node \
    -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU16,HEAP32 -sEXPORTED_FUNCTIONS=_main \
    -Wno-unused-function -o site/play/chipbox.js
ls -l site/play
