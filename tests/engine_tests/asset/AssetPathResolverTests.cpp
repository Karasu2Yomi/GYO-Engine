#include "doctest/doctest.h"
#include <filesystem>

#include "engine/asset/resolver/AssetPathResolver.hpp"

namespace fs = std::filesystem;
using Engine::Asset::Resolver::AssetPathResolver;

TEST_CASE("AssetPathResolver: normal join") {
    fs::path root = fs::temp_directory_path() / "asset_test_root";
    AssetPathResolver::Options options;
    options.assetsRoot = root.string();
    AssetPathResolver r(options);

    auto out = r.Resolve("textures/a.ppm");
    REQUIRE(out); // ここは value() を読むので REQUIRE が安全
    CHECK(out.value().find("textures") != std::string::npos);
}

TEST_CASE("AssetPathResolver: reject absolute path") {
    fs::path root = fs::temp_directory_path() / "asset_test_root";
    AssetPathResolver::Options options;
    options.assetsRoot = root.string();
    AssetPathResolver r(options);

#ifdef _WIN32
    auto out = r.Resolve("C:\\Windows\\win.ini");
#else
    auto out = r.Resolve("/etc/passwd");
#endif

    REQUIRE(!out); // CHECK ではなく REQUIRE
    CHECK(out.error().code == Engine::Asset::AssetErrorCode::InvalidPath);
}

TEST_CASE("AssetPathResolver: reject escape root") {
    fs::path root = fs::temp_directory_path() / "asset_test_root";
    AssetPathResolver::Options options;
    options.assetsRoot = root.string();
    AssetPathResolver r(options);

    auto out = r.Resolve("../outside.txt");

    REQUIRE(!out);
    CHECK(out.error().code == Engine::Asset::AssetErrorCode::PathEscapesRoot);
}

TEST_CASE("AssetPathResolver: allow dot segments within root") {
    fs::path root = fs::temp_directory_path() / "asset_test_root";
    AssetPathResolver::Options options;
    options.assetsRoot = root.string();
    AssetPathResolver r(options);

    auto out = r.Resolve("a/../b.txt");

    REQUIRE(out);
    CHECK(out.value().find("..") == std::string::npos);
    CHECK(out.value().find("b.txt") != std::string::npos);
}
