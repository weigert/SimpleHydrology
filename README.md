# ProcRiver
C++ implementation of a particle based hydrology system for terrain generation. This extends simple particle based hydraulic erosion to capture streams and pools.

Rendered using my homebrew (TinyEngine)[https://github.com/weigert/TinyEngine].

(Link to a blog pos about this)[https://weigert.vsos.ethz.ch/2020/04/15/procedural-hydrology/]

## Compiling

Use the makefile to compile the program.

    make all
    
### Dependencies

    Erosion System:
    - gcc
    - glm
    - libnoise
    
    Renderer (TinyEngine):
    - SDL2 (Core, Image, TTF, Mixer)
    - OpenGL3
    - GLEW
    - Boost
    - ImGUI (included already as header files)

## Usage

    ./hydrology [SEED]

If no seed is specified, it will take a random one.

### Controls

    - Zoom and Rotate Camera: Scroll
    - Toggle Pause: P (WARNING: PAUSED BY DEFAULT!!)
    - Change Camera Vertical Angle: UP / DOWN
    - Toggle Hydrology Map View: ESC
    - Move the Camera Anchor: WASD / SPACE / C

### Screenshots



## Reading
The main file is just to wrap the OpenGL code for drawing. At the very bottom, you can see the main game loop that calls the erosion and vegetation growth functions.

The part of the code described in the blog article is contained in the file `water.h`. Read this to find the implementation of the procedural hydrology.

The trees are implemented in `vegetation.h`.

All of the code is wrapped with the world class in `world.h`. The bottom of this file contains a bunch of stuff relevant for rendering, but not the erosion system.

The rest is shaders and rendering stuff.

## License
Mozilla Public License
