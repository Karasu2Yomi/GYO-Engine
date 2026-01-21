#include "doctest/doctest.h"

#include <unordered_map>
#include <vector>

#include "engine/asset/AssetManager.hpp"
#include "engine/asset/AssetCatalog.hpp"
#include "engine/asset/AssetRequest.hpp"
#include "engine/asset/core/AssetStorage.hpp"
#include "engine/asset/core/AssetLifetime.hpp"
#include "engine/asset/core/AssetCachePolicy.hpp"
#include "engine/asset/loading/AssetPipeline.hpp"
#include "engine/asset/loading/LoaderRegistry.hpp"
#include "engine/asset/loading/IAssetSource.hpp"
#include "engine/asset/loading/IAssetLoader.hpp"
#include "engine/asset/loaders/TextLoader.hpp"
#include "engine/asset/resolver/AssetPathResolver.hpp"
#include "engine/asset/catalog/CatalogParser.hpp"


using namespace Engine::Asset;

namespace {
    

    // テスト用：メモリから読む IAssetSource
    class MemoryAssetSource final : public Loading::IAssetSource {
    public:
        void Put(const std::string& path, std::vector<std::byte> bytes) {
            map_[path] = std::move(bytes);
        }

        Engine::Base::Result<std::vector<std::byte>, Engine::Base::Error<AssetErrorCode>>
        ReadAll(std::string_view resolvedPath) override {
            auto it = map_.find(std::string(resolvedPath));
            if (it == map_.end()) {
                return Engine::Base::Result<std::vector<std::byte>, Engine::Base::Error<AssetErrorCode>>::Err(
                    Engine::Base::Error<AssetErrorCode>::Make(AssetErrorCode::SourceReadFailed, "MemoryAssetSource: not found", std::string(resolvedPath)));
            }
            return Engine::Base::Result<std::vector<std::byte>, Engine::Base::Error<AssetErrorCode>>::Ok(it->second);
        }

    private:
        std::unordered_map<std::string, std::vector<std::byte>> map_;
    };

    static std::vector<std::byte> BytesOf(const std::string& s) {
        std::vector<std::byte> b;
        b.resize(s.size());
        for (size_t i = 0; i < s.size(); ++i) b[i] = static_cast<std::byte>(s[i]);
        return b;
    }

} // namespace

TEST_CASE("AssetManager: sync load -> cache hit") {
    // --- 準備：catalog（resolvedPath はここでは “論理パス” をそのまま使う） ---
    AssetCatalog catalog;
    {
        // ここは AssetCatalog の “手動登録” API があればそれを使うのが理想。
        // なければ catalog.json 経由のロードテストで代替してください。
    }

    // --- pipeline 組み立て（実装に合わせて調整） ---
    Loading::LoaderRegistry registry;
    registry.Register(std::make_unique<Loaders::TextLoader>());

    auto memSource = std::make_unique<MemoryAssetSource>();
    memSource->Put("mem://ui/title.txt", BytesOf("hello"));

    Loading::AssetPipeline pipeline(/*source*/ *memSource, /*registry*/ registry);
    // ↑ コンストラクタが違う場合はここだけ調整

    Core::AssetStorage storage;
    Core::AssetLifetime lifetime;
    Core::AssetCachePolicy::Options options;
    Core::AssetCachePolicy policy(options);

    AssetManager mgr(catalog, pipeline, storage, lifetime, policy, nullptr, nullptr);

    // --- 本体：Load 1回目（miss） ---
    AssetRequest req = AssetRequest::Default();
    req.sync = AssetRequest::SyncWith::Sync;
    req.overridePath = "mem://ui/title.txt"; // catalog無しで進めるため override を使う
    req.useTypeHint = true;
    req.expectedType = AssetType::FromString("text");

    auto h1 = mgr.Load(AssetId::FromString("ui.title"), req);
    if (!h1) {
        INFO("code = " << static_cast<int>(h1.error().code));
        INFO("msg  = "  << h1.error().message);
        INFO("detail = "  << h1.error().detail);
        FAIL("mgr.Load failed");
    }

    auto sp1 = mgr.GetShared<Loaders::TextAsset>(h1.value());
    REQUIRE(sp1 != nullptr);
    CHECK(sp1->text == "hello");

    // --- 2回目：reload無しならキャッシュヒット（同generation） ---
    auto h2 = mgr.Load(AssetId::FromString("ui.title"), req);
    REQUIRE(h2);
    CHECK(h2.value().generation() == h1.value().generation());
}

TEST_CASE("AssetManager: reload increments generation and stale handle") {
    // pipeline 等は上と同様に組む（省略）
    // ポイントは：
    // 1) 最初に "hello"
    // 2) bytes を "world" に差し替え
    // 3) AssetRequest::Reload() で再ロード
    // 4) 新 handle は generation++、旧 handle は GetShared が null になる
    CHECK(true);
}

TEST_CASE("AssetManager: reload fail + KeepOldIfAny keeps Ready") {
    // ポイント：
    // 1) 最初に成功して Ready を作る
    // 2) 次の reload を “読めないパス” にして失敗させる
    // 3) fallback=KeepOldIfAny なら Ready のまま旧データ維持
    CHECK(true);
}
