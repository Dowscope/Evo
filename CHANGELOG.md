# Changelog

All notable changes to EVO are documented here. Entries are grouped by date and
use the categories from [Keep a Changelog](https://keepachangelog.com/).

## 2026-08-20 - Solar Surface Temperature

### Added

- Added per-cell Celsius surface temperature and a fixed-tick surface energy
  system driven by slope-dependent solar incidence.
- Added configurable air temperature, solar irradiance, absorptivity, areal heat
  capacity, and sensible heat-transfer coefficient with explicit units.
- Added a T-key surface-temperature overlay using a blue-to-red -10 °C to 50 °C
  visual scale.
- Added an in-window stats panel with built-in bitmap text for day, average
  surface temperature, and temperature-overlay state.
- Added an ECS-owned four-layer soil temperature profile with configurable layer
  thicknesses, initial and deep-ground temperatures, conductivity, volumetric
  heat capacity, surface conductance, and deep-boundary depth.
- Added four naturally colored perimeter strata that visualize the configured
  thermal soil-layer thicknesses beneath the rolling surface.
- Added configurable surface emissivity and effective sky temperature for
  continuous thermal-infrared exchange.

### Changed

- Changed default timing to physical simulation seconds: one-minute fixed ticks,
  86400-second days, and 1440x acceleration for a 60-real-second day.
- Extended terrain mesh vertices and revisions to carry averaged ECS surface
  temperature to Vulkan without making render data authoritative.
- Routed the overlay toggle through `EventSystem` and kept it presentation-only.
- Separated parallel physical sunlight direction from the nearby visual sun
  position to prevent artificial parallax across the terrain.
- Registered T-key handling on both the game and stats windows so the overlay
  toggles regardless of which window has keyboard focus.
- Moved stats out of the native title bar and into the stats window's content
  area, leaving a fixed `EVO Stats` title.
- Added whole-world average surface temperature calculation after each fixed
  tick and displayed it to one decimal degree Celsius in the stats panel.
- Added a read-only `SurfaceTemperatureStatistics` interface between the
  temperature and screen systems.
- Linked OpenGL for the auxiliary stats window while keeping the world renderer
  on Vulkan.
- Coupled the surface, adjacent soil layers, and stable deep-ground boundary on
  every fixed simulation tick; internal layer transfers conserve energy and use
  a common pre-tick snapshot to avoid update-order artifacts.
- Changed the terrain bottom and layer interfaces to follow local surface
  elevation, preserving a coherent block profile across rolling terrain.
- Extended the surface energy balance with net Stefan-Boltzmann longwave
  radiation using absolute temperature.

## 2026-08-20 - Terrain ECS Systems

### Added

- Added focused terrain generation, analysis, and mesh systems behind narrow
  registered interfaces.
- Added derived slope in degrees, aspect in radians with explicit undefined
  state on level ground, and D8 steepest-descent drainage components.
- Added independent terrain-analysis and terrain-mesh dirty flags per chunk.
- Added land mesh revisions and conditional Vulkan terrain-buffer re-uploading.

### Changed

- Moved procedural entity/component generation and render-mesh construction out
  of `GameSystem`.
- Registered terrain analysis as the first fixed-tick `ChunkTickSystem`.
- Made terrain analysis read across chunk boundaries while writing only the
  currently processed chunk.
- Made `TerrainMeshSystem` rebuild only after ECS elevation is marked dirty.

## 2026-08-20 - Deterministic Simulation Pipeline

### Added

- Added `SunSystem` and the narrow `SunSimulation` interface for solar position
  and intensity derived from central day progress.
- Added `ChunkSimulationSystem`, `ChunkSimulation`, and `ChunkTickSystem` as the
  fixed-step ECS execution boundary.
- Added ordered local-update, boundary-collection, and boundary-application
  phases for every fixed tick.
- Added per-chunk completed-tick tracking while keeping all chunks fully active.

### Changed

- Reduced `GameSystem` to coordinating registered sun and chunk simulation
  interfaces instead of directly implementing solar behavior.
- Registered and initialized focused sun and chunk systems explicitly in
  `main.cpp`.

## 2026-08-20 - Day Stats Window

### Added

- Added a separate non-resizable stats window whose title displays the current
  one-based simulation day.
- Added configured simulation-day duration plus day number and normalized day
  progress to `TimeSystem` timing snapshots.
- Registered the narrow `Clock` interface with `ScreenSystem` for stats display.

### Changed

- Synchronized the sun orbit directly to normalized day progress so exactly one
  solar orbit occurs per displayed day.
- Limited stats-window title updates to day transitions.

## 2026-08-20 - Central Time System

### Added

- Added `TimeSystem` as the sole owner of monotonic real time, scaled simulation
  time, pause state, time scale, and deterministic fixed-step accumulation.
- Added the narrow `Clock` interface and immutable per-frame timing snapshots for
  explicitly registered consumers.
- Added validated JSON settings for fixed-step duration and initial time scale.

### Changed

- Registered `TimeSystem` with `GameSystem` and updated it before events and game
  state in the main loop.
- Moved sun movement to central simulation-day timing, checkpoint scheduling to
  real time, and game update counting to completed fixed simulation steps.
- Removed independent steady-clock timers from `GameSystem` and time accumulation
  from chunk data.

## 2026-08-20 - ECS Terrain Chunks

### Added

- Added stable ECS entity IDs and type-specific packed component storage.
- Added data-only grid-position, chunk-position, and meter-valued elevation
  components for every terrain cell.
- Added configurable chunks with simulation-level and elapsed-time scheduling
  metadata, defaulting to four 16 x 16 chunks in a 2 x 2 layout.
- Added explicit physical cell size configuration, defaulting to one meter.

### Changed

- Replaced the single terrain grid setting with `world.chunk_size`,
  `world.chunks_x`, `world.chunks_z`, and `world.cell_size_meters`.
- Made the visible terrain mesh derive its heights from ECS elevation components
  by averaging adjacent cells at mesh vertices.
- Scaled the starting camera, clipping range, and sun orbit for the larger
  default 32 x 32 meter world.

## 2026-08-20 - Seeded World Configuration

### Added

- Added typed `WorldConfig` settings for a 64-bit terrain seed and configurable
  grid size, validated between 2 and 256 cells per side.
- Added four-octave deterministic value noise so identical seeds reproduce the
  same terrain and different seeds create different landscapes.
- Added random world-seed generation when `world.seed` is `0`, with terminal
  reporting and immediate persistence as `world.last_seed` for later reuse.

### Changed

- Migrated startup configuration from `config/evo.cfg` to
  `config/config.json`, including the existing window and reserved network
  settings.
- Passed only `WorldConfig` to `GameSystem` and made terrain mesh resolution
  follow `world.grid_size`.
- Changed the example JSON configuration to request a random world by default.

## 2026-08-20 - Orbiting Sun

### Added

- Added a game-owned sun that follows an elapsed-time-based vertical orbit
  around the land.
- Added a golden low-poly sun mesh and per-draw model transforms to render the
  moving sun and stationary land through the same Vulkan pipeline.
- Added directional diffuse terrain lighting driven by the sun's position.
- Added sun-height-based daylight intensity and a smooth transition between a
  bright daytime sky and a deep-blue nighttime sky.
- Added a 32 x 32 procedural terrain surface with broad hills, crossing ridges,
  smaller undulations, and subtle elevation-based soil color variation.

### Changed

- Extended scene submission to carry environmental sun state alongside the
  land and camera.
- Kept the sun emissive while applying ambient and diffuse light to terrain.
- Replaced the flat land top with rolling terrain while retaining darker soil
  walls and an underground-ready bottom face.

## 2026-08-20 - Visible Land Rendering

### Added

- Added Vulkan surface and graphics-device selection, swapchain presentation,
  depth buffering, and resize-aware swapchain recreation.
- Added a dynamic-rendering graphics pipeline, GLSL land shaders, vertex/index
  buffers, camera push constants, command recording, and frame synchronization.
- Added shader compilation to the CMake and Make builds.

### Changed

- The existing game-owned dirt platform is now drawn against a blue background.
- Brightened the background to a natural sky-blue color.
- Gave the land visible thickness with a lighter top, darker soil sides, and a
  bottom face, preparing the world mesh for future underground layers.

## 2026-08-13 - Camera and Land Architecture

### Added

- Added `CameraSystem` with framebuffer-sized perspective projection.
- Added mouse events for orbiting, panning, and scroll-wheel zoom.
- Added game-owned dirt land geometry submitted through `RenderTarget`.
- Added scene and camera-frame data passed into the Vulkan rendering boundary.

### Changed

- Routed mouse input exclusively through `EventSystem` listeners.
- Established `_camelCase` prefixes for all private methods and variables.

### Pending

- Implement the Vulkan swapchain and graphics pipeline that will draw the land.

## 2026-08-13 - Modular Vulkan Foundation

### Added

- Created the C++23 EVO application and its 1280 x 720 GLFW window.
- Added Vulkan instance initialization through `VulkanSystem`.
- Added the `System`, `ScreenSystem`, `RenderSystem`, `EventSystem`, and
  `GameSystem` architecture.
- Added explicit event-source, event-listener, and render-target registration.
- Added Escape-key window shutdown through the event system.
- Added CMake and Make build configurations.
- Added `PROMPT.md` as the living architecture contract.
- Added the `Documentation` manual for users and programmers.
- Added a static `Logger` utility as the common logging entry point.
- Added typed startup configuration loaded from `config/evo.cfg`.
- Added configurable window title and dimensions with reserved network values.
- Added `SaveSystem` and a registered persistence interface for runtime state.
- Added periodic checkpoints, shutdown saves, and startup state restoration.

### Changed

- Renamed `GraphicsBackendSystem` to `RenderSystem`.
- Renamed `GraphicSystem` to `VulkanSystem` to identify its implementation.
- Routed system lifecycle output through the shared logger.
