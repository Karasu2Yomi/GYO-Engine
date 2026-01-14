#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

#include "engine/asset/AssetId.hpp"
#include "engine/asset/AssetType.hpp"

#include "engine/asset/core/AssetRecord.hpp"

namespace Engine::Asset::Core {

// AssetStorage:
// - map<AssetId, AssetRecord> の所有者
// - record のアドレス安定性を確保するため、unique_ptr で保持する
//   （unordered_map の rehash で参照が壊れる問題を避ける）
class AssetStorage final {
public:
    AssetStorage() = default;

    void Clear() {
        records_.clear();
    }

    std::size_t Size() const noexcept { return records_.size(); }

    AssetRecord* Find(const AssetId& id) noexcept {
        auto it = records_.find(id);
        return it == records_.end() ? nullptr : it->second.get();
    }

    const AssetRecord* Find(const AssetId& id) const noexcept {
        auto it = records_.find(id);
        return it == records_.end() ? nullptr : it->second.get();
    }

    bool Contains(const AssetId& id) const noexcept {
        return records_.find(id) != records_.end();
    }

    // 無ければ作る。type/path は「初回作成時のみ」設定する（既存なら保持）
    AssetRecord& GetOrCreate(const AssetId& id, const AssetType& type, std::string resolvedPath = {}) {
        auto it = records_.find(id);
        if (it != records_.end()) {
            return *it->second;
        }

        auto rec = std::make_unique<AssetRecord>();
        rec->id = id;
        rec->type = type;
        rec->resolvedPath = std::move(resolvedPath);
        rec->state = AssetState::Unloaded;

        auto* ptr = rec.get();
        records_.emplace(id, std::move(rec));
        return *ptr;
    }

    // “pathだけ後から埋めたい” 用（Catalog構築→Storage作成の順序差に対応）
    void SetResolvedPathIfEmpty(const AssetId& id, std::string resolvedPath) {
        if (auto* r = Find(id)) {
            if (r->resolvedPath.empty()) r->resolvedPath = std::move(resolvedPath);
        }
    }

    // 参照カウント（AssetHandle運用の補助）
    void AddRef(const AssetId& id) {
        if (auto* r = Find(id)) ++r->refCount;
    }

    void ReleaseRef(const AssetId& id) {
        if (auto* r = Find(id)) {
            if (r->refCount > 0) --r->refCount;
        }
    }

    // “解放可能か” の判定は CachePolicy/Lifetime と組み合わせて AssetManager が行う想定
    bool CanEvict(const AssetId& id) const noexcept {
        const auto* r = Find(id);
        if (!r) return false;
        return r->refCount == 0;
    }

    void EraseIf(const AssetId& id, bool force = false) {
        auto it = records_.find(id);
        if (it == records_.end()) return;

        if (force || it->second->refCount == 0) {
            records_.erase(it);
        }
    }

private:
    std::unordered_map<AssetId, std::unique_ptr<AssetRecord>> records_;
};

} // namespace Engine::Asset::Core
