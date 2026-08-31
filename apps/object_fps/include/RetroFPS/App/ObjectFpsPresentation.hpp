#pragma once

#include "RetroFPS/Game/CampaignContent.hpp"
#include "RetroFPS/Game/GameSession.hpp"
#include "RetroFPS/World/WorldSettings.hpp"

#include "engine/asset/AssetId.hpp"

#include <memory>
#include <string>

namespace Engine::Asset {
class AssetManager;
}

namespace Engine::Render {
class IRenderDevice;
}

namespace fps {

struct ObjectFpsPresentationConfig final {
    Engine::Asset::AssetId floorTexture{
        Engine::Asset::AssetId::FromString("object_fps.texture.world.floor")};
    Engine::Asset::AssetId wallTexture{
        Engine::Asset::AssetId::FromString("object_fps.texture.world.wall")};
    Engine::Asset::AssetId skyTexture{
        Engine::Asset::AssetId::FromString("object_fps.texture.sky.default")};
    WorldSettings world{};
    float viewportWidth{1280.0F};
    float viewportHeight{720.0F};
};

// App-side projection from an immutable game snapshot to GYO's RenderQueue.
// It owns only the GYO asset handles and GPU handles required by this game;
// simulation and backend implementation remain outside this class.
class ObjectFpsPresentation final {
public:
    ObjectFpsPresentation() noexcept;
    ~ObjectFpsPresentation();

    ObjectFpsPresentation(const ObjectFpsPresentation&) = delete;
    ObjectFpsPresentation& operator=(const ObjectFpsPresentation&) = delete;
    ObjectFpsPresentation(ObjectFpsPresentation&&) noexcept;
    ObjectFpsPresentation& operator=(ObjectFpsPresentation&&) noexcept;

    [[nodiscard]] bool Initialize(
        Engine::Render::IRenderDevice& renderDevice,
        Engine::Asset::AssetManager& assets,
        std::shared_ptr<const CampaignContent> content,
        const ObjectFpsPresentationConfig& config,
        std::string& error);

    [[nodiscard]] bool Present(
        const GameSessionSnapshot& snapshot,
        std::string& error);

    [[nodiscard]] bool IsInitialized() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace fps
