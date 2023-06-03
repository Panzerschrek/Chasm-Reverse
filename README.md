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

Install/extract the GOG version and mount the bin/cue image at

`GOG\ Games\Chasm\ The\ Rift/Chasm\ The\ Rift\ Original/CHASMPR.CUE`

 in cdemu or dosbox and install running `dossetup.exe` to `DOS_INSTALL_PATH=C:\CHASM`.

```sh
git clone --recurse-submodules --recursive http://github.com/Panzerschrek/Chasm-Reverse
cd Chasm-Reverse
cp -ar $DOS_INSTALL_PATH/CSM.BIN .
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

#### Control

##### Keyboard

* WASD - movement
* SPACE - jump
* digits 1-8 - weapon selection
* "~" - open/close the console
* ESC - menu.
* TAB - auto complete commands at the console

##### Mouse

* left mouse button   - fire/(select menu)
* middle mouse button - weapon next
* right mouse button  - jump/(back/escape menu)
* mouse wheel up/down - weapon next/previous

Part of the control can be changed in the settings menu.



### Authors
Copyright © 2016-2023 Artöm "Panzerschrek" Kunz, github contributors.

License: GNU GPL v3, http://www.gnu.org/licenses/gpl-3.0-standalone.html.
