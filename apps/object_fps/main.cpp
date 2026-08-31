#include "RetroFPS/App/CampaignContentLoader.hpp"
#include "RetroFPS/App/ObjectFpsPresentation.hpp"
#include "RetroFPS/App/ObjectFpsRuntimeClient.hpp"

#include "engine/asset/AssetCatalog.hpp"
#include "engine/asset/AssetManager.hpp"
#include "engine/asset/catalog/CatalogParser.hpp"
#include "engine/asset/core/AssetCachePolicy.hpp"
#include "engine/asset/core/AssetLifetime.hpp"
#include "engine/asset/core/AssetStorage.hpp"
#include "engine/asset/loaders/TextLoader.hpp"
#include "engine/asset/loaders/sdl_image/SdlImageTextureLoader.hpp"
#include "engine/asset/loading/AssetPipeline.hpp"
#include "engine/asset/loading/LoaderRegistry.hpp"
#include "engine/asset/loading/NativeFileAssetSource.hpp"
#include "engine/asset/resolver/AssetPathResolver.hpp"
#include "engine/runtime/RuntimeLoop.hpp"
#include "input/backend/sdl/SdlInput.hpp"
#include "platform/sdl/SdlPlatform.hpp"
#include "render/backend/sdl_gpu/SdlGpuRenderDevice.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#ifndef OBJECT_FPS_ASSET_ROOT
#error "OBJECT_FPS_ASSET_ROOT must identify assets/object_fps"
#endif

namespace {

template <class Error>
void LogError(const Error& error) {
    SDL_LogError(
        SDL_LOG_CATEGORY_APPLICATION,
        "%s%s%s",
        error.message.c_str(),
        error.detail.empty() ? "" : ": ",
        error.detail.c_str());
}

void LogError(const std::string& message) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message.c_str());
}

[[nodiscard]] bool HasSmokeTestArgument(int argc, char* argv[]) {
    for (int index = 1; index < argc; ++index) {
        if (std::string_view(argv[index]) == "--smoke-test") {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::filesystem::path ResolveAssetRoot() {
    if (const char* basePath = SDL_GetBasePath(); basePath != nullptr) {
        const std::filesystem::path deployed =
            std::filesystem::path(basePath) / "assets" / "object_fps";
        std::error_code error;
        if (std::filesystem::is_regular_file(
                deployed / "asset_catalog.json", error) &&
            !error) {
            return deployed;
        }
    }
    return std::filesystem::path{OBJECT_FPS_ASSET_ROOT};
}

} // namespace

int main(int argc, char* argv[]) {
    namespace Asset = Engine::Asset;
    namespace SdlInput = Engine::Input::Backend::Sdl;
    namespace SdlPlatform = Engine::Platform::Sdl;
    namespace SdlGpu = Engine::Render::Backend::SdlGpu;

    SdlPlatform::SdlPlatformOptions platformOptions;
    platformOptions.title = "Object_FPS — GYO Runtime Conformance Game";
    platformOptions.width = 1280;
    platformOptions.height = 720;
    platformOptions.resizable = false;

    auto platformResult = SdlPlatform::SdlPlatform::Create(platformOptions);
    if (!platformResult) {
        LogError(platformResult.error());
        return 1;
    }
    auto platform = std::move(platformResult).value();

    auto renderResult = SdlGpu::SdlGpuRenderDevice::Create(*platform);
    if (!renderResult) {
        LogError(renderResult.error());
        return 1;
    }
    auto renderDevice = std::move(renderResult).value();

    const std::filesystem::path assetRoot = ResolveAssetRoot();
    Asset::Resolver::AssetPathResolver::Options resolverOptions;
    resolverOptions.assetsRoot = assetRoot.string();
    resolverOptions.allowAbsolutePath = false;
    resolverOptions.allowEscapeAssetsRoot = false;
    Asset::Resolver::AssetPathResolver resolver(std::move(resolverOptions));
    Asset::Catalog::CatalogParser parser;
    Asset::AssetCatalog catalog;
    const auto catalogResult = catalog.LoadFromFile(
        (assetRoot / "asset_catalog.json").string(), parser, resolver);
    if (!catalogResult) {
        LogError(catalogResult.error());
        return 1;
    }

    Asset::Loading::LoaderRegistry loaders;
    auto textLoaderResult =
        loaders.Register(std::make_unique<Asset::Loaders::TextLoader>());
    if (!textLoaderResult) {
        LogError(textLoaderResult.error());
        return 1;
    }
    auto imageLoaderResult = loaders.Register(
        std::make_unique<
            Asset::Loaders::SdlImage::SdlImageTextureLoader>());
    if (!imageLoaderResult) {
        LogError(imageLoaderResult.error());
        return 1;
    }

    Asset::Loading::NativeFileAssetSource source;
    Asset::Loading::AssetPipeline pipeline(source, loaders);
    Asset::Core::AssetStorage storage;
    Asset::Core::AssetLifetime lifetime;
    Asset::Core::AssetCachePolicy cache({
        Asset::Core::AssetCachePolicy::Mode::KeepWhileReferenced,
        120,
        true,
        0,
        0,
    });
    Asset::AssetManager assets(
        catalog, pipeline, storage, lifetime, cache);

    fps::CampaignContentBuildResult contentResult =
        fps::CampaignContentLoader::Load(assets);
    if (!contentResult) {
        LogError(contentResult.error);
        return 1;
    }
    auto content = std::make_shared<const fps::CampaignContent>(
        std::move(*contentResult.content));

    const bool smokeTest = HasSmokeTestArgument(argc, argv);
    fps::GameSessionConfig sessionConfig;
    if (smokeTest) {
        sessionConfig.fadeOutSeconds = 0.0001F;
        sessionConfig.fadeInSeconds = 0.0001F;
    }
    fps::ObjectFpsPresentationConfig presentationConfig;
    presentationConfig.world = sessionConfig.world;

    fps::ObjectFpsPresentation presentation;
    std::string error;
    if (!presentation.Initialize(
            *renderDevice,
            assets,
            content,
            presentationConfig,
            error)) {
        LogError(error);
        return 1;
    }

    SdlInput::SdlInput input(*platform);
    fps::ObjectFpsRuntimeClient client(
        *platform,
        input,
        assets,
        presentation,
        {smokeTest, smokeTest});
    if (!client.Initialize(content, sessionConfig, error)) {
        LogError(error);
        return 1;
    }

    Engine::Runtime::RuntimeLoop loop(client);
    loop.Run();
    if (!client.LastError().empty()) {
        LogError(client.LastError());
    }
    return client.ExitCode();
}
