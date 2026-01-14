add_library(engine STATIC)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(engine
    PRIVATE
    # asset/catalog
    src/asset/catalog/CatalogParser.cpp
    # asset/loaders
    src/asset/loaders/BinaryLoader.cpp
    src/asset/loaders/FontLoader.cpp
    src/asset/loaders/SoundLoader.cpp
    src/asset/loaders/TextLoader.cpp
    src/asset/loaders/TextureLoader.cpp

    # asset/
    src/asset/AssetCatalog.cpp
    src/asset/AssetManager.cpp
    src/asset/AssetPathResolver.cpp
    src/asset/AssetPipeline.cpp
    src/asset/AssetWatcher.cpp
    src/asset/LoaderRegistry.cpp
)
target_link_libraries(engine PRIVATE
    nlohmann_json::nlohmann_json
)