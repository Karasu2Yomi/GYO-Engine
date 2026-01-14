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

    // フォントは decode せず “Blob” として保持（後段の font rasterizer が使う）
    struct FontAsset final {
        std::vector<std::byte> bytes; // TTF/OTF
    };

    class FontLoader final : public Loading::IAssetLoader {
    public:
        AssetType GetType() const noexcept override;

        Detail::Result<Core::AnyAsset, AssetError>
        Load(Detail::ConstSpan<std::byte> bytes, const Loading::LoadContext& ctx) override;
    };

} // namespace Engine::Asset::Loaders
