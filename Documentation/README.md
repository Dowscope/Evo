# EVO Manual

This manual explains how to use EVO first, followed by its internal design for
programmers. Keep it current as systems and user-visible behavior are added.

## User Guide

### Starting EVO

Build and run from the repository root:

```sh
make run
```

EVO opens in a 1280 x 720 window. Press Escape or use the window close button
to exit.

### Camera controls

Use these controls to inspect the visible land:

- Hold the left mouse button and drag to orbit around the land.
- Hold the middle mouse button and drag to pan across the land.
- Use the mouse wheel to zoom in and out.
- Resizing the window automatically updates the camera viewport.

### World simulation

A golden low-poly sun continuously completes a vertical orbit around the land.
It passes beneath the world during the nightward half of its cycle. Its
direction illuminates exposed terrain faces, while its height controls daylight
intensity and transitions the sky between bright blue and deep night blue. The
same game-owned sun state can later drive temperature and vegetation growth.

### Configuration

Edit `config/config.json` before starting EVO to change startup settings:

```json
{
  "window.title": "EVO",
  "window.width": 1280,
  "window.height": 720,
  "network.address": "127.0.0.1",
  "network.port": 0,
  "world.seed": 0,
  "world.grid_size": 32
}
```

The network values are reserved for future networking. Invalid settings stop
startup with an error rather than silently selecting an unexpected value.
Set `world.seed` to `0` to create a fresh random landscape on every launch. EVO
prints the selected seed in the terminal and immediately checkpoints it as
`world.last_seed` in `Data/evo.save`. Copy that nonzero value back into
`world.seed` to recreate the landscape later. Any nonzero seed always produces
the same starting terrain. `world.grid_size` sets the number of terrain cells
along each side and must be between 2 and 256; larger values create a smoother
mesh but require more vertices and triangles.

### Saved progress

EVO stores persistent runtime data in `Data/evo.save`. It loads this file during
startup, checkpoints active game state periodically, and saves again during an
orderly shutdown. The `Data` directory is local runtime data and is not tracked
by Git.

### Startup messages

EVO reports each system's lifecycle in the terminal:

```text
[Screen System] Starting
[Screen System] Initialized
[Screen System] Shutdown
```

These messages help identify how far startup progressed if EVO cannot launch.

## Programmer Guide

### Application lifecycle

`src/main.cpp` owns the five application functions: `init()`, `gameLoop()`,
`update()`, `render()`, and `shutdown()`. It owns the top-level systems through
smart pointers, registers their relationships during initialization, and
destroys them in reverse dependency order.

### Dependency rule

Systems do not locate one another globally. A system can only communicate with
a dependency that has been explicitly registered through a narrow interface.
This keeps implementations replaceable and makes dependencies visible in
`init()`.

### System overview

- `System` provides the shared lifecycle and lifecycle logging behavior.
- `ScreenSystem` owns the GLFW window and the selected rendering backend. It is
  also an event source, event listener, and render target.
- `RenderSystem` is the interface implemented by rendering backends.
- `VulkanSystem` is the current renderer and owns all Vulkan code and objects.
- `CameraSystem` owns the 3D view and projection. It receives mouse input only
  as events and registers with `ScreenSystem` through the `Camera` interface.
- `EventSystem` polls registered event sources and dispatches queued events to
  registered listeners.
- `GameSystem` contains game simulation behavior and renders only through its
  registered `RenderTarget`. It chooses which game state is persistent through
  its registered `Persistence` interface. It owns land and other world data.
- `SaveSystem` owns the on-disk save file and atomic-style replacement writes.
- `Logger` is the process-wide logging entry point. Systems use it instead of
  writing directly to standard output.

### Input flow

```text
GLFW callback in ScreenSystem
    -> EventSystem queue
    -> registered EventListener (ScreenSystem or CameraSystem)
    -> listener handles the engine event
    -> window requests closure
    -> main game loop exits
```

### Rendering boundary

Only `ScreenSystem` owns a `RenderSystem`. The active implementation is
`VulkanSystem`. A future `DirectXSystem` should implement the same interface and
be selected by `ScreenSystem`; no changes should be required in `GameSystem`,
`EventSystem`, or the main loop.

`GameSystem` creates the dirt land as a configurable procedural grid. Four
octaves of seeded, smoothly interpolated value noise produce deterministic
large landforms and smaller surface variation. Subtle height-based color
variation helps expose the shape. Darker soil walls follow the uneven perimeter
down to a flat bottom, preserving visible thickness for future underground
layers.

`GameSystem` also advances the sun's orbit using elapsed time. The terrain and
sun are submitted together as scene data, without involving rendering systems
in world generation or simulation.
`ScreenSystem` combines that data with its registered camera frame, then passes
the complete scene to `RenderSystem`. This keeps world creation out of the
screen, camera, and Vulkan layers.

`VulkanSystem` presents the scene through a swapchain and Vulkan 1.3 dynamic
rendering. It owns the graphics pipeline, depth buffer, compiled shaders,
vertex/index buffers, command recording, synchronization, presentation, and
swapchain recreation. The camera's view-projection matrix is supplied to the
vertex shader each frame through a push constant. Per-draw model transforms let
the renderer draw the stationary land and moving sun through the same pipeline.
Sun position and intensity are included in that draw state. The fragment shader
derives face normals, applies ambient and diffuse sunlight to terrain, and keeps
the sun emissive so it remains bright at night.

Shader sources live under `shaders/`. Both CMake and Make compile them to SPIR-V
with `glslc`, so the Vulkan SDK shader compiler is required at build time.

### Naming convention

Every private member variable and private method uses an `_camelCase` name.
Public and protected APIs do not use the underscore prefix.

### Logging

Call `Logger::write(message)` for an exact message, or use `Logger::info()`,
`Logger::warning()`, and `Logger::error()` for leveled output. Logger methods are
static, synchronized, and safe to call from any system. Future terminal and file
destinations should be implemented inside `Logger`, without changing callers.

### Configuration architecture

`ConfigLoader` is core startup infrastructure, not a system. It parses
`config/config.json` into typed `ApplicationConfig` data before system
construction. `main.cpp` passes only `WindowConfig` to `ScreenSystem` and
`WorldConfig` to `GameSystem`. Continue this pattern for future network settings:
pass typed data to the system that needs it, and do not expose a global
configuration object.

### Persistence architecture

`SaveSystem` implements the narrow `Persistence` interface. `GameSystem` is
registered with that interface and therefore does not know the save path or
file format. It restores `game.update_count` as the initial continuation proof,
updates it during runtime, checkpoints every five seconds, and records it again
during shutdown. `GameSystem` also records `world.last_seed` immediately after
resolving the configured seed so a randomly created landscape can be reproduced.

The current key/value format is intentionally small. Before adding structured
world state, add a save-format version and migration strategy. Keep save-format
serialization inside `SaveSystem` and decisions about what game data matters
inside `GameSystem`.

### Documentation workflow

For each meaningful commit:

1. Update code and tests/build verification.
2. Update `PROMPT.md` when architecture or working rules change.
3. Add the change beneath today's dated heading in `CHANGELOG.md`.
4. Update this manual when behavior, systems, or developer usage changes.
