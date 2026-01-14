#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "engine/asset/AssetError.hpp"
#include "engine/asset/AssetType.hpp"
#include "engine/asset/core/AnyAsset.hpp"
#include "engine/asset/detail/Result.hpp"
#include "engine/asset/detail/Span.hpp"
#include "engine/asset/loading/IAssetLoader.hpp"
#include "engine/asset/loading/LoadContext.hpp"

namespace Engine::Asset::Loaders {

    // 最小のテクスチャ表現（RGBA8）
    struct TextureAsset final {
        std::uint32_t width  = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> rgba; // size = width*height*4
    };

    class TextureLoader final : public Loading::IAssetLoader {
    public:
        AssetType GetType() const noexcept override;

        Detail::Result<Core::AnyAsset, AssetError>
        Load(Detail::ConstSpan<std::byte> bytes, const Loading::LoadContext& ctx) override;
    };

} // namespace Engine::Asset::Loaders
