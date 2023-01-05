# SimpleHydrology
C++ implementation of a particle based procedural hydrology system for terrain generation. This extends simple particle based hydraulic erosion to capture streams and pools.

Rendered using my homebrew [TinyEngine](https://github.com/weigert/TinyEngine).

[Link to a blog post about this](https://nickmcd.me/2020/04/15/procedural-hydrology/)

![Banner Image](https://github.com/weigert/SimpleHydrology/blob/master/screenshots/banner.png)

## Compiling

Use the makefile to compile the program.

    make all

### Dependencies

    Erosion System:
    - gcc
    - glm
    - libnoise

    Renderer:
    - TinyEngine (and sub dependencies)

## Usage

    ./hydrology [SEED]

If no seed is specified, it will take a random one.

### Controls

    - Zoom and Rotate Camera: Scroll
    - Arrow Keys: Move Camera Position
    - WASD / C / V: Move Camera Anchor
    - Toggle Pause: P (WARNING: PAUSED BY DEFAULT!!)
    - Toggle Map View: M
    - Toggle Hydrology Map View: ESC

### Screenshots
![Example Output 1](https://github.com/weigert/SimpleHydrology/blob/master/screenshots/top4.png)

![Example Output 2](https://github.com/weigert/SimpleHydrology/blob/master/screenshots/side5.png)

![Example Output 3](https://github.com/weigert/SimpleHydrology/blob/master/screenshots/top5.png)

![Example Output 4](https://github.com/weigert/SimpleHydrology/blob/master/screenshots/side4.png)

![Example Output 5](https://github.com/weigert/SimpleHydrology/blob/master/screenshots/side3.png)

## Reading
The main file is just to wrap the OpenGL code for drawing. At the very bottom, you can see the main game loop that calls the erosion and vegetation growth functions.

The part of the code described in the blog article is contained in the file `water.h`. Read this to find the implementation of the procedural hydrology.

The trees are implemented in `vegetation.h`.

All of the code is wrapped with the world class in `world.h`. The bottom of this file contains a bunch of stuff relevant for rendering, but not the erosion system.

The rest is shaders and rendering stuff.

## Update October 2021

The rendering system has been updated to use [vertex pooling](https://nickmcd.me/2021/04/04/high-performance-voxel-engine/) to remove all remeshing cost.

Improvements have also been made to the flooding algorithm to increase the search plane at fixed, deterministic increments by iteratively looking for the lowest point on the boundary. Additionally, the region can be grown by only tracking the boundary.

For a next iteration on this system, see [SoilMachine](https://github.com/weigert/SoilMachine).

## Update January 2023

The flooding system has been removed for now, because of buggyness and slowness. A better system has been proposed [here](https://github.com/weigert/SoilMachine).

Momentum and discharge maps are now explicit and interact physically with the water particles, giving river meandering behavior.

Parameters have been properly separated out.

Libnoise dependency has been removed.

## Windows Ports
Windows ports for this program exist:

https://github.com/pawel-mazurkiewicz/SimpleHydrologyWindows

https://github.com/Roipo/SimpleHydrology


## License
MIT License.

See my website for a [more detailed copyright notice](https://nickmcd.me/copyright-notice/).
