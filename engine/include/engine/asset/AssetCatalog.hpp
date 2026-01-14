#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/asset/AssetError.hpp"
#include "engine/asset/AssetId.hpp"
#include "engine/asset/detail/Result.hpp"
#include "engine/asset/catalog/CatalogEntry.hpp"

namespace Engine::Asset::Catalog {
    struct RawCatalogEntry;
    class CatalogParser;
}

namespace Engine::Asset::Resolver {
    class AssetPathResolver;
}

namespace Engine::Asset {

    class AssetCatalog final {
    public:
        AssetCatalog() = default;

        void Clear();

        // resolvedPath込みで構築する
        Detail::Result<void, AssetError>
        LoadFromFile(std::string_view catalogJsonPath,
                     Catalog::CatalogParser& parser,
                     const Resolver::AssetPathResolver& resolver);

        const Catalog::CatalogEntry* Find(const AssetId& id) const noexcept;

        // 任意：watch登録したい場合などに全件列挙
        std::vector<const Catalog::CatalogEntry*> Entries() const;

    private:
        Detail::Result<void, AssetError>
        BuildFromRaw_(const std::vector<Catalog::RawCatalogEntry>& raw,
                      const Resolver::AssetPathResolver& resolver);

    private:
        std::unordered_map<AssetId, Catalog::CatalogEntry> map_;
    };

} // namespace Engine::Asset
