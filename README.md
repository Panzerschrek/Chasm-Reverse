## Chasm-Reverse

Repository for "PanzerChasm" and related tools. 

![Build Status (master)](https://github.com/Panzerschrek/Chasm-Reverse/actions/workflows/cmake.yml/badge.svg)

### About

"PanzerChasm" is a free recreation of the game "Chash: - The Rift" by "ActionForms" using the original game data (CSM.BIN file).

https://en.wikipedia.org/wiki/Chasm:_The_Rift.

#### System requirements

* Keyboard, mouse.
* DOS/Windows for original game releases
* Platform supported by SDL2 for PanzerChasm
* Support for OpenGL 3.3 in the system/drivers (for OpenGL mode).
* At least 128 MB of memory.

### Building

Install/extract the GOG version to

`GOG_INSTALL_PATH=C:\GOG\ Games\Chasm\ The\ Rift/`

mount the bin/cue image at

`$GOG_INSTALL_PATH/Chasm\ The\ Rift\ Original/CHASMPR.CUE`

in cdemu or dosbox and install running `dossetup.exe` to `DOS_INSTALL_PATH=C:\CHASM`.

```sh
git clone --recurse-submodules --recursive http://github.com/Panzerschrek/Chasm-Reverse
cd Chasm-Reverse
cp -ar $DOS_INSTALL_PATH/CSM.BIN .
cp -ar $GOG_INSTALL_PATH/csm.bin csm.tar
cmake .
```

### Installation

Copy CSM.BIN from the original game folder and shaders/ from the Chasm-Reverse to the destination directory DESTDIR

```sh
mkdir -p $DESTDIR
cp -ar CSM.BIN $DESTDIR
cp -ar shaders/ $DESTDIR
cp -ar PanzerChasm $DESTDIR
```

### Running

Run `./PanzerChasm` in a folder containing `shaders/` and the original game data `CSM.BIN`

You can specify another file that contains game data through 
the command option `--csm` strings, for example:

`./PanzerChasm --csm CSM_RUS.BIN`

In order to execute some console command at start, use `--exec` option, for example:

 `./PanzerChasm --exec "load saves/save_00.pcs"` to start game and immediately load first saved game.

#### Available add-ons:

* Chasm - The Shadow Zone: ADDON1
* Chasm - Cursed Land    : cursed
* Chasm - Grim Borough   : borough

To run the add-on, you must additionally specify the path to it through the 
parameter command line `--addon` or `-addon`, for example:

`./PanzerChasm --addon ADDON1`

### Resources

* [AwesomeChasm](https://github.com/jopadan/awesomeChasm)
* [Shikadi Modding Wiki](https://moddingwiki.shikadi.net/wiki/Chasm:_The_Rift)
* [The Shadow Zone](https://discord.com/channels/768103789411434586/1374778669612007527)

### Authors
Copyright © 2016-2023 Artöm "Panzerschrek" Kunz, github contributors.

License: GNU GPL v3, http://www.gnu.org/licenses/gpl-3.0-standalone.html.
