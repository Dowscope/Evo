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

### Configuration

Edit `config/evo.cfg` before starting EVO to change startup settings:

```text
window.title=EVO
window.width=1280
window.height=720
network.address=127.0.0.1
network.port=0
```

The network values are reserved for future networking. Invalid settings stop
startup with an error rather than silently selecting an unexpected value.

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
- `EventSystem` polls registered event sources and dispatches queued events to
  registered listeners.
- `GameSystem` contains game simulation behavior and renders only through its
  registered `RenderTarget`. It chooses which game state is persistent through
  its registered `Persistence` interface.
- `SaveSystem` owns the on-disk save file and atomic-style replacement writes.
- `Logger` is the process-wide logging entry point. Systems use it instead of
  writing directly to standard output.

### Input flow

```text
GLFW callback in ScreenSystem
    -> EventSystem queue
    -> registered EventListener
    -> ScreenSystem handles Escape
    -> window requests closure
    -> main game loop exits
```

### Rendering boundary

Only `ScreenSystem` owns a `RenderSystem`. The active implementation is
`VulkanSystem`. A future `DirectXSystem` should implement the same interface and
be selected by `ScreenSystem`; no changes should be required in `GameSystem`,
`EventSystem`, or the main loop.

### Logging

Call `Logger::write(message)` for an exact message, or use `Logger::info()`,
`Logger::warning()`, and `Logger::error()` for leveled output. Logger methods are
static, synchronized, and safe to call from any system. Future terminal and file
destinations should be implemented inside `Logger`, without changing callers.

### Configuration architecture

`ConfigLoader` is core startup infrastructure, not a system. It parses
`config/evo.cfg` into typed `ApplicationConfig` data before system construction.
`main.cpp` then passes only `WindowConfig` to `ScreenSystem`. Continue this
pattern for future network settings: pass typed data to the system that needs
it, and do not expose a global configuration object.

### Persistence architecture

`SaveSystem` implements the narrow `Persistence` interface. `GameSystem` is
registered with that interface and therefore does not know the save path or
file format. It restores `game.update_count` as the initial continuation proof,
updates it during runtime, checkpoints every five seconds, and records it again
during shutdown.

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
