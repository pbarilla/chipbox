#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Everything that runs on a computer, under the address and undefined-behaviour sanitizers: the engine fuzz (kinds_hash, fds_fuzz),
# every host script, random input (tests/fuzz.py, FUZZ seeds of FUZZ_SECS seconds each), the installer test. Prints FAIL lines; exit 1 if any.
#   FUZZ=12 FUZZ_SECS=900 sh tests/run_all.sh
cd "$(dirname "$0")/.." || exit 1
mkdir -p build/host/asan
fail=0
CF="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined"
for t in kinds_hash fds_fuzz; do
    cc $CF -DPOOL= '-D__attribute__(x)=' -DCHIPS_C='"chips.c"' -Itests -Ifirmware/src -Ifirmware/gen -o build/host/$t tests/$t.c -lm 2>/dev/null || { echo "FAIL build $t"; fail=1; continue; }
    build/host/$t 1 > build/host/asan/$t.log 2>&1 && ! grep -q "runtime error\|AddressSanitizer" build/host/asan/$t.log && echo "ok   $t" || { echo "FAIL $t"; tail -3 build/host/asan/$t.log; fail=1; }
done
cc -O2 -DPOOL= '-D__attribute__(x)=' -Itests -Ifirmware/src -Ifirmware/gen tests/level_check.c -lm -o build/host/level_check 2>/dev/null && build/host/level_check check > build/host/asan/level_check.log 2>&1 && echo "ok   level_check" || { echo "FAIL level_check"; fail=1; }
cc $CF -DCHIPBOX_HOST -DPOOL= '-D__attribute__(x)=' -DAPP_VER='"dev"' -Ifirmware/src -Ifirmware/gen -o build/host/chipbox_san firmware/src/chipbox.c -lm 2>/dev/null || { echo "FAIL build chipbox_san"; exit 1; }
run() {                                         # run NAME SCRIPT
    rm -rf build/host/asan/$1 build/host/asan/$1.bin; mkdir -p build/host/asan/$1
    HOST_FLASH=build/host/asan/$1.bin timeout 1800 build/host/chipbox_san build/host/asan/$1/f $2 > build/host/asan/$1.log 2>&1 || { echo "FAIL $1 (exit $?)"; tail -3 build/host/asan/$1.log; fail=1; return; }
    grep -q "runtime error\|AddressSanitizer" build/host/asan/$1.log && { echo "FAIL $1 (sanitizer)"; fail=1; } || echo "ok   $1"
    rm -rf build/host/asan/$1
}
for f in tests/*.txt; do run "$(basename $f .txt)" "$f"; done
n=0
while [ $n -lt ${FUZZ:-3} ]; do
    n=$((n + 1)); python3 tests/fuzz.py $n ${FUZZ_SECS:-300} > build/host/asan/fuzz$n.txt; run fuzz$n build/host/asan/fuzz$n.txt
done
node tests/web_installer.mjs build/chipbox.fwsc > build/host/asan/installer.log 2>&1 && echo "ok   web_installer" || { echo "FAIL web_installer"; tail -3 build/host/asan/installer.log; fail=1; }
exit $fail
