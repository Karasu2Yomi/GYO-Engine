#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "engine/asset/AssetError.hpp"
#include "engine/asset/detail/Result.hpp"

namespace Engine::Asset::Loading {

    // バイト列の所有バッファ（I/O結果）
    using ByteBuffer = std::vector<std::byte>;

    // IAssetSource（アイ・アセット・ソース）
    // - 実体の読み出し担当（filesystem / pak / zip / memory などの抽象）
    // - 変換（decode）はしない（Loaderの責務）
    // - 失敗は AssetError に集約
    class IAssetSource {
    public:
        virtual ~IAssetSource() = default;

        virtual Detail::Result<ByteBuffer, AssetError>
        ReadAll(std::string_view resolvedPath) = 0;

        // 任意：将来使うなら
        virtual bool Exists(std::string_view /*resolvedPath*/) { return true; }
    };

} // namespace Engine::Asset::Loading
