The purpose of this project is to build a game engine from scratch using as few
libraries as possible/practical. The purpose of that isn't for bragging rights,
but rather to learn how every part of the process works.

This project draws inspiration from Casey Muratori's "Handmade Hero," but it
doesn't use any of that code.

My aim is for this engine is to facilitate a 2D dungeon crawler with some light 
tactics elements. Beyond that, things are very much in an early exploratory 
phase.

Things that are working:
- Software-based tile map rendering
- Rudimentary player movement and collision
- Very basic asset loading
- Sound playback (though at the current stage it's fragile and bad -- only used for debug)

Before I make more progress, I want to write a tile map editor to be able to
make maps easily, which would in turn make testing new additions to the engine
a lot less painful. I'm working on the tile map editor currently.

## Project values
### Simplicity
Keep implementation simple, only use as much abstraction as is strictly
necessary.

The build process should also be easy. Right now, compilation is so dead-simple
and fast that this project does not require any sort of build system, not even
a makefile, and I intend to keep it that way.

### Performance
Obviously, we want the thing to perform reasonably well. My goal is to keep
the game running at 60 fps in software rendering mode on my crappy laptop.
Perhaps this isn't realistic, but I'd rather aim for this and fail than get
lazy and write slow software.

However, where performance and simplicity conflict, slightly prefer simplicity.

### Portability

Should be extremely easy to write ports. The architecture I'm using essentially
only requires one to re-write one file (about 400 lines of code) to make a port.

Although creating too many interface boundaries can make software needlessly
difficult to understand, separating the platform layer from the actual game
layer is something that makes a lot of sense to do, so I try to keep them
as "decoupled" as possible to make sure writing ports stays easy.

## Compiling

Currently there's only a Linux version of this engine/game. 

To compile on Linux, just make sure you have ALSA and X11. ALSA is in pretty 
much any Linux kernel version since 2004. X11 is included in virtually every 
Linux distribution. 

You may need to install some development packages for X11 and ALSA though.

On Debian
```
	sudo apt install libx11-dev
	sudo apt install libxt-dev
	sudo apt install libasound2-dev
```

One you have the dependencie, simply run

`./linux_build.sh`

in the main project folder.


To run the game with assets you will also needs assets stored in a directory
in the main project folder called `resources`.

## License

Copyright (C) 2021 Alex Garrett

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
