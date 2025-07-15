# Math Crunchers

This is a small game I wrote both to help my kids with their math skills, and to demonstrate
how to use the Atari/Brainstorm JRISC C compiler in a real-world setting. The majority of the
gameplay, rendering, and animation logic is handled by C code running on the GPU.

## Overview of the code

The code is laid out as follows:

* **startup.s** - The standard system init code, with some variables renamed to be accessible in C, and the addition of some code to initialize the U-235 Sound Engine. Additionally, this contains the interrupt handlers that update the object list and dispatch commands from the GPU to 68k routines (E.g., to change the song U-235 is playing, or play a sound).
* **gpugame.c** - The gameplay init code, screen rendering, main gameplay loop, and most of its supporting logic.
* **gpuasm.s** - A few routines supporting gpugame.c. Besides the text rendering routine, which I borrowed from another project, this is largely used to work around compiler bugs by hand-coding some functions that otherwise could have been written in C as well. There's nothing in here I'd consider an optimization hand-code, though I did shrink code down where possible to help all the logic fit better.
* **main.c** - 68k C code used to display intro splash screens, bootstrap the GPU code, and handle some of the 68k commands from the GPU. Some data tables are also defined here.
* **sprintf.c** - 68k C code for sprintf and printf routines.
* **utils.s** - Misc. 68k C compiler arithmetic helper functions.
* **u235sec.s** - Some pointers to allow accessing certain U-235SE variables from C code via an extra indirection.
* **sprites.s** - Sprite & background graphics data.
* **music.s** - Music and sound effects data, as well as the U235-SE sample banks.

## Building

To build the game, you'll need the following:

* My packaging of the Jaguar SDK: https://github.com/cubanismo/jaguar-sdk
* The MOD files used as music, which are more or less just placeholder tracks at the moment. See music.s for the URLs where you can freely download these files, and feel free to replace them with your own favorites by editing that file to change the file names as well. Place the files in the matchcrunch project's directory.
* U-235SE 0.24, ideally patched to avoid the bug on K-model Jaguars that causes it to hang on startup. Place this in the subdirectory u235se/ within the Math Crunchers directory.
* The Math Crunchers source code (This repository).

Then all you have to do is pull in the environment for the Jaguar SDK and run `make`, which will produce `mathcrunch.cof`. This file can then be run on a Skunkboard, GameDrive, or in BigPEmu. For example, this set of commands and instructions will download all the needed sources and tools, set them up, build mathcrunch.cof, and run it on a Skunkboard:
```sh
$ mkdir mathcrunch_build
$ cd mathcrunch_build
$ git clone --recurse-submodules https://github.com/cubanismo/jaguar-sdk.git
$ cd jaguar-sdk
```
Follow the directions in README.md from the jaguar-sdk to install its dependencies on your system
```sh
$ ./maketools.sh
$ . env.sh
$ cd ..
$ git clone https://github.com/cubanismo/mathcrunch.git
$ cd mathcrunch
$ mkdir u235se
$ cd u235se/
```
Get the latest U-235 SE from [this AtariAge forum post](https://forums.atariage.com/topic/191157-u-235-soundengine-released/?do=findComment&comment=4589438).
```sh
$ unzip u235se-2020-07-20.zip
```
Follow the directions in [this post to the AtariAge U-235 SE thread](https://forums.atariage.com/topic/191157-u-235-soundengine-released/?do=findComment&comment=5623556) to patch the hang bug in U-235 SE.

Download the MOD files and place them in this directory (See music.s for links).
```sh
$ make
$ jcp mathcrunch.cof
```

## Details on building C code for the GPU or DSP

When using my packaging of the Jaguar SDK to build this code, you can rely on some built in rules to help compile C code to run on the GPU or DSP. To define your GPU object files corresponding to the C files you want to build into GPU code, set the variable `CGPUOBJS`. For example, if your GPU code is in the file `gpugame.c`, this:
```make
CGPUOBJS=gpugame.o
```
will cause it to be compiled to JRISC code for the GPU using the Atari/Brainstorm JRISC gcc compiler. Similarly, you can set `CDSPOBJS` to define DSP C object files. You can include multiple files, separated by spaces. However, keep in mind that the compiler places the code for each file at the beginning of GPU RAM by default, so you'll need to define custom CFLAGS (See below) to manually offset them at other locations, or define one GPU "overlay" in each file and switch between them (By e.g., stopping the GPU and blitting a new overlay into GPU RAM, or using some overlay management code in a reserved portion of GPU RAM) depending on what part of your game logic is needed for a given situation.

To set additional C flags that will be used for all JRISC C file compilation, add them to the variable `CFLAGS_JRISC`. Be careful not to replace the contents of this variable entirely, as the SDK relies on some default values assigned in it as well, as recommended by the Atari/Brainstorm documentation (available in the SDK as well). For example, this project disables linking in built-in GCC functions, and overrides the default stack location at the end of GPU RAM, instead placing it just below the main memory program starting point (to preserve more GPU RAM for code) with the following line:
```make
CFLAGS_JRISC += -fno-builtin -mstk=4000
```
See the JRISC C compiler documentation in the Jaguar SDK for details on the available options, as various useful options were added specifically for the JRISC target. You can also set rules specifically for the DSP using `CFLAGS_DSP`. There is currently no equivalent `CFLAGS_GPU` variable, though there probably should be for things like the above stack override. If that's flying right over your head, just stick to one C file for all your GPU code to start, but keep in mind you'll still be limited to about 4K of code total that way.

Note that in order to include your JRISC C objects in the usual `make clean` rules and whatnot, it is best to include them in the `OBJS` variable used by some default rules included in `jagrules.mk` in the SDK:
```make
OBJS = ... $(CGPUOBJS)
```

Additionally, note the build rules for GPU/DSP C files generate intermeediate assembly files named `g_[your_base_file_name].s`. You can safely ignore these, and you probably want to add a rule such as `g_*.s` to your .gitignore file to avoid checking them in. They will be deleted by the default `clean` rule. Feel free to peruse these files to better understand the code the compilers are generating. It's a great way to build an understanding of JRISC, and also the only way to figure out what's going on when you think you may be encountering a compiler bug, of which there are several (See comments in gpugame.c).

## Enabling debug output

If you're running on a Skunkboard or GameDrive, the program can be built to print misc. debug logging information via the JCP console (Skunkboard) or USB serial port (GameDrive). To enable this code, edit the Makefile and set one (Not both!) of these variables to `1`. In this example, Skunkboard debug printouts have been enabled:
```make
SKUNKLIB := 1
GDLIB := 0
```
To view Skunkboard printouts, you'll have to launch the program using JCP on a host computer with the console enabled, e.g.:
```sh
$ jcp -c mathcrunch.cof
```

To view the GameDrive printouts, you'll need to connect a USB-A or USB-C to micro-USB cable from the GameDrive's micro-USB port to your computer's USB-A or USB-C port and open a serial terminal on the associated serial port (E.g., HyperTerm in Windows, or minicom in Linux). You'll have to see the documentation for your particular serial terminal program for details on how to configure that. You can also view GameDrive output running on the latest version of BigPEmu with the "Force JGD Emulation" option enabled in the "System-\>Settings" menu and then running with the `-conout` command line parameter, e.g:
```sh
$ bigpemu mathcrunch.cof -conout
```

## Game Play

The game is currently pretty simple, as it's in the alpha/proof-of-concept state. Wait for the splash screens, then when the first level loads, press `B` to eat numbers that are multiples of 2. In the next level, you'll need to find multiples of 3, and so on. The current multiple is always displayed at the top of the screen. If you die or beat a level, press `C` to start over or continue to the next level.

## Acknowledgements

Thanks to those who built the tools used here, those who kept them around long enough for myself and others to be able to use them, and the authors of the MOD files referenced in music.s.
