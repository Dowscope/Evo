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

EVO also opens a compact, non-resizable window titled `EVO Stats`. Its content
area displays the current simulation day, whole-world average surface
temperature rounded to one decimal degree Celsius, and temperature-overlay
state. The day counter follows simulation time, so it stops while simulation
time is paused and responds to the configured time scale.

### Camera controls

Use these controls to inspect the visible land:

- Hold the left mouse button and drag to orbit around the land.
- Hold the middle mouse button and drag to pan across the land.
- Use the mouse wheel to zoom in and out.
- Resizing the window automatically updates the camera viewport.
- Press T to toggle the surface-temperature overlay. Blue represents colder
  surfaces and the scale progresses through cyan and yellow to red for hotter
  surfaces. The shortcut works while either EVO window has focus, and the stats
  window reports `OVERLAY: ON` or `OVERLAY: OFF` in its content area.

### World simulation

A golden low-poly sun continuously completes a vertical orbit around the land.
It passes beneath the world during the nightward half of its cycle. Its
direction illuminates exposed terrain faces, while its height controls daylight
intensity and transitions the sky between bright blue and deep night blue. The
same sun state drives cell-level surface heating.
The orbit follows conventional simulation-local time: midnight at 00:00,
sunrise near 06:00, solar noon at 12:00, and sunset near 18:00.

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
  "time.fixed_step_seconds": 60.0,
  "time.scale": 1440.0,
  "time.day_length_seconds": 86400.0,
  "climate.initial_surface_temperature_celsius": 15.0,
  "climate.solar_irradiance_w_m2": 1000.0,
  "climate.surface_absorptivity": 0.75,
  "climate.surface_heat_capacity_j_m2_k": 200000.0,
  "climate.surface_heat_transfer_w_m2_k": 10.0,
  "climate.surface_emissivity": 0.95,
  "atmosphere.minimum_air_temperature_celsius": 10.0,
  "atmosphere.maximum_air_temperature_celsius": 22.0,
  "atmosphere.minimum_temperature_hour": 6.0,
  "atmosphere.maximum_temperature_hour": 15.0,
  "atmosphere.clear_sky_temperature_offset_celsius": -20.0,
  "soil.initial_temperature_celsius": 15.0,
  "soil.deep_ground_temperature_celsius": 12.0,
  "soil.thermal_conductivity_w_m_k": 1.0,
  "soil.volumetric_heat_capacity_j_m3_k": 2000000.0,
  "soil.surface_conductance_w_m2_k": 5.0,
  "soil.layer_1_thickness_m": 0.1,
  "soil.layer_2_thickness_m": 0.2,
  "soil.layer_3_thickness_m": 0.5,
  "soil.layer_4_thickness_m": 1.2,
  "soil.deep_boundary_depth_m": 3.0
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
must be greater than zero and at most 3600 seconds. `time.scale` multiplies
simulation time without changing real-time clocks and must be between 0 and
100000. A scale of zero stops simulation time.
`time.day_length_seconds` defines one complete simulated day and sun orbit in
simulation seconds and must be positive. The default is 86400 seconds, matching
24 hours. A scale of 1440 makes that day pass in 60 real seconds while each
fixed tick represents one simulated minute.

Climate values use explicit SI-derived units. Air temperature is in degrees
Celsius, solar irradiance in watts per square meter, surface heat capacity in
joules per square meter per kelvin, and surface heat transfer in watts per
square meter per kelvin. Soil conductivity is in watts per meter per kelvin and
volumetric heat capacity is in joules per cubic meter per kelvin. Layer
thickness and deep-boundary depth are in meters; the boundary must be deeper
than the combined layer thickness. Surface conductance is in watts per square
meter per kelvin. Absorptivity is dimensionless from zero through one.
Surface emissivity is also dimensionless from zero through one. Effective sky
temperature represents incoming atmospheric infrared radiation as an equivalent
blackbody temperature; it is not the local air temperature.

Atmospheric minimum and maximum temperatures are degrees Celsius. Their hours
use simulation-local time from 0 through 24 and must place the minimum before
the maximum. The clear-sky offset is added to current air temperature to derive
effective sky temperature. Defaults produce 10 °C air at 06:00, 22 °C at
15:00, and a sky equivalent 20 °C colder than the air.

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
- `SurfaceTemperatureSystem` applies solar energy and sensible heat exchange to
  every active terrain cell as a fixed-tick ECS processor.
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
deterministic rolling hills and smaller surface variation. The modestly stronger
base amplitude exposes light and shadow without producing mountains. It creates
each terrain entity and initializes its position, elevation, slope, aspect, and
drainage component slots.

`TerrainMeshSystem` converts ECS elevation into the visible surface. Subtle
height-based color variation helps expose the shape. The uneven perimeter shows
four naturally colored soil bands whose thicknesses match the thermal depth
cells. Their interfaces follow the local terrain down to a closed bottom face,
preserving a coherent block-like profile.

`SunSystem` advances the sun from central day progress. `GameSystem` coordinates
the registered simulation interfaces and submits terrain and sun together as
scene data, without involving rendering systems in world generation or
simulation.

### ECS terrain and chunks

`GameSystem` owns an ECS `Registry`. Every terrain cell has a stable entity ID
and data-only components for position, elevation, derived terrain analysis,
surface temperature, and a four-value soil temperature profile. Elevation is
measured in meters and temperature in degrees Celsius. Components are held in
type-specific packed arrays; entities do not contain behavior.

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

### Surface temperature

Every terrain entity has `SurfaceTemperature::celsius`, initialized from the
configured initial surface temperature. `SurfaceTemperatureSystem` runs after
terrain analysis on every fixed tick. It reconstructs an upward surface normal from
slope and aspect, then calculates solar incidence as the nonnegative dot product
between that normal and the direction to the sun.

Absorbed solar power is:

```text
solar irradiance × surface absorptivity × daylight intensity × solar incidence
```

Sensible heat exchange with the configured air is:

```text
heat-transfer coefficient × (surface temperature - air temperature)
```

Net longwave radiation toward the sky is:

```text
emissivity × Stefan-Boltzmann constant ×
(surface absolute temperature⁴ - effective sky absolute temperature⁴)
```

Temperatures are converted from Celsius to kelvin before exponentiation. This
term operates continuously, including at night, and increases rapidly for hot
surfaces. A negative result correctly represents net infrared heating if the
configured effective sky is warmer than the surface.

Conductive heat flow between adjacent layers is:

```text
thermal conductance × (upper temperature - lower temperature)
```

The surface uses its configured contact conductance. Between soil layers,
conductance is thermal conductivity divided by the distance between layer
centres. Every internal flux is subtracted from the warmer-side energy balance
and added to the cooler-side balance, so internal conduction conserves energy.
Each layer's areal heat capacity is its thickness multiplied by volumetric heat
capacity. All fluxes use the temperatures from the start of the tick, avoiding
update-order artifacts.

The default profile represents successive bands 0.1, 0.2, 0.5, and 1.2 metres
thick. Its deepest band conducts toward a fixed 12 °C boundary at 3 metres. This
provides a stable reservoir for the daily-scale simulation: shallow soil stores
daytime heat, returns some after sunset, and gradually passes some deeper.
`TerrainMeshSystem` uses those same thicknesses for the exposed perimeter
strata, keeping visible underground geometry aligned with the thermal profile.
The colors communicate depth only and are not authoritative soil-composition
data.

Net power multiplied by fixed-step seconds gives energy per square meter. That
energy divided by surface heat capacity gives the temperature change in kelvin,
which has the same interval size as degrees Celsius. Distance to the sun is not
used because its variation across the modeled 32-meter world is physically
negligible; `SunSystem` supplies a parallel directional-light vector separately
from the nearby visual sun position. Surface orientation dominates spatial solar
differences.

This is still a deliberately limited energy balance. Its four bands are thermal
depth cells, not geological horizons with distinct composition, and the fixed
deep boundary is an explicit approximation. It does not yet include terrain
shadow occlusion in the thermal calculation, soil moisture and latent heat,
cloud-dependent sky temperature, or wind-dependent convection. Those effects
must be added explicitly rather than hidden in arbitrary offsets.

Directional shadows are currently presentation-only. The GPU shadow map does
not feed ECS temperature because render data is not authoritative simulation
state. Thermal terrain shading will require a deterministic CPU-side horizon or
ray-occlusion calculation using ECS elevation.

Temperature is averaged from adjacent cells into terrain mesh vertices. Pressing
T toggles a shader overlay with a fixed visual range from -10 °C (blue) through
cyan and yellow to 50 °C (red). The overlay changes presentation only; it never
changes simulation state.

After every completed fixed tick, `SurfaceTemperatureSystem` sums the current
temperature of every cell across every chunk and divides by the total cell
count. `ScreenSystem` reads that value through the narrow
`SurfaceTemperatureStatistics` interface. `ScreenSystem` renders the value in
the stats window each frame; display rounding does not alter ECS values.

### Atmosphere

`AtmosphereSystem` owns the current `AtmosphereState`. It uses normalized day
progress from `TimeSystem` and smooth half-cosine transitions: air warms from
the configured morning minimum to the afternoon maximum, then cools through
midnight toward the next minimum. This avoids discontinuities at extrema and at
the day boundary.

`GameSystem` advances the atmosphere after the sun and before chunk simulation.
It passes the resulting state through `SurfaceTemperatureSimulation`, so every
fixed surface-energy tick uses current air and effective sky temperatures.
`ScreenSystem` reads the same state through the read-only
`AtmosphereStatistics` interface and displays both values inside the stats
window.

The current atmosphere is spatially uniform and deterministic. Its dedicated
state and system boundary are intended to accept humidity, pressure, wind,
cloud water, and precipitation later; none of those are implied by the present
temperature cycle.

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
number inside its separate stats window.
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
the sun emissive so it remains bright at night. Before the camera pass, Vulkan
renders terrain depth from the directional sun into a 2048 x 2048 orthographic
shadow map fitted to current land bounds. Slope-aware bias and 3 x 3
percentage-closer filtering soften artifacts and edges. Occlusion removes direct
sunlight but retains ambient sky illumination.

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
