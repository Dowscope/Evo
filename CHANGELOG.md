# Changelog

All notable changes to EVO are documented here. Entries are grouped by date and
use the categories from [Keep a Changelog](https://keepachangelog.com/).

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
