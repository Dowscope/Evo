# EVO

A modular C++23 Vulkan game foundation with a 1280 x 720 GLFW window.

The application is split into independently registered screen, graphics,
event, and game systems. See `PROMPT.md` for the living architecture contract.
See `Documentation/README.md` for the user and programmer manual.

Startup settings are stored in `config/evo.cfg`. Persistent runtime data is
created locally under `Data/`.

## Current rendering status

The camera, event routing, and game-owned dirt land scene are implemented. The
Vulkan draw pipeline is the next milestone, so the platform is not visible yet.

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
