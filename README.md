# GYO-Engine

GYO is a **Reusable C++ Game Runtime**. It turns infrastructure capabilities from SDL, operating-system APIs, and graphics APIs into reusable game mechanisms. A game supplies policy and content; optional external controllers may observe or influence the runtime only through neutral public seams.

GYO is not an editor ecosystem and is not intended to become a small Godot, Unity, or Unreal. The repository grows from concrete game-runtime needs, not from speculative framework completeness.

`Object_FPS` is the current conformance vertical slice. It is used to check whether GYO's runtime skeleton is practical: the game is refactored to follow GYO input actions, asset identity/loading, render submission/device contracts, lifecycle, and typed runtime boundary directly. GYO is the standard being verified; GYO never depends on Object_FPS-specific policy or data.

See [docs/architecture.md](docs/architecture.md) for ownership and dependency rules.

## Status legend

- `[implemented]`: real code that predates the current conformance milestone.
- `[this milestone]`: real, bounded code introduced or corrected while using Object_FPS to exercise the skeleton.
- `[on demand]`: an architectural direction only. The directory or type may not exist and must not be created until a concrete responsibility requires it.

The tree below deliberately retains the project's **status-labelled architecture vision**. It is not a claim that every displayed directory or feature already exists.

## Architecture vision

```text
GYO-Engine/
├─ CMakeLists.txt
├─ README.md                                      [this milestone]
├─ docs/
│  └─ architecture.md                            [this milestone]
│
├─ third_party/                                  dependency wrappers
│  ├─ sdl3/                                      [implemented]
│  ├─ sdl3_image/                                [this milestone; optional PNG decode]
│  ├─ nlohmann_json/                             [implemented; asset catalogs]
│  ├─ imgui/                                     [implemented; Sandbox only]
│  └─ doctest/                                   [implemented; tests only]
│
├─ engine/                                       reusable, backend-neutral mechanisms
│  ├─ include/engine/
│  │  ├─ base/                                   [implemented]
│  │  ├─ io/                                     [implemented]
│  │  ├─ asset/                                  [implemented + this milestone]
│  │  │  ├─ catalog/                             identity/path metadata
│  │  │  ├─ core/                                records, handles, cache, lifetime
│  │  │  ├─ loading/
│  │  │  │  ├─ IAssetSource.hpp
│  │  │  │  ├─ NativeFileAssetSource.hpp         [this milestone]
│  │  │  │  ├─ LoaderRegistry.hpp
│  │  │  │  └─ AssetPipeline.hpp
│  │  │  └─ loaders/
│  │  │     ├─ TextureAsset.hpp                  CPU-side decoded pixels
│  │  │     └─ sdl_image/                        [this milestone; optional loader]
│  │  └─ runtime/                                [this milestone]
│  │     ├─ FrameContext.hpp
│  │     ├─ IRuntimeClient.hpp
│  │     ├─ IRuntimePort.hpp                     typed Query/Command/Event seam
│  │     ├─ RuntimeControl.hpp
│  │     └─ RuntimeLoop.hpp
│  └─ src/
│     ├─ io/                                     [implemented]
│     ├─ asset/                                  [implemented + this milestone]
│     └─ runtime/RuntimeLoop.cpp                  [this milestone]
│
├─ platform/                                     OS/window/event adapters
│  ├─ sdl/                                       [this milestone]
│  └─ win32/                                     [on demand; not created]
│
├─ input/                                        input mechanisms, not a systems/ bucket
│  ├─ include/engine/input/                      [this milestone]
│  │  ├─ PhysicalInputFrame.hpp
│  │  └─ InputActionMap.hpp
│  ├─ backend/sdl/                               [this milestone]
│  │  └─ SdlInput                                SDL events -> physical input
│  └─ tests/                                     [this milestone]
│
├─ render/                                       renderer contract and implementations
│  ├─ include/render/                            [this milestone; backend-neutral]
│  │  ├─ RenderQueue.hpp
│  │  ├─ IRenderDevice.hpp
│  │  ├─ RenderHandle.hpp
│  │  └─ RenderTypes.hpp
│  ├─ backend/
│  │  ├─ sdl/                                    [this milestone; clear/present adapter]
│  │  ├─ sdl_gpu/                                [this milestone; optional Windows/MSVC]
│  │  ├─ dx12/                                   [on demand; not created]
│  │  ├─ vulkan/                                 [on demand; not created]
│  │  └─ opengl/                                 [on demand; not created]
│  └─ tests/                                     [this milestone]
│
├─ framework/                                    reusable game-domain policy [on demand]
│  ├─ stage/
│  ├─ combat/
│  └─ ai/
│
├─ apps/                                         composition roots and concrete games
│  ├─ runtime/                                   [this milestone] minimal clear/present app
│  ├─ sandbox/                                   [this milestone; optional ImGui demo]
│  └─ object_fps/                                [this milestone; opt-in conformance slice]
│     ├─ include/RetroFPS/                       Object_FPS policy/domain types
│     ├─ src/                                    adapters use GYO contracts directly
│     └─ tests/                                  headless game-policy tests
│
├─ assets/                                       game-owned runtime content
│  ├─ object_fps/                                [this milestone]
│  │  ├─ asset_catalog.json
│  │  ├─ data/                                   campaign/enemy/weapon definitions
│  │  ├─ maps/                                   current conformance fixtures
│  │  └─ textures/
│  └─ <game_id>/                                 [on demand; one isolated root per game]
│
├─ tools/                                        [on demand; not created]
│  └─ import/                                    source asset -> engine-ready asset
│
└─ tests/
   ├─ engine_tests/                              [implemented + this milestone]
   └─ backend integration tests/                 [on demand]
```

There is deliberately no `systems/` catch-all. Update frequency is an execution characteristic, not a module responsibility. Input belongs to `input/`; future audio, animation, collision, physics, and navigation mechanisms belong to their own responsibility-based modules only when implemented.

`apps/<game_id>` and `assets/<game_id>` form a pair. Object_FPS therefore uses `apps/object_fps` and `assets/object_fps`; its catalog and content do not enter a shared global game-asset bucket.

## Build the current skeleton

Requirements are CMake 3.30 or newer and a C++20 compiler. The first configure may fetch the enabled third-party dependencies into the selected build tree.

Build the default standalone runtime and tests:

```sh
cmake -S . -B build -DGYO_BUILD_RUNTIME=ON -DGYO_BUILD_SANDBOX=OFF
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Build the Object_FPS conformance slice in a separate build tree:

```sh
cmake -S . -B build-object-fps -DGYO_BUILD_OBJECT_FPS=ON -DGYO_BUILD_SANDBOX=OFF
cmake --build build-object-fps --config Debug
ctest --test-dir build-object-fps -C Debug --output-on-failure
```

The current Object_FPS presentation path selects the SDL_GPU backend and therefore requires Windows with MSVC. The game option also enables the optional SDL_image PNG decoder. This is a current backend constraint, not a dependency from GYO Core to Windows, SDL_GPU, or Object_FPS.

Build switches are responsibility-based:

| Option | Default | Effect |
|---|---:|---|
| `GYO_BUILD_RUNTIME` | `ON` | Builds the standalone SDL window/clear-present composition root. |
| `GYO_BUILD_SANDBOX` | `OFF` | Adds the optional ImGui/JSON demonstration app. |
| `GYO_BUILD_OBJECT_FPS` | `OFF` | Adds the Object_FPS GYO-conformance game, its isolated assets, and required optional adapters. |
| `GYO_BUILD_SDL_GPU_BACKEND` | `OFF` | Builds the current Windows/MSVC SDL_GPU implementation of `IRenderDevice`. |
| `GYO_BUILD_SDL_IMAGE_LOADER` | `OFF` | Builds the SDL_image-backed runtime PNG texture loader. |
| `BUILD_TESTING` | `ON` | Adds doctest targets registered with CTest. |

With all application and optional adapter/decoder options disabled, the backend-neutral Engine and its tests do not require SDL, SDL_image, ImGui, or Object_FPS. Optional decoders and backends are selected at the outer build/composition layer.

## Responsibility at a glance

```text
Infrastructure provides capability.
GYO provides reusable game mechanisms.
Game provides policy and content.
```

Current examples:

- SDL reports keys, buttons, pointer motion, and focus; `SdlInput` translates them into a GYO physical frame; `InputActionMap` produces named actions and axes; Object_FPS decides how those actions move, aim, fire, reload, pause, or navigate menus.
- `NativeFileAssetSource` reads catalog-resolved runtime files; the optional SDL_image loader decodes image bytes into a CPU-side `TextureAsset`; `AssetManager` owns identity, handles, cache/lifetime, and loader dispatch; the selected render device alone creates GPU textures.
- Object_FPS projects its immutable game snapshot into GYO `RenderQueue` submissions. `IRenderDevice` owns opaque render handles and executes the queue; backend-native SDL_GPU, D3D12, or shader details remain private to the backend.

## Object_FPS is a conformance consumer

The migration is not a source-to-source KamataEngine port. Its rule is:

```text
Object_FPS game policy/content
        |
        v
GYO lifecycle + input actions + assets + render contracts + runtime port
        |
        v
selected SDL platform / SDL_GPU / SDL_image infrastructure
```

When the old game exposed a reusable need, the neutral mechanism was added to GYO first and Object_FPS then consumed it. Object_FPS does not carry a parallel input framework, resource manager, renderer contract, or application loop, and its game-specific names and state remain outside GYO Core.

The current maps and CSV files are conformance fixtures, not hard-coded engine knowledge. Asset IDs are catalogued under the `object_fps.*` namespace and the campaign data determines which content is used.

## Runtime Boundary and Weaver

The caller-neutral boundary is:

```text
Game / Test / Debug Console / Replay / AI / Weaver
                         |
                         v
               GYO Runtime Boundary
              Query / Command / Event
                         |
                         v
                 GYO mechanisms
```

`IRuntimePort<Snapshot, Command, Event>` is the current minimum typed seam: a controller can query the current immutable snapshot, submit a legal typed intent, and inspect typed facts emitted by that runtime. `Query()` and `Events()` are borrowed views valid only until the runtime advances again; a controller copies anything it must retain. Concrete payloads still belong to the runtime/game domain that defines their meaning; the interface shape, ownership rule, and lack of backend pointers are GYO standards.

This is not a complete world/entity API, remote protocol, command scheduler, or generic event bus. Those abstractions remain deferred until a current consumer establishes their responsibility.

Weaver is not implemented here. It is an optional external high-level runtime and may eventually use the same public mechanisms as a game, test, or debug tool. GYO must build and run ordinary games without Weaver, and GYO Core must never depend on Weaver.

## Growth rule

A feature is added where its responsibility belongs. New Font, Render3D, Physics3D, Navigation, or external-controller integration should normally add a module, backend, or adapter rather than rewrite unrelated Asset, Input, Render, Runtime, or game code.

Do not pre-create editor ecosystems, node trees, universal ECS layers, visual scripting, plugin frameworks, full RenderGraphs, complete physics engines, large DI containers, generic managers, or empty interfaces. An abstraction enters the repository only when current code gives it a concrete responsibility and a real caller.

## Naming

| Kind | Rule | Example |
|---|---|---|
| Directory | lowercase with `_` when separation is needed | `render/backend/sdl_gpu` |
| C++ namespace | PascalCase or engine-rooted | `Engine::Asset` |
| C++ source/header | PascalCase | `RuntimeLoop.hpp` |
| Per-game asset root | lowercase game identifier | `assets/object_fps/` |
| Data file | lowercase | `asset_catalog.json`, `levels.csv` |
