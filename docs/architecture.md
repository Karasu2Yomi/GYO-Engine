# GYO Architecture

## 1. Mission

GYO is a **Reusable C++ Game Runtime** built above SDL, SDL_image, SDL_ttf, SDL_mixer, operating-system APIs, and graphics APIs such as SDL Renderer, SDL_GPU, DX12, Vulkan, and OpenGL.

Infrastructure supplies capabilities. GYO converts those capabilities into reusable mechanisms that a game can use. A concrete game remains responsible for policy and content.

```text
Infrastructure provides capability.
GYO provides reusable game mechanisms.
Game provides policy and content.
```

GYO is intentionally not a universal editor ecosystem. It does not pre-commit to a node tree, universal ECS, visual scripting, plugin framework, full RenderGraph, PBR stack, terrain system, complete physics engine, large dependency-injection framework, or generic manager hierarchy.

## 2. Layering and dependency direction

The source dependency direction is:

```text
Concrete Game / Application composition root
                    |
                    v
          GYO public mechanisms
  Runtime / Input / Asset / Render contracts
                    |
                    v
          selected outer adapters
     SDL platform / SDL input / renderer
                    |
                    v
        SDL / OS / graphics APIs
```

An optional external controller has a separate, one-way relationship:

```text
Game / Test / Debug Console / Replay / AI / Weaver
                         |
                         v
               GYO Runtime Boundary
                         |
                         v
                  GYO mechanisms
```

Normative dependency rules:

- GYO Core must not depend on a concrete game, including Object_FPS.
- GYO Core must not depend on Weaver or know which external controller is calling it.
- GYO Core must not depend on a concrete graphics backend.
- A backend implements an engine-facing contract; the application composition root selects and connects it.
- `platform/` owns host/window/event integration, not graphics implementations.
- Asset code must not create renderer or GPU resources.
- Public input, asset, render, and runtime-facing types must not expose SDL, Vulkan, DX12, OpenGL, or native OS pointers.
- Optional games, decoders, backends, and future modules must be removable without forcing unrelated modules to change.

There is no architectural `systems/` layer. “Runs every frame” describes execution frequency, not ownership. Each capability belongs to a responsibility-based module such as `input/`, `audio/`, `animation/`, `collision/`, or `navigation/` when that responsibility is actually implemented.

## 3. GYO standards and the Object_FPS conformance slice

Object_FPS is not a library from which GYO copies an application framework. It is the current vertical slice used to reveal missing or misplaced GYO mechanisms.

The conformance rule is:

1. Keep Object_FPS gameplay rules, campaign state, and content in `apps/object_fps` and `assets/object_fps`.
2. When the game needs a reusable runtime mechanism, define the smallest caller-neutral contract in the responsible GYO module.
3. Make Object_FPS consume that GYO contract directly.
4. Do not preserve a parallel KamataEngine input, asset, rendering, or main-loop layer merely to make the old source layout compile.
5. Do not add Object_FPS names, campaign assumptions, or game policy to GYO Core.

The standard is therefore GYO's: `IRuntimeClient` defines frame participation, `InputActionMap` defines action/axis evaluation, `AssetId`/`AssetHandle` and `AssetManager` define runtime asset ownership, `RenderQueue`/`IRenderDevice` define rendering submission, and `IRuntimePort` defines the typed observation/intent seam. Object_FPS supplies only the domain payload and policy needed to use those mechanisms.

Repository ownership follows the same rule:

```text
apps/object_fps/       Object_FPS code and composition
assets/object_fps/     Object_FPS catalog and runtime content

apps/<game_id>/        another game's code
assets/<game_id>/      that game's isolated runtime content
```

There is no shared global bucket where every game's stages, textures, or data acquire accidental cross-game ownership.

## 4. Mechanism versus policy

| Infrastructure capability | GYO mechanism | Game policy/content |
|---|---|---|
| SDL key/button/pointer events | physical input snapshot, named action/axis mapping | what Move, Fire, Reload, or Pause does |
| Filesystem and image decoder | asset source, ID, handle, catalog, cache/lifetime, loader dispatch | which texture or data asset belongs to a stage/enemy/weapon |
| SDL_GPU or another graphics API | opaque render handles, resource creation, render queue execution | which world surfaces, enemies, projectiles, and overlays are submitted |
| Host clock and event pump | deterministic runtime lifecycle | state transition and gameplay update policy |

An abstraction is admitted only when current code gives it a concrete responsibility and a real caller. “Might be useful later” is not sufficient.

## 5. Current bounded milestone

This exploration milestone builds a small but connected runtime skeleton:

- Existing `base/`, `io/`, and asset identity/cache/loading mechanisms remain in place.
- `NativeFileAssetSource` supplies catalog-resolved runtime bytes from the native filesystem.
- The optional SDL_image loader decodes PNG bytes to a CPU-side RGBA `TextureAsset`; it does not create GPU resources.
- `RuntimeLoop` owns deterministic frame lifecycle order.
- `IRuntimePort<Snapshot, Command, Event>` supplies a minimal typed Query/Command/Event seam.
- `SdlPlatform` owns SDL process/window/event lifecycle.
- GYO input owns physical-frame and action-map semantics; `SdlInput` is only the SDL event adapter.
- GYO render owns opaque handles, primitive mesh data, `RenderQueue`, and `IRenderDevice`; concrete SDL rendering stays under `render/backend/`.
- The optional Windows/MSVC SDL_GPU backend exercises mesh, texture, sprite, and 3D submission without leaking native handles into game-facing contracts.
- `apps/object_fps` and `assets/object_fps` exercise those mechanisms as an opt-in concrete game vertical slice.
- `apps/runtime` remains a small standalone SDL clear/present composition root, while `apps/sandbox` keeps optional ImGui demonstration concerns separate.

`RuntimeLoop` calls an injected `IRuntimeClient` in this order:

```text
ProcessEvents(frame) -> Update(frame) -> Render(frame)
```

`FrameContext` carries the frame index and delta time. `RuntimeControl::Stop` terminates at the phase that requests it.

This milestone does **not** claim a complete ordinary-game engine. Scene management, audio playback, text presentation, animation, physics, navigation, generalized world/entity queries, remote control, and Weaver remain outside the implemented set.

## 6. Responsibility map

Status terms used below are `implemented`, `this milestone`, and `on demand`.

### Base (`implemented`)

```yaml
Module: Engine Base
Owns:
  - fundamental Result, Error, and Span value types
Does:
  - provide small dependency-light primitives to other GYO modules
Depends On:
  - C++ standard library
Must Not Depend On:
  - Asset, Runtime, Platform, Input, Render, Game, or Weaver
```

### IO (`implemented`)

```yaml
Module: Engine IO
Owns:
  - path, URI, stream, filesystem, mount, and whole-file IO mechanisms
Does:
  - expose engine-facing file access without asset or gameplay policy
Depends On:
  - Base
  - native filesystem implementation details behind its IO surface
Must Not Depend On:
  - asset types, render resources, scenes, concrete games, or Weaver
```

### Asset identity, catalog, cache, and dispatch (`implemented`)

```yaml
Module: Engine Asset
Owns:
  - AssetId and AssetHandle semantics
  - catalog and path resolution
  - runtime records, cache, lifetime policy, and statistics
  - loader registration and dispatch
Does:
  - locate engine-ready assets
  - request bytes through an IAssetSource
  - retain CPU-side decoded/deserialized objects
Depends On:
  - Base
  - IO-level mechanisms
  - loader contracts
Must Not Depend On:
  - renderer backends or GPU/native graphics types
  - development import tools
  - gameplay policy, Object_FPS, or Weaver
```

### Native runtime asset source (`this milestone`)

```yaml
Module: NativeFileAssetSource
Owns:
  - reading an already-resolved native file path into bytes
Does:
  - implement IAssetSource for shipped/runtime filesystem content
Depends On:
  - Asset source contract
  - native file IO
Must Not Depend On:
  - catalog selection, loader selection, cache policy, render, Game, or Weaver
```

### SDL_image texture loader (`this milestone`, optional)

```yaml
Module: GYO::AssetSdlImage
Owns:
  - SDL_image-backed decoding of supported runtime image bytes
Does:
  - produce CPU-side RGBA TextureAsset values
Depends On:
  - GYO Asset loader contract
  - SDL and SDL_image
Must Not Depend On:
  - IRenderDevice or a concrete render backend
  - GPU resource creation
  - Object_FPS policy or Weaver
```

Only PNG is enabled for the current fixture set. Broader codec support is not implied.

### Runtime lifecycle (`this milestone`)

```yaml
Module: RuntimeLoop / IRuntimeClient
Owns:
  - per-frame phase order
  - frame index and delta-time production
  - Continue/Stop lifecycle propagation
Does:
  - call ProcessEvents, Update, and Render on an injected client
Depends On:
  - small engine runtime value types
  - C++ steady clock
Must Not Depend On:
  - SDL polling, a renderer backend, AssetManager, concrete game policy, or Weaver
```

`IRuntimeClient` is a lifecycle port, not an all-services interface. It must not grow into a service locator.

### Typed runtime port (`this milestone`)

```yaml
Module: IRuntimePort<Snapshot, Command, Event>
Owns:
  - caller-neutral shape for Query, Submit, and Events
Does:
  - expose an immutable typed snapshot
  - accept a typed intent for implementation-controlled execution
  - expose typed facts emitted by the runtime
Depends On:
  - caller/runtime-supplied payload types
  - C++ span
Must Not Depend On:
  - backend pointers or native graphics/input types
  - a universal world schema or GenericEvent payload
  - caller identity, Object_FPS assumptions, or Weaver
```

The concrete runtime owns validation, queuing, and the lifecycle point at which submitted commands execute. `IRuntimePort` does not bypass game/runtime legality rules. `Query()` and `Events()` return borrowed current-frame views: callers must copy retained data before the next runtime advance, which may replace the snapshot and clear the event span.

### Input action mechanism (`this milestone`)

```yaml
Module: GYO::Input
Owns:
  - backend-neutral keys/buttons/pointer state for one frame
  - stable action and axis IDs
  - bindings and action-frame evaluation
Does:
  - translate physical state into named actions and axes
Depends On:
  - C++ standard library
Must Not Depend On:
  - SDL
  - Object_FPS action consequences
  - render, assets, scenes, or Weaver
```

### SDL input adapter (`this milestone`)

```yaml
Module: GYO::InputBackendSDL
Owns:
  - translation from SDL events to a PhysicalInputFrame
  - SDL relative-pointer mode and focus-edge handling
Does:
  - feed GYO input state without defining game actions
Depends On:
  - GYO::Input
  - SDL
Must Not Depend On:
  - Object_FPS controllers or gameplay state
  - renderer policy, AssetManager, or Weaver
```

SDL events may cross between concrete SDL adapters at the composition edge, but do not enter the neutral runtime port or game-policy interfaces.

### Neutral render mechanism (`this milestone`)

```yaml
Module: GYO::Render
Owns:
  - opaque generational MeshHandle and TextureHandle values
  - backend-neutral mesh/image descriptions
  - FrameDescription, camera, mesh, and sprite submissions
  - RenderQueue validation and primitive mesh generation
  - IRenderDevice resource and frame-execution contract
Does:
  - describe what a game-facing presentation layer submits
  - separate CPU assets/submission from backend resource implementation
Depends On:
  - Engine Base Result/Error values
  - C++ standard library
Must Not Depend On:
  - SDL, SDL_GPU, DX12, Vulkan, OpenGL, or native resource types
  - concrete game content or Weaver
```

This is a bounded frame queue, not a full RenderGraph, material framework, or generic `RenderManager`.

### SDL Renderer clear/present adapter (`this milestone`)

```yaml
Module: GYO::RenderBackendSDL
Owns:
  - SDL_Renderer lifecycle for the minimal runtime/sandbox path
  - clear and present
Does:
  - support the existing small SDL composition roots
Depends On:
  - concrete SDL platform window
  - SDL
Must Not Depend On:
  - Object_FPS game policy
  - generalized scene/asset ownership
  - Weaver
```

This adapter is distinct from the neutral `IRenderDevice` contract and from the SDL_GPU backend.

### SDL_GPU render backend (`this milestone`, optional Windows/MSVC)

```yaml
Module: GYO::RenderBackendSDLGPU
Owns:
  - SDL_GPU device, swapchain, pipelines, uploads, and backend resources
  - translation of opaque GYO handles and RenderQueue submissions
Does:
  - implement IRenderDevice for the current mesh/sprite/3D vertical slice
Depends On:
  - GYO::Render
  - concrete SDL platform/window integration
  - SDL_GPU and private Windows shader/backend details
Must Not Depend On:
  - Object_FPS state or asset catalog policy
  - Weaver
Must Not Expose:
  - SDL_GPU, D3D12, command-buffer, descriptor, or shader handles through GYO APIs
```

Its Windows/MSVC limitation is an implementation constraint of this optional backend, not a platform requirement of GYO Core.

### SDL platform adapter (`this milestone`)

```yaml
Module: GYO::PlatformSDL
Owns:
  - SDL process/video lifecycle needed by applications
  - SDL window
  - event polling and close-request detection
Does:
  - translate host lifecycle into a small platform-facing surface
Depends On:
  - Base Error/Result and RuntimeControl
  - SDL
Must Not Depend On:
  - render submission, AssetManager, game state, or Weaver
```

`NativeWindow()` is an explicit concrete-adapter escape hatch used only while composing SDL-based adapters. It is not part of GYO's neutral runtime or game-facing render contracts.

### Object_FPS code and content (`this milestone`, opt-in)

```yaml
Module: apps/object_fps + assets/object_fps
Owns:
  - FPS campaign, world, player, weapon, enemy, collision, and presentation policy
  - Object_FPS-specific Snapshot, Command, and Event payloads
  - Object_FPS catalog, CSV definitions, maps, and textures
Does:
  - implement IRuntimeClient and the typed IRuntimePort specialization
  - map GYO InputActionFrame values into game commands/policy
  - load assets through GYO AssetId/AssetHandle/AssetManager
  - project immutable game snapshots into GYO RenderQueue submissions
  - select concrete adapters at the application composition edge
Depends On:
  - GYO public Runtime, Input, Asset, and Render mechanisms
  - selected optional SDL adapters/backends in its composition root
Must Not Depend On:
  - KamataEngine
  - backend-native render resources in gameplay/domain code
  - Weaver
Must Not Be Depended On By:
  - GYO Core or reusable GYO modules
```

The current three maps are data/content fixtures. Their number, IDs, and order are campaign data, not GYO engine constants.

### Runtime and Sandbox applications (`this milestone`)

```yaml
Module: apps/runtime
Owns:
  - minimal standalone production composition
Does:
  - connect RuntimeLoop to SDL window and clear/present adapters
Depends On:
  - GYO runtime and selected SDL adapters
Must Not Depend On:
  - Object_FPS, Sandbox, ImGui, or Weaver
```

```yaml
Module: apps/sandbox
Owns:
  - optional demonstration and experimentation composition
Does:
  - host ImGui/JSON sample behavior without making it a runtime dependency
Depends On:
  - selected GYO/SDL mechanisms and optional demo libraries
Must Not Depend On:
  - Object_FPS or Weaver
Must Not Become:
  - an editor or a requirement for ordinary games
```

## 7. Current target/dependency model

The intended target direction is:

```text
GYO::Engine -> nlohmann_json

GYO::AssetSdlImage (optional)
  -> GYO::Engine + SDL3_image + SDL3

GYO::Input
GYO::InputBackendSDL -> GYO::Input + GYO::PlatformSDL + SDL3

GYO::Render -> GYO::Engine
GYO::RenderBackendSDL -> GYO::Engine + concrete SDL platform adapter + SDL3
GYO::RenderBackendSDLGPU (optional)
  -> GYO::Render + concrete SDL platform adapter + SDL3

Object_FPS (optional)
  -> GYO Runtime + Input + Asset + Render public mechanisms
  -> selected SDL adapters at the composition root

GYO modules -X-> Object_FPS
GYO modules -X-> Weaver
```

The `GYO_BUILD_OBJECT_FPS` option is off by default. Enabling it selects the current SDL_image PNG loader and Windows/MSVC SDL_GPU implementation needed by this vertical slice. Disabling Object_FPS removes the concrete game without changing GYO Core.

Third-party source population uses the active build tree. ImGui is selected only for Sandbox; SDL_image is selected only for the image loader/Object_FPS path; doctest is selected only when testing is enabled.

## 8. Platform and graphics backend strategy

SDL offers several capabilities, but architecture classifies adapters by responsibility:

```text
platform/sdl
  SDL process + window + host lifecycle + event pump

input/backend/sdl
  SDL input events -> GYO PhysicalInputFrame

render/backend/sdl
  minimal SDL_Renderer clear/present implementation

render/backend/sdl_gpu
  GYO IRenderDevice implementation using SDL_GPU
```

Win32 belongs under `platform/` only when a requirement cannot be met through the chosen portable platform adapter. DX12, Vulkan, OpenGL, and SDL_GPU belong under `render/backend/`.

The presence of SDL in more than one adapter does not make those adapters one module. They share infrastructure, not responsibility.

## 9. Asset identity, runtime loading, importing, and GPU resources

These are separate responsibilities:

```text
Asset identity/lifetime
  AssetId + AssetHandle + catalog + cache + lifetime

Runtime source/loading
  resolved engine-ready path -> bytes -> loader -> CPU runtime asset

Development import
  source authoring asset -> importer/tool -> engine-ready asset

Renderer resource creation
  CPU runtime asset -> selected render backend -> opaque render handle
```

`NativeFileAssetSource` owns only the first native-file read after path resolution. The SDL_image loader owns only image decoding. `AssetManager` owns identity, lookup, records, cache/lifetime, and loader dispatch. `IRenderDevice` owns GPU resource creation from an `ImageView` or `MeshView`.

Hashed public IDs use their numeric value as identity. `debugName` is diagnostic metadata only and never changes equality or hashing; invalid IDs/types use value zero. This rule also applies to input action/axis IDs so named mechanisms behave consistently across maps and frame views.

A loader must not create `SDL_Texture`, `SDL_GPUTexture`, `ID3D12Resource`, Vulkan images, or OpenGL textures. Renderer resources have separate lifetime because upload, residency, device loss, and destruction are renderer concerns.

Names describe the actual pipeline. A component that parses runtime stage/map data is a Loader, Parser, or Deserializer, not an Importer. Development import tools remain absent until a real authoring pipeline requires them.

## 10. Scene, Stage, collision, and 2D/3D rules

These are architecture rules, not claims of currently implemented modules.

A Scene represents runtime lifecycle/execution state:

```text
TitleScene -> GameScene -> PauseScene -> ResultScene
```

A Stage represents gameplay content hosted by an appropriate scene:

```text
GameScene
    |
    v
StageRuntime(stage_01)
```

Scene and Stage must not independently evolve duplicate load, update, spawn, unload, and lifecycle stacks. Stage definition and gameplay runtime normally belong to Framework/Game unless multiple concrete games prove a reusable GYO-level mechanism.

GYO may share cross-cutting mechanisms between 2D and 3D: asset identity/lifetime, runtime lifecycle, input actions, audio/text mechanisms when implemented, render resource ownership rules, and the typed runtime boundary. It does not force algorithm or data-model unification. Separate `SpriteRenderer`/`MeshRenderer`, `Camera2D`/`Camera3D`, `Transform2D`/`Transform3D`, and `Physics2D`/`Physics3D` remain valid.

Collision grows from real needs. AABB, circle, hitbox, trigger, and grid tests do not justify an empty complete-physics abstraction. Bodies, response, integration, and queries belong to Physics only when those responsibilities exist. Object_FPS collision remains game policy until reuse proves a neutral engine mechanism.

## 11. Runtime Boundary

The public boundary is caller-neutral:

```text
External Controller
  ├─ Game
  ├─ Automated Test
  ├─ Debug Console
  ├─ Replay
  ├─ AI Controller
  └─ Weaver
          |
          v
  Query / Command / Event
          |
          v
      GYO Runtime
```

Current minimum seam:

- **Query:** `IRuntimePort::Query()` returns a borrowed immutable view of the runtime's current typed snapshot.
- **Command:** `IRuntimePort::Submit()` accepts a typed intent; the runtime implementation retains validation and execution control.
- **Event:** `IRuntimePort::Events()` exposes a borrowed span of typed facts emitted by that runtime for the current advance.

The interface is neutral even though each concrete runtime supplies meaningful payload types. An Object_FPS controller sees Object_FPS state and legal intents through a GYO-owned interface; another game can use the same GYO shape with its own domain types. Borrowed query/event views are valid only until the runtime advances again, so persistent history remains the caller's responsibility. Neither caller receives `SDL_Event`, `SDL_GPUCommandBuffer`, `ID3D12Resource`, mutable `AssetRecord`, or unrestricted internal pointers.

The current seam is deliberately small. It is not a complete World/Entity/Scene query language, thread-safe remote transport, generalized subscription service, persistent event journal, command scheduler, or universal snapshot schema. Those require concrete consumers and ownership rules before they can enter GYO.

Do not create a giant `GenericEvent`, universal payload variant, complete `WorldSnapshot`, or Weaver-specific bus in anticipation. Add concrete runtime views, commands, or event delivery mechanisms only when a current Game/Test/Debug use case establishes their data and lifecycle.

## 12. Weaver relationship

The following rules are normative.

### English

> Weaver is NOT part of GYO.
>
> Weaver is an optional external high-level runtime.
>
> GYO MUST be fully functional without Weaver.
>
> Weaver MAY observe and influence GYO only through
> public runtime-facing mechanisms.
>
> GYO MUST NOT depend on Weaver.

### 繁體中文

> Weaver 不屬於 GYO。
>
> Weaver 是可選的高階因果 / 敘事 Runtime。
>
> GYO 在完全不存在 Weaver 的情況下，
> 仍必須是一個可以獨立執行普通遊戲的完整 Runtime。
>
> Weaver 未來只能透過 GYO 的公開 Runtime 邊界
> 觀察狀態、接收事件與提交意圖。
>
> GYO Core 永遠不能反向依賴 Weaver。

This milestone does not implement Weaver, a Weaver adapter, world model, causal graph, narrative graph, event-node solver, LLM integration, AI agent, or story generator. It establishes only a Weaver-compatible architecture seam and one-way dependency rule.

## 13. Growth and removal rules

- Add a module for a real new responsibility; do not add a placeholder for a roadmap label.
- Prefer adding a backend or adapter over editing unrelated Engine Core modules.
- A new module should depend on narrower, lower-level contracts and must not require a generic `EverythingManager`.
- An `EngineServices` bundle, if ever justified, is only a composition/dependency utility. It is not the Runtime Boundary and must not become a universal service locator.
- `AudioSystem` is justified only by spatial/listener/source world updates; a wrapper that only calls `AudioManager::Update()` is not a module.
- Render growth extends neutral submissions/resources and adds backend implementations; it must not expose backend-native commands or handles.
- Adding Font presentation, Render3D features, Physics3D, Navigation, or an external controller should mostly add code in its own module/backend/adapter.
- Removing Object_FPS, Physics, Render3D, tools, or an external controller must not break unrelated Engine, Asset, Input, or base Runtime behavior.
- Public install/export packaging can be added after a real external consumer stabilizes the public surface; no empty packaging facade is required now.

## 14. Deliberately deferred decisions

The following are intentionally not created or generalized in this milestone:

- Universal World/Entity/Scene snapshot or query language
- Generic runtime command scheduler, remote protocol, event subscription bus, or replay journal
- SceneManager, SceneStack, or a reusable StageRuntime
- Audio playback or spatial-audio modules
- Text presentation, general font rendering, animation, navigation, or physics modules
- General material system, RenderGraph, GPU asset cache shared across devices, or renderer-wide `EverythingManager`
- Non-Windows SDL_GPU support and dedicated DX12, Vulkan, or OpenGL backends
- Universal 2D/3D transforms, renderer algorithms, or physics data models
- Development import pipeline and asset authoring tools
- Reusable game Framework policy extracted prematurely from Object_FPS
- Editor, node tree, universal ECS, visual scripting, or plugin framework
- Weaver and all causal/narrative functionality

Deferral is deliberate. The present vertical slice does not yet establish stable responsibilities for these abstractions, and GYO must not claim functionality that only exists in the architecture vision.

## 15. Architecture checks for each change

Before accepting a new runtime module, game integration, or backend, verify:

1. **Standalone:** GYO and ordinary applications configure without Weaver, Object_FPS, or development tools unless explicitly selected.
2. **GYO conformance:** a concrete game uses GYO lifecycle, input, asset, render, and runtime-facing contracts instead of preserving a parallel engine layer.
3. **External controller:** supported observation and intent use neutral public mechanisms rather than internal/backend access.
4. **Additive growth:** the feature mainly adds a module, backend, adapter, or game-owned policy/content.
5. **Removal:** disabling the game or feature does not break unrelated modules.
6. **Ownership:** Asset, Platform, Input, Render, Runtime, and Game policy retain distinct responsibilities.
7. **Dependency direction:** GYO Core has no Game, Weaver, or concrete-backend dependency.
8. **Honest status:** documentation distinguishes implemented code from on-demand architecture and reports build/test results separately from design intent.
