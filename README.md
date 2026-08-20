# EVO

A modular C++23 Vulkan game foundation with a 1280 x 720 GLFW window.

The application is split into independently registered screen, graphics,
event, and game systems. See `PROMPT.md` for the living architecture contract.
See `Documentation/README.md` for the user and programmer manual.

Startup settings are stored in `config/config.json`. Its world seed reproduces
the terrain, while chunk dimensions and one-meter cells define the world extent.
Persistent runtime data is created locally under `Data/`.
Set `world.seed` to `0` for a new random world; EVO records the chosen value as
`world.last_seed` in `Data/evo.save` so it can be reused later.

## Current rendering status

The Vulkan renderer draws game-owned rolling dirt terrain with
underground-ready soil walls against a sky-blue background. A visible golden
sun continuously orbits the land as the first piece of the world's
environmental simulation. Its height controls directional terrain lighting and
a smooth daylight-to-night sky transition.
The default terrain contains four 16 x 16 ECS chunks arranged into a 32 x 32
meter world.
`TimeSystem` separates monotonic real time from scaled simulation time and
produces deterministic fixed simulation steps.
A separate stats window displays the current simulation day and advances in
sync with the sun's orbit.
`ChunkSimulationSystem` executes deterministic local, boundary-collection, and
boundary-application phases, while `SunSystem` owns solar behavior.
Dedicated terrain generation, analysis, and mesh systems create ECS cells,
derive physical terrain properties, and maintain the revisioned render view.
It includes a swapchain, depth buffer, graphics pipeline, compiled shaders,
vertex/index buffers, frame synchronization, and resize recreation.

## Requirements

- A C++23 compiler
- Vulkan SDK or Vulkan development packages
- GLFW 3.3 or newer
- CMake 3.25 or newer (or GNU Make and `pkg-config`)

## Build with CMake

```sh
cmake -S . -B build
cmake --build build
./build/EVO
```

## Build with Make

```sh
make
./EVO
```

Press Escape or close the window to exit.
