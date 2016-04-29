# SleepingBarberWin32
## Overview

The sleeping barber ipc problem, with threads, in WinAPI and GDI native drawing.

There are 3 resolutions in this version but it can scale easily from 640x480 as a base resolution and a 1.5 
coefficient.

Right click to see the menu from anywhere in the window.

Under heavy development. Doesn't work well at the moment.

## Building

The Makefile compiles with mingw-w64. No external dependencies. To build the release:

    mingw32-make.exe rls

The debug version isn't a single executable. It loads the draw.c compilation unit as a DLL at runtime. 
If the DLL has been recompiled while the program is running, then it reloads it. This is to make tedious
drawing with GDI primitives easier. To compile the draw.dll and the version with debugging flags:

    mingw32-make.exe draw_dll & mingw32-make.exe dbg

## Libs used

Winapi, GDI

### License

See LICENSE.txt

SleepingBarberWin32 (C) 2016 <georgekoskerid@outlook.com>
