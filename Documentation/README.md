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

EVO also opens a compact, non-resizable stats window. Its title displays the
current simulation day, beginning with `EVO Stats - Day 1`. The counter follows
simulation time, so it stops while simulation time is paused and responds to
the configured time scale.

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
  "world.chunk_size": 16,
  "world.chunks_x": 2,
  "world.chunks_z": 2,
  "world.cell_size_meters": 1.0,
  "time.fixed_step_seconds": 0.1,
  "time.scale": 1.0,
  "time.day_length_seconds": 60.0
}
```

The network values are reserved for future networking. Invalid settings stop
startup with an error rather than silently selecting an unexpected value.
Set `world.seed` to `0` to create a fresh random landscape on every launch. EVO
prints the selected seed in the terminal and immediately checkpoints it as
`world.last_seed` in `Data/evo.save`. Copy that nonzero value back into
`world.seed` to recreate the landscape later. Any nonzero seed always produces
the same starting terrain. `world.chunk_size` sets the cell width and depth of
each square chunk and must be between 2 and 64. `world.chunks_x` and
`world.chunks_z` set the initial chunk layout and must each be between 1 and 64.
`world.cell_size_meters` defines physical cell width and must be between 0.1 and
100 meters. The defaults create four 16 x 16 chunks and a 32 x 32 meter world.
`time.fixed_step_seconds` defines deterministic simulation tick duration and
must be greater than zero and at most 10 seconds. `time.scale` multiplies
simulation time without changing real-time clocks and must be between 0 and
1000. A scale of zero stops simulation time.
`time.day_length_seconds` defines one complete simulated day and sun orbit in
simulation seconds and must be positive. The default is 60 seconds.

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
  also an event source, event listener, and render target. It owns the separate
  stats window and reads day state through its registered `Clock`.
- `RenderSystem` is the interface implemented by rendering backends.
- `VulkanSystem` is the current renderer and owns all Vulkan code and objects.
- `CameraSystem` owns the 3D view and projection. It receives mouse input only
  as events and registers with `ScreenSystem` through the `Camera` interface.
- `EventSystem` polls registered event sources and dispatches queued events to
  registered listeners.
- `TimeSystem` owns monotonic real time, scaled simulation time, pause and time
  scale state, and deterministic fixed-step production. Other systems access it
  only through the registered `Clock` interface.
- `ChunkSimulationSystem` consumes fixed steps and executes registered ECS tick
  processors through local update, boundary collection, and boundary application
  phases.
- `SunSystem` derives sun position and intensity from normalized day progress and
  configured world dimensions.
- `TerrainGenerationSystem` creates chunk membership and ECS terrain components
  from the resolved world seed.
- `TerrainAnalysisSystem` derives slope, aspect, and D8 drainage on terrain-dirty
  chunks as a registered fixed-tick processor.
- `TerrainMeshSystem` rebuilds revisioned render geometry only when a chunk marks
  its terrain mesh dirty.
- `GameSystem` coordinates registered simulation interfaces and renders only
  through its registered `RenderTarget`. It chooses which game state is
  persistent through its registered `Persistence` interface and owns land and
  other current world data.
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

`TerrainGenerationSystem` creates the dirt land as a configurable procedural
grid. Four octaves of seeded, smoothly interpolated value noise produce
deterministic large landforms and smaller surface variation. It creates each
terrain entity and initializes its position, elevation, slope, aspect, and
drainage component slots.

`TerrainMeshSystem` converts ECS elevation into the visible surface. Subtle
height-based color variation helps expose the shape. Darker soil walls follow
the uneven perimeter down to a flat bottom, preserving visible thickness for
future underground layers.

`SunSystem` advances the sun from central day progress. `GameSystem` coordinates
the registered simulation interfaces and submits terrain and sun together as
scene data, without involving rendering systems in world generation or
simulation.

### ECS terrain and chunks

`GameSystem` owns an ECS `Registry`. Every terrain cell has a stable entity ID
and three initial data-only components: `GridPosition`, `ChunkPosition`, and
`Elevation`, whose value is explicitly measured in meters. Components are held
in type-specific packed arrays; entities do not contain behavior.

The world is partitioned into configurable square `Chunk` values. Each chunk
contains the terrain entity IDs for its local cells, a simulation level, and a
completed fixed-tick count. The default 2 x 2 layout contains four independently
schedulable 16 x 16 chunks. All four begin active; nearby, distant, and dormant
levels exist for future scheduling but do not yet approximate biological state.

`ChunkSimulationSystem` runs every completed fixed step in three ordered phases:

1. Every registered `ChunkTickSystem` updates each active chunk locally.
2. Each tick system collects transfers that must cross chunk boundaries.
3. Each tick system applies those transfers only after local work is complete.

This phase barrier prevents one chunk from observing partially updated neighbor
state and establishes a deterministic path to later parallel execution. No
reduced-frequency scheduling is active yet; correctness comes before simulation
level-of-detail approximations.

The terrain mesh is derived from ECS elevation components. Mesh vertices average
adjacent cell elevations, so rendering is a view of simulation data rather than
the authoritative world model. Each chunk has an independent mesh-dirty flag.
When any flag is set, `TerrainMeshSystem` rebuilds the mesh, increments its
revision, and clears the flags. `VulkanSystem` waits for outstanding GPU work and
re-uploads terrain buffers only when that revision changes. Do not store future
moisture, soil, climate, or genome state in render vertices.

### Terrain analysis

`TerrainAnalysisSystem` is the first real `ChunkTickSystem`. It runs only for a
chunk whose terrain-analysis dirty flag is set and derives:

- `Slope::degrees` from the magnitude of central elevation differences divided
  by physical cell distance. World edges use one-sided differences.
- `Aspect::radians` as the steepest-descent direction from positive world X
  toward positive world Z, normalized to `[0, 2π)`. Aspect is explicitly marked
  undefined on level terrain.
- `Drainage::downhillNeighbor` using the D8 neighbor with the greatest positive
  elevation drop per meter. It also records vertical drop in meters and whether
  no lower modeled neighbor exists.

Analysis may read elevation across chunk boundaries because elevation is stable
during this phase. It writes only components belonging to the chunk currently
being processed. Dynamic water transfer will still use the separate collection
and application phases.

### Time architecture

`main.cpp` updates `TimeSystem` before events and game state each frame.
`TimeSystem` publishes a `TimeFrame` containing real frame delta, total real
time, scaled simulation delta, total simulation time, fixed-step duration, and
the number of complete fixed steps available this frame.

Real time uses `std::chrono::steady_clock` and is never affected by pause or time
scale. Simulation time stops while paused and otherwise advances by real delta
multiplied by the time scale. Fractional fixed-step time remains accumulated for
the next frame, preventing frame rate from changing simulation tick counts.

`GameSystem` receives the narrow `Clock`, `SunSimulation`, and `ChunkSimulation`
interfaces. It delegates solar and fixed-tick behavior, while five-second save
checkpoints follow real time. No other class should call a system clock directly
or maintain an independent frame timer.

`TimeSystem` derives a one-based day number and normalized day progress from
simulation time and configured day length. `SunSystem` uses day progress for the
sun orbit, making one orbit exactly one day. `ScreenSystem` displays the day
number in its separate stats window and updates the title only when it changes.
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
`WorldConfig` to `GameSystem`, and `TimeConfig` to `TimeSystem`. World
configuration includes seed, chunk size, chunk counts, and physical cell size.
Continue this pattern for future network settings: pass typed data to the system
that needs it, and do not expose a global configuration object.

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
