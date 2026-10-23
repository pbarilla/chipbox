# Chipbox

Chiptune 'grovebox' thing with NES, GB, and SMS stuff

<p>
<img src="site/img/rack.png" width="32%"> <img src="site/img/chip.png" width="32%"> <img src="site/img/song.png" width="32%">
</p>

## What is it

A simple (to me) workflow for making music. Theres 3 main views you'll spend all your time in:

RACK - this is the current page (16 steps). Use this to add channels

CHIPs - this is where you can config each channel. You pick a channel from the hardware first (NES, GB, SMS), and then what channel (like PU1, WAV, etc). If you want appregio, make a channel and change the shape to an appregio shape.

SONG - this is where you stitch together the pages into a song

## Terminology

- Channel: Can add up to 16, but they wont ever be able to play all at once. You define a Patch which sits on a channel.
- Patch: This is how you specify how the sound...sounds. In lieu of _adding accents to notes in other channels_, you define exactly how the sound should sound and reuse. Eg. you might want to spice up a melody by adding an ARP effect. You'd duplicate the patch, change the shape to ARP, and configure whatever else you want. That'd sit in its own channel.

## Play online

[ihavenocluewhatimdoing.com/chipbox/play](https://ihavenocluewhatimdoing.com/chipbox/play/)

## Install

[ihavenocluewhatimdoing.com/chipbox/install.html](https://ihavenocluewhatimdoing.com/chipbox/install.html)

## Tutorial

[ihavenocluewhatimdoing.com/chipbox/manual](https://ihavenocluewhatimdoing.com/chipbox/manual/)

## Tested?

- Tested on a few real FM-1's!

## Build

Note: I've only ever ran this on a mac. good luck for other platforms

```sh
tools/get_toolchain.sh                      # clang for the chip, to ~/.jieli/toolchain
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
./build.sh                                  # -> build/chipbox.fwsc
./build.sh --release 1.0.0                  # -> build/chipbox-1.0.0.fwsc
tests/run_host.sh tests/screens.txt         # on Linux: the screens as PNG, no FM-1 needed
```

On a Mac, run the host simulator in Docker (see `docs/RELEASING.md`).

## Licence

GPL-3.0-only, see `LICENSING.md`. Built on [Dinghy](https://github.com/zvenson/dinghy), which is built on
[Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments) with the changes of
[SLOOP](https://github.com/isod89/sloop-fm1) (isod89). `docs/API.md` and `docs/RULES.md` describe the platform and
come from Dinghy.
