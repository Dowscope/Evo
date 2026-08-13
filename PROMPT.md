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

## Configuration

- Startup settings live in `config/evo.cfg` and are loaded by the core
  `ConfigLoader` before systems are constructed.
- Configuration is startup-only, read-only application data. Pass the smallest
  relevant config structure explicitly to each system; do not let systems read
  the configuration file themselves.
- Window title and dimensions belong to `WindowConfig`. Future IP addresses and
  ports belong to `NetworkConfig`.
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
- `ScreenSystem` alone declares, owns, initializes, and accesses the rendering
  backend.
- All Vulkan types, calls, and lifetime management belong in `VulkanSystem`.
  No other system may include Vulkan headers.
- Rendering backends implement the `RenderSystem` interface. A future
  backend such as `DirectXSystem` should be selectable inside `ScreenSystem`
  without requiring changes to the game, event system, or main loop.
- `ScreenSystem` exposes narrow interfaces (`EventSource` and `RenderTarget`),
  not its GLFW or Vulkan internals.

## Events

- `EventSystem` owns keyboard and mouse event processing and dispatch.
- Event producers and listeners are explicitly registered with `EventSystem`.
- `ScreenSystem` is registered as both an event source and listener.
- Pressing Escape produces an event; `ScreenSystem` handles it by requesting
  window closure, which ends the main loop.
- Add future input without leaking GLFW details into the rest of the program.

## Game

- Game logic belongs in `GameSystem`.
- A render target is explicitly registered with `GameSystem`; currently this is
  the `ScreenSystem` through the `RenderTarget` interface.
- `GameSystem` must not know how the window or graphics backend is implemented.
- `GameSystem` receives persistence through explicit registration and owns the
  decision about which game state must be saved.

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

## Dependency direction

```text
main.cpp registers systems
    EventSystem <-> EventSource/EventListener interfaces <- ScreenSystem
    GameSystem  -> RenderTarget interface               <- ScreenSystem
    GameSystem  -> Persistence interface                <- SaveSystem
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
