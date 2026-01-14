#include "doctest/doctest.h"

#include <filesystem>
#include <fstream>

#include "engine/asset/AssetCatalog.hpp"
#include "engine/asset/catalog/CatalogParser.hpp"
#include "engine/asset/resolver/AssetPathResolver.hpp"
#include "engine/asset/AssetId.hpp"

namespace fs = std::filesystem;
using Engine::Asset::AssetCatalog;
using Engine::Asset::AssetId;
using Engine::Asset::Catalog::CatalogParser;
using Engine::Asset::Resolver::AssetPathResolver;

static void WriteText(const fs::path& p, const std::string& s) {
    fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string(), std::ios::binary);
    ofs << s;
}

TEST_CASE("AssetCatalog: build entries with resolvedPath") {
    fs::path tmp = fs::temp_directory_path() / "asset_catalog_test";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    fs::path assetsRoot = tmp / "assets";
    fs::path catalogPath = tmp / "config/engine/asset_catalog.json";

    WriteText(catalogPath, R"({
      "assets":[
        {"id":"ui.title","type":"text","path":"ui/title.txt"}
      ]
    })");
    AssetPathResolver::Options options;
    options.assetsRoot = assetsRoot.string();
    AssetPathResolver resolver(options);
    CatalogParser parser;
    AssetCatalog catalog;

    auto r = catalog.LoadFromFile(catalogPath.string(), parser, resolver);
    CHECK(r);

    const auto* e = catalog.Find(AssetId::FromString("ui.title"));
    REQUIRE(e != nullptr);
    CHECK(!e->sourcePath.empty());
    CHECK(!e->resolvedPath.empty());
    CHECK(e->resolvedPath.find("assets") != std::string::npos);
}

TEST_CASE("AssetCatalog: duplicate id should fail") {
    fs::path tmp = fs::temp_directory_path() / "asset_catalog_test_dup";
    fs::remove_all(tmp);

    fs::path assetsRoot = tmp / "assets";
    fs::path catalogPath = tmp / "config/engine/asset_catalog.json";

    WriteText(catalogPath, R"({
      "assets":[
        {"id":"a","type":"text","path":"a.txt"},
        {"id":"a","type":"text","path":"b.txt"}
      ]
    })");

    AssetPathResolver::Options options;
    options.assetsRoot = assetsRoot.string();
    AssetPathResolver resolver(options);
    CatalogParser parser;
    AssetCatalog catalog;

    auto r = catalog.LoadFromFile(catalogPath.string(), parser, resolver);
    CHECK(!r);
    CHECK(r.error().code == Engine::Asset::AssetErrorCode::InvalidCatalogEntry);
}
