#pragma once

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

    struct BinaryAsset final {
        std::vector<std::byte> bytes;
    };

    class BinaryLoader final : public Loading::IAssetLoader {
    public:
        AssetType GetType() const noexcept override;

        Detail::Result<Core::AnyAsset, AssetError>
        Load(Detail::ConstSpan<std::byte> bytes, const Loading::LoadContext& ctx) override;
    };

} // namespace Engine::Asset::Loaders
