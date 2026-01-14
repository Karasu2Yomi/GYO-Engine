add_executable(engine_tests
    test_main.cpp
    asset/CatalogParserTests.cpp
    asset/AssetPathResolverTests.cpp
    asset/AssetCatalogTests.cpp
    asset/AssetWatcherTests.cpp
    asset/AssetManagerTests.cpp
)

target_link_libraries(engine_tests PRIVATE
    engine
    doctest::doctest
    # Framework  # engineテストで必要なら
)

add_test(NAME engine_tests COMMAND engine_tests)
