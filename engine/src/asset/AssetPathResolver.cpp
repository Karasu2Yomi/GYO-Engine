#include "engine/asset/resolver/AssetPathResolver.hpp"

namespace Engine::Asset::Resolver {

    AssetPathResolver::AssetPathResolver(Options opt) : opt_(std::move(opt)) {}

    void AssetPathResolver::SetOptions(Options opt) { opt_ = std::move(opt); }

    const AssetPathResolver::Options& AssetPathResolver::GetOptions() const noexcept { return opt_; }

    std::string AssetPathResolver::NormalizePath(std::string_view path,
                                                 bool normalizeSeparators,
                                                 bool squashSlashes) {
        return Engine::IO::Path::NormalizeSlashes(path, normalizeSeparators, squashSlashes);
    }

    Base::Result<std::string, AssetError> AssetPathResolver::Resolve(std::string_view catalogPath) const {
        if (catalogPath.empty()) {
            return Base::Result<std::string, AssetError>::Err(
                AssetError::Make(
                    AssetErrorCode::InvalidPath,
                    "AssetPathResolver: empty path",
                    std::string(catalogPath))
            );
        }

        // 1) scheme 剥がし（res://, assets:// など）
        std::string p;
        if (opt_.allowSchemes) {
            auto u = Engine::IO::Path::ParseUriLoose(catalogPath);
            p = u.path.Str();
        } else {
            p = std::string(catalogPath);
        }

        // 2) 正規化（区切り/連続スラッシュ）
        p = Engine::IO::Path::NormalizeSlashes(p, opt_.normalizeSeparators, opt_.squashSlashes);

        // 3) 絶対パス判定
        if (Engine::IO::Path::IsAbsolutePathLike(p)) {
            if (!opt_.allowAbsolutePath) {
                return Base::Result<std::string, AssetError>::Err(
                    AssetError::Make(
                        AssetErrorCode::InvalidPath,
                        "AssetPathResolver: absolute path is not allowed",
                        std::string(catalogPath))
                );
            }

            // 絶対パス許可の場合：dot segments だけ解決（root制約なし）
            bool escaped = false;
            std::string cleaned = Engine::IO::Path::RemoveDotSegments(p, escaped);
            return Base::Result<std::string, AssetError>::Ok(std::move(cleaned));
        }

        // 4) assetsRoot と結合
        std::string joined = Engine::IO::Path::JoinRootAndRelative(opt_.assetsRoot, p);
        joined = Engine::IO::Path::NormalizeSlashes(joined, opt_.normalizeSeparators, opt_.squashSlashes);

        // 5) dot segments 解決（assetsRoot の外へ出るか検出）
        bool escapedAboveRoot = false;
        std::string cleaned = Engine::IO::Path::RemoveDotSegments(joined, escapedAboveRoot);

        if (escapedAboveRoot && !opt_.allowEscapeAssetsRoot) {
            return Base::Result<std::string, AssetError>::Err(
                AssetError::Make(
                    AssetErrorCode::PathEscapesRoot, // ←ここはより適切なコードを使える
                    "AssetPathResolver: path escapes assetsRoot via '..' which is not allowed",
                    std::string(catalogPath))
            );
        }

        return Base::Result<std::string, AssetError>::Ok(std::move(cleaned));
    }

} // namespace Engine::Asset::Resolver
