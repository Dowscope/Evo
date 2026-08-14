# Changelog

All notable changes to EVO are documented here. Entries are grouped by date and
use the categories from [Keep a Changelog](https://keepachangelog.com/).

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
