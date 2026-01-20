add_library(engine STATIC
        include/engine/io/fs/IFileSystem.hpp
        include/engine/io/fs/FileInfo.hpp)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(engine
    PRIVATE
    # base
    # io/path
    src/io/path/PathUtils.cpp
    src/io/path/Uri.cpp
    # io/stream
    src/io/stream/BufferedStream.cpp
    src/io/stream/StreamReader.cpp
    src/io/stream/StreamWriter.cpp

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