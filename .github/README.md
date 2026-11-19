# ClassicStation

[![Build](https://github.com/DefKorns/ClassicStation/actions/workflows/build.yml/badge.svg)](https://github.com/DefKorns/ClassicStation/actions/workflows/build.yml)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit)](https://github.com/pre-commit/pre-commit)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE.md)

![ClassicStation, EmulationStation for the Nintendo Classic Mini](../classicmini/docs/banner.png)

ClassicStation is [EmulationStation](https://emulationstation.org) for the NES Classic and the SNES Classic. It takes
the place of Clover-UI, the console's own home menu, on a console modded with hakchi. It shows the games you sync with
hakchi grouped by system, with covers, descriptions, favorites and themes, and starts them the way the stock menu does.
It comes as a hakchi mod, `emulationstation_classicmini`.

> [!CAUTION]
> **DO NOT INSTALL ON THE NAND.** Only the firmware swap method has been tested. The mod is large, so only its core
> (the program and its scripts) is kept on the NAND; themes and the rest go to the SD card or USB stick.

This is an alpha and it is not affiliated with Nintendo. Download the latest build from the
[releases page](https://github.com/DefKorns/ClassicStation/releases) and read its known issues before you install it.

## What you get

- A systems carousel, and for each system a grid of covers with the title, game code, genre and description.
- Favorites, last played and all games collections, sorting, and a kid mode that only shows the games you pick.
- An Original games list, made automatically, with the games the console shipped with.
- Suspend points with a screenshot and play time: four slots per game that you can save, resume, lock or delete. They
  are shared with Clover-UI, and so are the save files.
- Background music: put your own `.wav` or `.mp3` files in `/media/hakchi/music_menu`, or keep the console's own menu music.
- Two themes: `simple`, and `classicstation`, which looks like the console's own menu.
- Video modes: 4:3, pixel perfect or CRT filter, and bezels.
- Filters, each with a preview: Nearest, Linear, Horizontal linear, Sharp, Composite (a chroma blur) and Phosphor (an RGB
  mask on top of it), plus five scanline steps. On the NES the CRT filter has its own options.
- A scraper for covers and descriptions. It keeps what it finds on the SD card and never touches hakchi's game list.
- The Clovercon pads and USB pads work in the menus.

If ClassicStation crashes twice in a row right after starting, the next boot goes to Clover-UI by itself. You can also
hold **Select** while the console boots to get Clover-UI.

Not supported: video playback, and anything that needs more than the console's 256 MB of memory.

## Requirements

- A NES Classic or SNES Classic (or their Famicom and Super Famicom versions) running hakchi.
- A RetroArch mod for the games that run on RetroArch: **RetroArch (S)NESCE** v1.22.2-beta.13 or newer, or the
  RetroArch 1.9.10 from KMFDManic.
- **An SD card or USB stick.** The install stops without one.

I have only tested it with the firmware swap method, on a NES (USA) and a SNES (EUR). The code handles the other
regions and the Famicom and Super Famicom firmware, but I have not run it there.

## Install

1. Put the SD card or USB stick in the console, with your games synced by hakchi.
2. Download `emulationstation_classicmini.hmod` from the releases page. It is not in my hakchi mod repository yet, so
   for now it has to be installed by hand. Put it in hakchi's `user_mods` folder, pick **ClassicStation** under Frontend and
   sync. The install mounts the card itself and puts the themes in `/media/data/emulationstation/themes`.
3. The first boot builds the library, which takes a while on large collections.

If hakchi says the install is done but ClassicStation never starts, read hakchi's log. Without a card the install stops
and hakchi still marks the mod as installed. Remove it with "Remove modules" and try again.

To update, install the new mod over the old one. Your settings, favorites and the theme files you changed are kept.

To uninstall, remove the mod in hakchi. That deletes the program, its settings and the library cache, and keeps
`favorites.txt`. The theme files you did not change are removed from the card; the ones you edited stay.

## Using it

| Where | Buttons |
| --- | --- |
| Systems | left/right choose, A open, X random game, Start menu |
| Games | left/right browse, A play, B back to systems, X random game, Y favorite, Select options, down suspend points, up the header menu |
| Suspend points | left/right slot, Start resume, A (NES pad) or Y (SNES pad) save, hold to overwrite, down lock, Select delete, B back |

The NES pad has no X or Y, so everything they do is also in the Select menu of a game.

The header icons open the menu, which has six tabs: Display, Sound, Library, Network, Tools and System. Appearance
(theme, games view style) and Behaviour (UI mode, power saver) are entries in it.

**Covers.** Put yours in `/media/data/emulationstation/covers`. They are matched to the games by game code, alias or
name. If a copy with the same file name exists in `covers_small`, it is used instead, which keeps the grid smooth on the
console.

**Music.** The background music plays the `.wav` and `.mp3` files in `/media/hakchi/music_menu` on the SD card or USB
stick, the same folder the theme selector uses. With no files there, it plays the console's own menu music. Turn it on or
off, and set the volume, in the Sound tab.

**Kid mode.** In Behaviour, set UI MODE to Kid. Only the games you marked with ADD TO KID GAMES (in their Select menu)
show up, and most menu options are hidden. To leave it, enter the code shown when you switched (up, up, down, down,
left, right, left, right, B, A by default).

**Themes.** Extra themes go in the `themes` folder, next to `simple` and `classicstation`. The classicstation theme is
generated on the console from the artwork of the firmware that is installed, and it is generated again when that changes.
The first time it exists, ES switches to it; you can go back to `simple` in Appearance. The Cross Stitch theme is a
separate repository, `es-theme-cross-stitch`. To write your own, see [THEMES.md](../THEMES.md).

## Still to do

- Put the mod in my hakchi mod repository, so it can be installed from hakchi without downloading it.
- Test it on the other regions and on the Famicom and Super Famicom firmware.
- Translations, so the menus can change language.
- Choose the emulator for each system from the menu.
- Make the DualShock 4 work inside games, not only in the menus.

## If something goes wrong

If you end up in the stock menu (after a crash, or after booting with Select), the **Return to ClassicStation** tile
starts ClassicStation again without a reboot. It is a hakchi game package: add `CLV-Z-ESRET.clvg` from the
[`classicmini/hakchi-app`](../classicmini/hakchi-app) folder with "Add more games" in hakchi and sync. ClassicStation
does not list it as a game.

Open an issue on [GitHub](https://github.com/DefKorns/ClassicStation/issues) and attach
`/var/lib/hakchi/emulationstation/home/.emulationstation/es_log.txt`. After a crash, attach
`/var/lib/hakchi/emulationstation/crash.log` too.

## Building it

Only docker is needed:

```sh
classicmini/build.sh
cd classicmini/hmod
sh stage.sh
sh ../../vendor/hmod-build/docker-make.sh
```

The details are in [DEVNOTES.md](../DEVNOTES.md). ClassicStation is a fork of
[RetroPie's EmulationStation](https://github.com/RetroPie/EmulationStation); [UPSTREAM.md](../UPSTREAM.md) has the
branches, how upstream is merged and what was removed. During the alpha the work and the releases happen on
`classicstation`, not on `main`.

## Credits and license

EmulationStation is by Alec Lofquist and the RetroPie project. ClassicStation is under the same MIT license
([LICENSE.md](../LICENSE.md)). The fonts, the CA bundle and the other third-party pieces are listed in
[classicmini/ATTRIBUTION.md](../classicmini/ATTRIBUTION.md), and the people who helped are in
[CONTRIBUTORS.md](../CONTRIBUTORS.md) and [CREDITS.md](../CREDITS.md).
