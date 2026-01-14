#pragma once

#include <cstddef>

#include "engine/asset/AssetError.hpp"
#include "engine/asset/AssetType.hpp"
#include "engine/asset/core/AnyAsset.hpp"
#include "engine/asset/detail/Result.hpp"
#include "engine/asset/detail/Span.hpp"
#include "engine/asset/loading/LoadContext.hpp"

namespace Engine::Asset::Loading {

    // IAssetLoader（アイ・アセット・ローダ）
    // - bytes -> runtime resource（Core::AnyAsset）へ変換する
    // - 具体実装：TextureLoader / SoundLoader / ... がこれを実装
    class IAssetLoader {
    public:
        virtual ~IAssetLoader() = default;

        // このローダが担当する AssetType
        virtual AssetType GetType() const noexcept = 0;

        // bytes を decode/parse して AnyAsset を返す
        virtual Detail::Result<Core::AnyAsset, AssetError>
        Load(Detail::ConstSpan<std::byte> bytes, const LoadContext& ctx) = 0;
    };

} // namespace Engine::Asset::Loading
