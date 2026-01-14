#pragma once

#include <string>
#include <string_view>

#include "engine/asset/AssetError.hpp"
#include "engine/asset/detail/Result.hpp"


namespace Engine::Asset::Resolver {

    class AssetPathResolver final {
    public:
        struct Options final {
            // assets ディレクトリのルート（例: "assets" / "assets/"）
            std::string assetsRoot = "assets";

            // 絶対パス（/xxx, C:\xxx など）を許可するか
            bool allowAbsolutePath = false;

            // ".." を許可して assetsRoot の外へ出ても良いか（通常は false 推奨）
            bool allowEscapeAssetsRoot = false;

            // パス区切りを '/' に正規化するか
            bool normalizeSeparators = true;

            // 連続スラッシュ "a//b" を縮めるか
            bool squashSlashes = true;

            // "res://", "assets://" などを許可して剥がすか
            bool allowSchemes = true;
        };

    public:
        AssetPathResolver() = default;
        explicit AssetPathResolver(Options opt);

        void SetOptions(Options opt);
        const Options& GetOptions() const noexcept;

        // Catalog の path を assetsRoot を基準に解決する
        // - 入力: "textures/player.png" / "res://textures/player.png" / "assets://textures/player.png"
        // - 出力: "<assetsRoot>/textures/player.png" (正規化済み)
        Detail::Result<std::string, AssetError> Resolve(std::string_view catalogPath) const;

        // 便利：正規化のみ（単体テストにも使える）
        static std::string NormalizePath(std::string_view path,
                                         bool normalizeSeparators = true,
                                         bool squashSlashes = true);

    private:
        static bool IsAbsolutePath(std::string_view p);
        static std::string StripScheme(std::string_view p, bool& stripped);
        static std::string JoinRootAndRelative(std::string_view root, std::string_view rel);
        static std::string RemoveDotSegments(std::string_view path, bool& escapedAboveRoot);

    private:
        Options opt_{};
    };

} // namespace Engine::Asset::Resolver
