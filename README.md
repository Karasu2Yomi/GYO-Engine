## プロジェクトの構造
```txt
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
    │  ├─ include/engine/
    │  │  ├─ EngineCore.hpp
    │  │  ├─ services/             # EngineServices（必要最小の参照束）
    │  │  ├─ io/                   # IFileSystem, File APIs（抽象）
    │  │  ├─ asset/                # AssetManager, AssetCatalog, ImporterRegistry
    │  │  ├─ scene/                # SceneManager, SceneStack, SceneDef
    │  │  ├─ event/                # EventBus, Event types（境界イベントのみ）
    │  │  ├─ render/               # RenderManager, DrawCommand, RenderQueue, IRenderAPI
    │  │  ├─ audio/                # AudioManager(Handle管理), IAudioDevice
    │  │  ├─ animation/            # AnimationLibrary(Clip等Asset)
    │  │  ├─ debug/                # Log, Profiler(任意), DebugDrawQueue(任意)
    │  │  └─ math/                 # Vec2/Mat, Rect 等
    │  │
    │  └─ src/
    │     ├─ EngineCore.cpp
    │     ├─ services/
    │     ├─ io/
    │     ├─ asset/
    │     ├─ scene/
    │     ├─ event/
    │     ├─ render/
    │     ├─ audio/
    │     ├─ animation/
    │     ├─ debug/
    │     └─ math/
    │
    ├─ platform/                   # ★外部API隔離（DX12/SDL/vulkan等）
    │  ├─ include/platform/
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
    │  ├─ include/systems/
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
    │  ├─ include/game_framework/
    │  │  ├─ combat/
    │  │  ├─ status/
    │  │  ├─ ai/
    │  │  ├─ stage/                # StageDef/StageRuntime（DefとRuntimeはここが分かりやすい）「元Level」
    │  │  └─ components/           # Actorにぶら下げる共通部品（任意）
    │  └─ src/
    │     ├─ combat/
    │     ├─ status/
    │     ├─ ai/
    │     ├─ stage/
    │     └─ components/
    │
    ├─ game/                       # ★具体プロジェクト
    │  ├─ include/game/
    │  │  ├─ GameApp.hpp
    │  │  ├─ scenes/
    │  │  ├─ actors/
    │  │  ├─ ui/
    │  │  └─ importers/            # ★ここが重要：TileMapImporter/StageImporter等を登録する側
    │  └─ src/
    │     ├─ GameApp.cpp
    │     ├─ scenes/
    │     ├─ actors/
    │     ├─ ui/
    │     └─ importers/
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
## 命名原則（ネーミング規則）
| 種別                  | 命名規則                       | 例                                |
| ------------------- | -------------------------- | -------------------------------- |
| **フォルダ名**           | すべて小文字 + `_` 区切り           | `engine_core`, `render_api`      |
| **C++ の namespace** | パスカルケース（大文字始まり）またはエンジン名ベース | `Engine::Render`, `Game::Scenes` |
| **C++ ファイル名**       | パスカルケース + `.hpp / .cpp`    | `SceneManager.hpp`               |
| **アセット用フォルダ**       | 小文字、種類ごとに分類                | `textures/`, `sounds/`, `data/`  |
| **JSON / CSV**      | すべて小文字                     | `scene_all.json`, `mapdata.csv`  |
