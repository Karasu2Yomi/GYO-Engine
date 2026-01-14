#include "engine/asset/resolver/AssetPathResolver.hpp"

#include <cctype>
#include <vector>

namespace Engine::Asset::Resolver {

    static inline bool IsSlash(char c) noexcept { return c == '/' || c == '\\'; }

    AssetPathResolver::AssetPathResolver(Options opt) : opt_(std::move(opt)) {}

    void AssetPathResolver::SetOptions(Options opt) { opt_ = std::move(opt); }

    const AssetPathResolver::Options& AssetPathResolver::GetOptions() const noexcept { return opt_; }

    Detail::Result<std::string, AssetError> AssetPathResolver::Resolve(std::string_view catalogPath) const {
        if (catalogPath.empty()) {
            return Detail::Result<std::string, AssetError>::Err(
                AssetError::Make(
                    AssetErrorCode::InvalidPath,
                    "AssetPathResolver: empty path",
                    std::string(catalogPath))
            );
        }

        // 1) スキーム除去（res://, assets://）
        bool stripped = false;
        std::string p = opt_.allowSchemes ? StripScheme(catalogPath, stripped)
                                          : std::string(catalogPath);

        // 2) 正規化（区切り/連続スラッシュ）
        p = NormalizePath(p, opt_.normalizeSeparators, opt_.squashSlashes);

        // 3) 絶対パス判定
        if (IsAbsolutePath(p)) {
            if (!opt_.allowAbsolutePath) {
                return Detail::Result<std::string, AssetError>::Err(
                    AssetError::Make(
                        AssetErrorCode::InvalidPath,
                        "AssetPathResolver: absolute path is not allowed",
                        std::string(catalogPath))
                );
            }

            // 絶対パス許可の場合：dot segments だけ解決（root制約なし）
            bool escaped = false;
            std::string cleaned = RemoveDotSegments(p, escaped);
            return Detail::Result<std::string, AssetError>::Ok(std::move(cleaned));;
        }

        // 4) assetsRoot と結合
        std::string joined = JoinRootAndRelative(opt_.assetsRoot, p);
        joined = NormalizePath(joined, opt_.normalizeSeparators, opt_.squashSlashes);

        // 5) dot segments 解決（assetsRoot の外へ出るか検出）
        bool escapedAboveRoot = false;
        std::string cleaned = RemoveDotSegments(joined, escapedAboveRoot);

        if (escapedAboveRoot && !opt_.allowEscapeAssetsRoot) {
            return Detail::Result<std::string, AssetError>::Err(
                AssetError::Make(
                    AssetErrorCode::InvalidPath,
                    "AssetPathResolver: path escapes assetsRoot via '..' which is not allowed",
                    std::string(catalogPath))
            );
        }

        return Detail::Result<std::string, AssetError>::Ok(std::move(cleaned));;
    }

    std::string AssetPathResolver::NormalizePath(std::string_view path,
                                                bool normalizeSeparators,
                                                bool squashSlashes) {
        std::string out;
        out.reserve(path.size());

        bool prevSlash = false;
        for (char c : path) {
            char x = c;

            if (normalizeSeparators && x == '\\') x = '/';

            const bool isSlash = (x == '/');
            if (squashSlashes) {
                if (isSlash) {
                    if (prevSlash) continue;
                    prevSlash = true;
                } else {
                    prevSlash = false;
                }
            }
            out.push_back(x);
        }

        // 末尾の "/" は原則保持（ただし空や "/" 単体などはそのまま）
        return out;
    }

    bool AssetPathResolver::IsAbsolutePath(std::string_view p) {
        if (p.empty()) return false;

        // Unix: "/..."
        if (p.size() >= 1 && p[0] == '/') return true;

        // Windows UNC: "\\server\share" は Normalize 前でも来るので両方対応
        if (p.size() >= 2 && IsSlash(p[0]) && IsSlash(p[1])) return true;

        // Windows drive: "C:\..." or "C:/..."
        if (p.size() >= 3 &&
            std::isalpha(static_cast<unsigned char>(p[0])) &&
            p[1] == ':' &&
            IsSlash(p[2])) {
            return true;
        }
        return false;
    }

    std::string AssetPathResolver::StripScheme(std::string_view p, bool& stripped) {
        stripped = false;

        // 例: "res://textures/a.png"
        // 例: "assets://textures/a.png"
        auto pos = p.find("://");
        if (pos == std::string_view::npos) {
            return std::string(p);
        }

        // scheme 名は検証しない（許可するなら剥がす方針）
        stripped = true;

        // "://"" の直後
        std::string_view rest = p.substr(pos + 3);

        // "res:///a" のようなケースを "a" に寄せる（先頭スラッシュ削除）
        while (!rest.empty() && (rest.front() == '/' || rest.front() == '\\')) {
            rest.remove_prefix(1);
        }
        return std::string(rest);
    }

    std::string AssetPathResolver::JoinRootAndRelative(std::string_view root, std::string_view rel) {
        std::string r = std::string(root);
        if (r.empty()) r = "assets";

        // 正規化前提で "/" へ寄せる
        for (auto& c : r) {
            if (c == '\\') c = '/';
        }

        // root の末尾が "/" で終わるようにする
        if (!r.empty() && r.back() != '/') r.push_back('/');

        // rel 先頭の "/" を除く（相対のはずだが保険）
        while (!rel.empty() && IsSlash(rel.front())) rel.remove_prefix(1);

        r.append(rel.data(), rel.size());
        return r;
    }

    std::string AssetPathResolver::RemoveDotSegments(std::string_view path, bool& escapedAboveRoot) {
        escapedAboveRoot = false;

        // "/" 区切り想定（NormalizePath で揃っている前提）
        // ただし Windows drive/UNC も来るので prefix を扱う
        std::string prefix;
        std::string_view p = path;

        // drive prefix "C:/"
        if (p.size() >= 3 &&
            std::isalpha(static_cast<unsigned char>(p[0])) &&
            p[1] == ':' &&
            p[2] == '/') {
            prefix = std::string(p.substr(0, 3)); // "C:/"
            p.remove_prefix(3);
        }
        // UNC prefix "//server/share/" は簡易対応：先頭 "//" を prefix として保持
        else if (p.size() >= 2 && p[0] == '/' && p[1] == '/') {
            prefix = "//";
            p.remove_prefix(2);
        }
        // unix absolute "/" を prefix として保持
        else if (!p.empty() && p[0] == '/') {
            prefix = "/";
            p.remove_prefix(1);
        }

        std::vector<std::string> stack;
        stack.reserve(16);

        size_t i = 0;
        while (i <= p.size()) {
            size_t j = p.find('/', i);
            if (j == std::string_view::npos) j = p.size();

            std::string_view seg = p.substr(i, j - i);
            i = j + 1;

            if (seg.empty() || seg == ".") {
                continue;
            }
            if (seg == "..") {
                if (!stack.empty()) {
                    stack.pop_back();
                } else {
                    // ルートより上に出ようとした
                    escapedAboveRoot = true;
                }
                continue;
            }

            stack.emplace_back(seg);
        }

        std::string out = prefix;
        for (size_t k = 0; k < stack.size(); ++k) {
            if (!out.empty() && out.back() != '/') out.push_back('/');
            out += stack[k];
        }

        // prefix が "/" で stack が空のときは "/" を返す（絶対パスのルート）
        if (!prefix.empty() && stack.empty()) {
            // "C:/" や "/" を維持
            return out.empty() ? prefix : out;
        }

        // 相対で空になった場合は空文字を返す（呼び出し元で扱う）
        return out;
    }

} // namespace Engine::Asset::Resolver
