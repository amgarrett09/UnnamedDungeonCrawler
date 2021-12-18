The purpose of this project is to build a game engine from scratch using as few
libraries as possible/practical. The purpose of that isn't for bragging rights,
but rather to learn how every part of the process works.

This project draws inspiration from Casey Muratori's "Handmade Hero," but it
doesn't use any of that code.

My aim is for this engine is to facilitate a 2D dungeon crawler with some light 
tactics elements. Beyond that, things are very much in an early exploratory 
phase.

Things that are working (in a rudimentary way):
- Software-based tile map rendering
- Player movement and collision
- Asset loading
- AI vision and pathfinding


## Project values
### Simplicity
Keep implementation simple, only use as much abstraction as is strictly
necessary.

It should also be extremely easy to get set-up with the code base and build it.

As of now, this is a unity build (meaning only one translation unit), and
compilation just requires running a simple script.

### Performance
The engine should perform reasonably well and be able to run smoothly
on systems with limited hardware and resources. Although the engine is not
(yet) aggressively optimized, implementation should be non-pessimized, meaning
it should avoid unnecessary work wherever possible. 

Where performance and simplicity conflict, slightly prefer simplicitly. Simple
code is easier to re-write to be faster later on.

Compilation should also be fast. The unity build helps with this, but also,
the engine doesn't import libraries that would substantially increase compile
times. Currently, building the whole engine is so fast that we don't even need 
incremental compiation (try it yourself if you don't believe). The aim is for 
it to stay that way.

### Portability

It should be extremely easy to write ports.

Although creating too many interface boundaries can make software needlessly
difficult to understand, separating the platform layer from the actual game
layer is something that makes a lot of sense to do, so they are as "decoupled"
as is practical. This means writing a port is as easy as re-writing a single
file of about 400 lines;

## Compiling

There is currently only a Linux version, but since we're using SDL, making
it cross-platform in the future will be pretty simple. This readme will update
when compiling on other platforms is available.

You basically just need to have SDL and clang.

On Debian
```
sudo apt install clang libsdl2-2.0 libsdl2-dev
```

One you have the dependencies, simply run

`./sdl_build.sh`

in the main project folder. The executable can then be found in the `build` 
directory.

To run the game with assets, you will need a `resources` folder in the main
project directory with assets in it.

Since the engine is still in the phase of having very basic functionality
worked out, the engine is developed with random test assets, and there's
no well-defined structure to that yet. This README will update with what assets
you need exactly when that begins to stabilize.

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
