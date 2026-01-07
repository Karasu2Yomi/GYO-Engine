
``` bash
ProjectRoot/
    ├─ CMakeLists.txt
    ├─ README.md
    ├─ docs/
    │  ├─ architecture.md          # 分層/依存ルール/更新順序など面接用に強い
    │  └─ assets_pipeline.md
    │
    ├─ third_party/                   # 3rd party（submoduleでもOK）
    │  ├─ imgui/
    │  ├─ nlohmann_json/
    │  └─ ...
    │
    ├─ engine/                     # ★通用エンジン本体（ゲームに依存しない）
    │  ├─ include/Engine/
    │  │  ├─ EngineCore.hpp
    │  │  ├─ Services/             # EngineServices（必要最小の参照束）
    │  │  ├─ IO/                   # IFileSystem, File APIs（抽象）
    │  │  ├─ Asset/                # AssetManager, AssetCatalog, ImporterRegistry
    │  │  ├─ Scene/                # SceneManager, SceneStack, SceneDef
    │  │  ├─ Event/                # EventBus, Event types（境界イベントのみ）
    │  │  ├─ Render/               # RenderManager, DrawCommand, RenderQueue, IRenderAPI
    │  │  ├─ Audio/                # AudioManager(Handle管理), IAudioDevice
    │  │  ├─ Animation/            # AnimationLibrary(Clip等Asset)
    │  │  ├─ Debug/                # Log, Profiler(任意), DebugDrawQueue(任意)
    │  │  └─ Math/                 # Vec2/Mat, Rect 等
    │  │
    │  └─ src/
    │     ├─ EngineCore.cpp
    │     ├─ Services/
    │     ├─ IO/
    │     ├─ Asset/
    │     ├─ Scene/
    │     ├─ Event/
    │     ├─ Render/
    │     ├─ Audio/
    │     ├─ Animation/
    │     ├─ Debug/
    │     └─ Math/
    │
    ├─ platform/                   # ★外部API隔離（DX12/SDL/vulkan等）
    │  ├─ include/Platform/
    │  │  ├─ dx12/                 # Platform::Dx12RenderAPI 等（IRenderAPI実装）
    │  │  ├─ sdl/                  # 入力/ウィンドウ等
    │  │  ├─ vulkan/               # vulkan用Adapter
    │  │  └─ win32/                # OS依存（必要なら）
    │  └─ src/
    │     ├─ dx12/
    │     ├─ sdl/
    │     ├─ vulkan/
    │     └─ win32/
    │
    ├─ systems/                    # ★General Systems（汎用ロジック：毎フレーム更新）
    │  ├─ include/Systems/
    │  │  ├─ InputSystem.hpp
    │  │  ├─ UISystem.hpp
    │  │  ├─ CollisionSystem.hpp
    │  │  ├─ PhysicsSystem.hpp
    │  │  ├─ NavigationSystem.hpp
    │  │  ├─ AnimationSystem.hpp
    │  │  └─ AudioSystem.hpp
    │  └─ src/
    │     ├─ InputSystem.cpp
    │     ├─ UISystem.cpp
    │     ├─ CollisionSystem.cpp
    │     ├─ PhysicsSystem.cpp
    │     ├─ NavigationSystem.cpp
    │     ├─ AnimationSystem.cpp
    │     └─ AudioSystem.cpp
    │
    ├─ framework/                  # ★Game Framework / Domain Systems（ドメイン再利用）
    │  ├─ include/GameFramework/
    │  │  ├─ Combat/
    │  │  ├─ Status/
    │  │  ├─ AI/
    │  │  ├─ Level/                # LevelDef/LevelRuntime（DefとRuntimeはここが分かりやすい）
    │  │  └─ Components/           # Actorにぶら下げる共通部品（任意）
    │  └─ src/
    │     ├─ Combat/
    │     ├─ Status/
    │     ├─ AI/
    │     ├─ Level/
    │     └─ Components/
    │
    ├─ game/                       # ★具体プロジェクト（Atlus向け作品そのもの）
    │  ├─ include/Game/
    │  │  ├─ GameApp.hpp
    │  │  ├─ Scenes/
    │  │  ├─ Actors/
    │  │  ├─ UI/
    │  │  └─ Importers/            # ★ここが重要：TileMapImporter/LevelImporter等を登録する側
    │  └─ src/
    │     ├─ GameApp.cpp
    │     ├─ Scenes/
    │     ├─ Actors/
    │     ├─ UI/
    │     └─ Importers/
    │
    ├─ assets/                     # 実データ（実行時に読む）
    │  ├─ catalog.json             # id->type->path（AssetCatalog）
    │  ├─ textures/
    │  ├─ audio/
    │  ├─ fonts/
    │  ├─ levels/
    │  │  ├─ level_00.json
    │  │  └─ level_01.json
    │  ├─ tilemaps/                # 今はタイルでもOK（将来は別形式が増える）
    │  └─ ui/
    │
    ├─ tools/                      # ★将来GUI/エディタを追加する場所（今は空でもOK）
    │  ├─ editor/                  # ImGuiベースのEditorアプリ（別exe）
    │  └─ asset_packer/            # atlas生成など（任意）
    │
    ├─ tests/                      # 単体テスト（可能なら）
    │  ├─ engine_tests/
    │  └─ systems_tests/
    │
    └─ apps/                       # 実行ファイル群（複数ターゲットにすると説明が強い）
       ├─ runtime/                 # ゲーム実行（game + engine + platform）
       └─ editor/                  # ツール実行（engine + platform + tools）

```