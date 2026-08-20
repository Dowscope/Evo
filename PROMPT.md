# EVO Architecture Contract

Read this file before changing EVO. Keep it updated whenever an architectural
decision changes.

For every meaningful commit, update this contract when architecture or working
rules change, add an entry under today's date in `CHANGELOG.md`, and update the
user/programmer manual in `Documentation` when relevant.

## Project

- EVO is a C++23 game project.
- The application opens a 1280 x 720 (16:9) window titled `EVO`.
- Favor small, replaceable systems connected through explicit registration.
- A system must not discover or access another system through globals or hidden
  singletons. It may only use dependencies explicitly registered with it.
- Prefix every private member variable and private method with `_` using
  `_camelCase`. Public and protected names do not receive the prefix.

## Configuration

- Startup settings live in `config/config.json` and are loaded by the core
  `ConfigLoader` before systems are constructed.
- Configuration is startup-only, read-only application data. Pass the smallest
  relevant config structure explicitly to each system; do not let systems read
  the configuration file themselves.
- Window title and dimensions belong to `WindowConfig`. Future IP addresses and
  ports belong to `NetworkConfig`.
- Terrain seed, chunk size, chunk counts, and physical cell size belong to
  `WorldConfig`, which is passed only to `GameSystem`. Chunk size is validated
  from 2 through 64 cells, chunk counts from 1 through 64 per axis, and cell size
  from 0.1 through 100 meters. Identical settings must reproduce identical
  starting terrain.
- Fixed-step duration and initial time scale belong to `TimeConfig`, which is
  passed only to `TimeSystem`. Fixed steps must be positive and no longer than
  3600 seconds; time scale is validated from 0 through 100000. Positive
  configured day length defines one solar orbit in simulation seconds. Defaults
  use physical seconds: a 60-second tick and an 86400-second day, accelerated by
  a time scale of 1440.
- Air temperature, solar irradiance, surface absorptivity, areal heat capacity,
  surface heat-transfer coefficient and emissivity belong to `ClimateConfig`.
  Daily air-temperature extrema and hours plus clear-sky temperature offset
  belong to `AtmosphereConfig`. Soil initial and deep temperatures,
  conductivity, volumetric heat capacity, surface conductance, layer
  thicknesses, and deep-boundary depth belong to `SoilThermalConfig`. Preserve
  their documented units and validation when extending the energy model.
- A configured world seed of `0` requests a fresh random nonzero seed. Resolve it
  in `GameSystem` before terrain generation, log it, and immediately checkpoint
  it as `world.last_seed`. Do not rewrite the read-only startup configuration.
- Configuration and persistent saves are separate concerns: configuration
  describes how EVO starts, while saved data describes runtime progress.

## Main lifecycle

`src/main.cpp` is the program entry point and owns the main game loop. Its five
application lifecycle functions are:

1. `init()` - declare/initialize system pointers and explicitly register their
   relationships.
2. `gameLoop()` - run until the screen requests shutdown.
3. `update()` - update events and game state.
4. `render()` - render the current frame.
5. `shutdown()` - destroy systems in reverse dependency order.

Top-level systems are owned through pointers. Do not put window, input, game,
or graphics implementation details directly in `main.cpp`.

`update()` advances `TimeSystem` first, then events, then game state. Preserve
this order so every consumer observes the current frame's timing snapshot.

## Time

- `TimeSystem` is the sole owner of monotonic real time, simulation time, pause
  state, time scale, fixed-step accumulation, and fixed-step production.
- Systems that need time receive the narrow `Clock` interface through explicit
  registration. Do not call system clocks or maintain independent frame timers
  outside `TimeSystem`.
- `TimeFrame` exposes real delta/elapsed time, scaled simulation delta/elapsed
  time, fixed-step duration, completed fixed-step count, one-based day number,
  and normalized day progress.
- Pause and time scale affect simulation time only. Real time continues for
  operational behavior such as persistence checkpoints.
- Retain fractional fixed-step time between frames. Simulation systems use fixed
  steps for deterministic state changes and may use simulation delta or total
  simulation time for continuous presentation state.

## System lifecycle

- Every system inherits from `System`.
- `System` receives its display name in its constructor.
- Construction logs `[<name> System] Starting`.
- Successful initialization calls `System::init()` and logs
  `[<name> System] Initialized`.
- Destruction logs `[<name> System] Shutdown`.
- Derived systems release their own resources before the base destructor runs.
- All systems use the static `Logger`; do not write directly to standard output
  or standard error from a system.

## Logging

- `Logger` is a static utility available to every system without registration.
- Use `Logger::write()` when output must retain an exact format. Use the leveled
  methods for general information, warnings, and errors.
- Keep output destinations encapsulated in `Logger`. File logging, timestamps,
  filtering, and additional sinks will be added there without changing systems.
- Logging is infrastructure, not a system dependency, and must not own or access
  game, screen, event, or rendering state.

## Screen and graphics

- `ScreenSystem` is the only top-level system responsible for window creation
  and anything presented to the screen.
- `ScreenSystem` owns both the main game window and the compact stats window.
  The stats window renders the current day, average surface temperature,
  atmospheric air and sky temperatures, and overlay state in its content area.
  It receives timing and statistics only through explicitly registered narrow
  interfaces.
- `ScreenSystem` alone declares, owns, initializes, and accesses the rendering
  backend.
- The main scene remains Vulkan-rendered. The auxiliary stats presentation may
  use its own OpenGL context, but all of its calls and built-in text rendering
  remain confined to `ScreenSystem`.
- All Vulkan types, calls, and lifetime management belong in `VulkanSystem`.
  No other system may include Vulkan headers.
- `VulkanSystem` owns the surface, device and presentation selection, swapchain,
  depth resources, graphics pipeline, GPU buffers, command recording, and frame
  synchronization. It uses Vulkan 1.3 dynamic rendering. Its directional shadow
  pass owns the sampled depth image, light-space pipeline, descriptors, and
  filtering state.
- GLSL shader sources live in `shaders/` and are compiled to SPIR-V by both
  supported build systems. Keep shader compilation as an explicit build step.
  The 2048 x 2048 terrain shadow map uses an orthographic light camera fitted to
  land bounds, slope-aware bias, and 3 x 3 percentage-closer filtering.
- Rendering backends implement the `RenderSystem` interface. A future
  backend such as `DirectXSystem` should be selectable inside `ScreenSystem`
  without requiring changes to the game, event system, or main loop.
- `ScreenSystem` exposes narrow interfaces (`EventSource` and `RenderTarget`),
  not its GLFW or Vulkan internals.
- `CameraSystem` registers with `ScreenSystem`. The screen synchronizes the
  camera viewport to the framebuffer and uses its view for rendering.
- `ScreenSystem` and rendering backends do not create game-world geometry.

## Events

- `EventSystem` owns keyboard and mouse event processing and dispatch.
- Event producers and listeners are explicitly registered with `EventSystem`.
- `ScreenSystem` is registered as both an event source and listener.
- Pressing Escape produces an event; `ScreenSystem` handles it by requesting
  window closure, which ends the main loop.
- Add future input without leaking GLFW details into the rest of the program.
- `ScreenSystem` translates GLFW callbacks into engine events; it does not
  interpret mouse input. `EventSystem` alone dispatches those events.
- `CameraSystem` registers with `EventSystem` as a listener for mouse movement,
  mouse buttons, scrolling, and framebuffer-size events.

## Game

- Game logic belongs in `GameSystem`.
- `GameSystem` coordinates registered simulation interfaces and owns current
  world state; specialized behavior belongs in focused systems rather than
  accumulating inside the coordinator.
- Land and other current game-world data are owned by `GameSystem`, created and
  transformed through registered terrain interfaces, then submitted as scene
  data through its registered render target.
- `TerrainGenerationSystem` creates the configurable deterministic grid and ECS
  components from layered seeded value noise. `TerrainMeshSystem` derives the
  rolling surface, four configured-thickness perimeter strata, and bottom render
  geometry. Stratum interfaces follow local surface elevation, and their colors
  are presentation metadata rather than simulated composition. Vulkan receives
  completed geometry and must not generate or reshape the world.
- A render target is explicitly registered with `GameSystem`; currently this is
  the `ScreenSystem` through the `RenderTarget` interface.
- `GameSystem` must not know how the window or graphics backend is implemented.
- `GameSystem` receives persistence through explicit registration and owns the
  decision about which game state must be saved.
- The sun is game-world state coordinated by `GameSystem` and advanced through
  the registered `SunSimulation` interface implemented by `SunSystem`, then
  submitted through `Scene`. Rendering code must not own orbit simulation.
- `AtmosphereSystem` owns `AtmosphereState` and derives its smooth daily cycle
  only from the shared `TimeFrame`. `GameSystem` advances it before fixed chunk
  simulation and passes it to temperature simulation through narrow interfaces.
  Keep future humidity, pressure, wind, clouds, and precipitation in atmosphere
  state or focused atmospheric systems rather than `GameSystem`.
- `SunSystem` derives sun position and intensity from normalized day progress
  and world dimensions. The renderer consumes those values for directional
  terrain lighting and the day/night sky transition; it must not independently
  calculate world time.
- Solar phase follows conventional local time: midnight at 00:00, sunrise near
  06:00, noon at 12:00, and sunset near 18:00. Atmospheric extrema use the same
  local-time frame.
- `Sun::position` places the visible representation near the modeled world;
  `Sun::direction` represents effectively parallel physical solar rays. Lighting
  and thermal systems use direction, never distance to the display mesh.
- `SurfaceTemperatureSystem` receives current sun state through the narrow
  `SurfaceTemperatureSimulation` interface and participates in fixed chunk
  ticks after terrain analysis.

## ECS and world chunks

- Simulation state uses ECS composition. Entities are stable numeric IDs;
  components contain data only; behavior belongs in systems.
- The ECS `Registry` stores each component type in a packed array with entity
  lookup. Do not add behavior, inheritance, or rendering calls to components.
- Every terrain cell is an entity with `GridPosition`, `ChunkPosition`, and
  meter-valued `Elevation`, plus derived `Slope`, `Aspect`, and `Drainage`
  components plus Celsius-valued `SurfaceTemperature` and a four-layer
  `SoilTemperatureProfile`. Add environmental components only after their
  units, inputs, and update model are defined; do not seed arbitrary biological
  stats.
- The world is partitioned into configurable square chunks. Each chunk owns the
  IDs of its terrain cells and scheduling metadata, not duplicate cell state.
- The default world is a 2 x 2 layout of 16 x 16 chunks using one-meter cells.
  Chunk layout and cell size may change through `WorldConfig`.
- Terrain rendering is derived from ECS cell elevation. Render vertices are not
  authoritative simulation state.
- Vulkan directional shadows affect presentation only. Do not sample or copy
  the GPU shadow map back into ECS temperature. Physical terrain occlusion must
  be implemented deterministically from authoritative elevation data.
- `TerrainAnalysisSystem` derives slope in degrees, aspect in radians, and D8
  steepest-descent drainage. Flat terrain has undefined aspect; do not invent a
  direction. A null downhill neighbor means no lower cell exists inside the
  modeled domain, not necessarily that the real terrain is a closed basin.
- Terrain analysis and mesh rebuilding use separate per-chunk dirty flags.
  Elevation-changing systems must mark both. `TerrainMeshSystem` increments the
  land revision after rebuilding; Vulkan re-uploads only changed revisions.
- Surface temperature uses an explicit areal energy balance: absorbed shortwave
  solar power minus linear sensible heat exchange with configured air and net
  Stefan-Boltzmann longwave exchange with the effective sky, integrated over the
  fixed step and divided by areal heat capacity. Convert Celsius to kelvin for
  the fourth-power term. Solar incidence comes from slope/aspect and sun
  direction. Do not describe this limited model as a complete climate or
  soil-temperature model.
- Soil conduction uses four configured-thickness thermal cells. Calculate every
  internal flux from the same pre-tick temperature snapshot and transfer equal
  and opposite energy between adjacent cells. Derive each layer's areal heat
  capacity from volumetric capacity and thickness. The deepest cell exchanges
  heat with the configured stable deep-ground boundary; do not silently turn it
  into an insulated boundary.
- Pressing T toggles only the render overlay. Its fixed -10 °C to 50 °C color
  scale must not feed back into ECS simulation state.
- Both GLFW windows register the shared key callback because either may own
  keyboard focus. The stats content mirrors overlay state as input confirmation.
- `SurfaceTemperatureSystem` calculates whole-world average surface temperature
  after local chunk updates and exposes it through the read-only
  `SurfaceTemperatureStatistics` interface. `ScreenSystem` displays one decimal
  place and must not derive or mutate simulation statistics itself.
- Chunk simulation must be deterministic and chunk-local before parallelism is
  introduced. Exchange cross-boundary effects in a distinct synchronization
  phase to avoid order-dependent results.
- `ChunkSimulationSystem` consumes `TimeFrame::fixedSteps`. For each step, run
  all registered `ChunkTickSystem` local updates, then all boundary-collection
  calls, then all boundary-application calls. Do not interleave these phases.
- All chunks remain `Active` and receive every tick until reduced-frequency
  behavior has a defined approximation model and verification criteria.

## Persistence

- `SaveSystem` owns loading, checkpointing, and writing persistent runtime data.
- Other systems depend only on the narrow `Persistence` interface and must be
  explicitly registered with it.
- Save data lives under the ignored `Data/` runtime directory. Never commit
  player/runtime save files.
- Write replacements through a temporary file to reduce the risk of partial
  saves. Add schema/version migration before persistent structures become
  complex.
- Save during orderly shutdown and at periodic checkpoints so a crash or reboot
  loses as little progress as practical.
- Persist the resolved terrain seed as `world.last_seed`, including seeds created
  from a configured value of `0`, so users can reproduce a generated world.

## Dependency direction

```text
main.cpp registers systems
    EventSystem <-> EventSource/EventListener interfaces <- ScreenSystem
    EventSystem -> EventListener interface               <- CameraSystem
    GameSystem  -> RenderTarget interface               <- ScreenSystem
    GameSystem  -> Persistence interface                <- SaveSystem
    GameSystem  -> Clock interface                      <- TimeSystem
    GameSystem  -> ChunkSimulation interface            <- ChunkSimulationSystem
    GameSystem  -> SunSimulation interface              <- SunSystem
    GameSystem  -> TerrainGeneration interface          <- TerrainGenerationSystem
    GameSystem  -> TerrainMeshing interface             <- TerrainMeshSystem
    ChunkSimulationSystem -> ChunkTickSystem            <- TerrainAnalysisSystem
    ChunkSimulationSystem -> ChunkTickSystem            <- SurfaceTemperatureSystem
    GameSystem -> SurfaceTemperatureSimulation          <- SurfaceTemperatureSystem
    ScreenSystem -> SurfaceTemperatureStatistics        <- SurfaceTemperatureSystem
    ScreenSystem -> Camera interface                     <- CameraSystem
    ScreenSystem -> Clock interface                      <- TimeSystem
    ScreenSystem -> RenderSystem                        <- VulkanSystem
```

When adding a dependency, introduce the smallest useful interface and register
it explicitly. Do not expose a concrete system merely for convenience.

## Documentation

- `Documentation/README.md` is the project manual.
- Put user-facing instructions and behavior first.
- Put detailed architecture, APIs, and programmer guidance underneath.
- Add focused documents in `Documentation/` as a subject outgrows the main
  manual, and link them from the manual.
