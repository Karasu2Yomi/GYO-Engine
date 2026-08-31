#include "RetroFPS/App/ObjectFpsPresentation.hpp"

#include "RetroFPS/App/ObjectFpsUi.hpp"
#include "RetroFPS/Rendering/EnemyRenderSettings.hpp"
#include "RetroFPS/Rendering/MapGeometryGenerator.hpp"

#include "engine/asset/AssetHandle.hpp"
#include "engine/asset/AssetManager.hpp"
#include "engine/asset/AssetRequest.hpp"
#include "engine/asset/AssetType.hpp"
#include "engine/asset/loaders/FontAsset.hpp"
#include "engine/asset/loaders/TextureAsset.hpp"
#include "render/IRenderDevice.hpp"
#include "render/PrimitiveMesh.hpp"
#include "render/RenderQueue.hpp"
#include "text/ITextRasterizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <numbers>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fps {
namespace {

[[nodiscard]] std::string AssetName(const Engine::Asset::AssetId& id) {
    return id.debugName.empty() ? std::to_string(id.value) : id.debugName;
}

[[nodiscard]] Engine::Render::Transform3D ConvertTransform(
    const SurfaceTransform& source) noexcept {
    return {
        {source.translation.x, source.translation.y, source.translation.z},
        {
            source.rotationRadians.x,
            source.rotationRadians.y,
            source.rotationRadians.z,
        },
        {source.scale.x, source.scale.y, source.scale.z},
    };
}

} // namespace

struct ObjectFpsPresentation::Impl final {
    struct TextureResource final {
        Engine::Asset::AssetHandle asset;
        Engine::Render::TextureHandle gpu;
        std::uint32_t width{};
        std::uint32_t height{};
    };

    struct TextCacheKey final {
        std::string utf8;
        float pointSize{};

        friend bool operator==(
            const TextCacheKey&,
            const TextCacheKey&) noexcept = default;
    };

    struct TextCacheKeyHash final {
        [[nodiscard]] std::size_t operator()(
            const TextCacheKey& key) const noexcept {
            const std::size_t textHash = std::hash<std::string>{}(key.utf8);
            const std::size_t sizeHash = std::hash<float>{}(key.pointSize);
            return textHash ^
                   (sizeHash + 0x9e3779b97f4a7c15ULL +
                    (textHash << 6U) + (textHash >> 2U));
        }
    };

    struct TextResource final {
        Engine::Render::TextureHandle gpu;
        std::uint32_t width{};
        std::uint32_t height{};
    };

    Engine::Render::IRenderDevice* renderDevice{};
    Engine::Text::ITextRasterizer* textRasterizer{};
    Engine::Asset::AssetManager* assets{};
    std::shared_ptr<const CampaignContent> content;
    Engine::Asset::AssetHandle fontAsset;
    std::shared_ptr<const Engine::Asset::Loaders::FontAsset> font;
    ObjectFpsPresentationConfig config;
    Engine::Render::MeshHandle quadXy;
    Engine::Render::MeshHandle quadXz;
    Engine::Render::MeshHandle cube;
    Engine::Render::MeshHandle skySphere;
    std::unordered_map<Engine::Asset::AssetId, TextureResource> textures;
    std::unordered_map<TextCacheKey, TextResource, TextCacheKeyHash> textCache;
    std::vector<MapGeometry> stageGeometry;
    Engine::Render::RenderQueue queue;
    std::size_t lastVisibleSubmissionCount{};
    bool initialized{};

    ~Impl() { Reset(); }

    void Reset() noexcept {
        queue.Reset();
        if (renderDevice != nullptr) {
            for (const auto& [key, text] : textCache) {
                static_cast<void>(key);
                if (text.gpu.IsValid()) {
                    static_cast<void>(renderDevice->ReleaseTexture(text.gpu));
                }
            }
            for (const auto& [id, texture] : textures) {
                static_cast<void>(id);
                if (texture.gpu.IsValid()) {
                    static_cast<void>(renderDevice->ReleaseTexture(texture.gpu));
                }
            }
            if (quadXy.IsValid()) {
                static_cast<void>(renderDevice->ReleaseMesh(quadXy));
            }
            if (quadXz.IsValid()) {
                static_cast<void>(renderDevice->ReleaseMesh(quadXz));
            }
            if (cube.IsValid()) {
                static_cast<void>(renderDevice->ReleaseMesh(cube));
            }
            if (skySphere.IsValid()) {
                static_cast<void>(renderDevice->ReleaseMesh(skySphere));
            }
        }
        if (assets != nullptr) {
            for (const auto& [id, texture] : textures) {
                static_cast<void>(id);
                assets->Release(texture.asset);
            }
            if (fontAsset.valid()) {
                assets->Release(fontAsset);
            }
        }
        textCache.clear();
        textures.clear();
        stageGeometry.clear();
        font.reset();
        fontAsset.reset();
        quadXy = {};
        quadXz = {};
        cube = {};
        skySphere = {};
        content.reset();
        assets = nullptr;
        textRasterizer = nullptr;
        renderDevice = nullptr;
        lastVisibleSubmissionCount = 0;
        initialized = false;
    }

    [[nodiscard]] bool CreateMesh(
        const Engine::Render::MeshData& data,
        Engine::Render::MeshHandle& destination,
        const char* name,
        std::string& error) {
        auto result = renderDevice->CreateMesh(data.View());
        if (!result) {
            error = std::string("failed to create GYO ") + name + " mesh: " +
                    result.error().message;
            return false;
        }
        destination = result.value();
        return true;
    }

    [[nodiscard]] bool LoadFont(
        const Engine::Asset::AssetId& id,
        std::string& error) {
        if (!id.IsValid()) {
            error = "Object_FPS UI font asset id is invalid";
            return false;
        }

        const auto load = assets->Load(
            id,
            Engine::Asset::AssetRequest::WithTypeHint(
                Engine::Asset::AssetType::Font()));
        if (!load) {
            error = "failed to load UI font asset '" + AssetName(id) + "': " +
                    load.error().message;
            return false;
        }

        fontAsset = load.value();
        font = assets->GetSharedConst<Engine::Asset::Loaders::FontAsset>(fontAsset);
        if (!font || font->bytes.empty()) {
            assets->Release(fontAsset);
            fontAsset.reset();
            font.reset();
            error = "UI font asset '" + AssetName(id) +
                    "' has no encoded font payload";
            return false;
        }
        return true;
    }

    [[nodiscard]] const TextResource* ResolveText(
        const UiTextCommand& command,
        std::string& error) {
        const TextCacheKey key{command.text, command.sizePixels};
        const auto cached = textCache.find(key);
        if (cached != textCache.end()) {
            return &cached->second;
        }
        if (textRasterizer == nullptr || !font) {
            error = "Object_FPS text presentation is not initialized";
            return nullptr;
        }

        const std::span<const std::byte> encodedFont{
            font->bytes.data(),
            font->bytes.size(),
        };
        auto rasterized = textRasterizer->Rasterize(
            encodedFont,
            Engine::Text::TextRasterRequest{command.text, command.sizePixels});
        if (!rasterized) {
            error = "failed to rasterize Object_FPS UI text: " +
                    rasterized.error().message;
            return nullptr;
        }

        Engine::Text::TextBitmap& bitmap = rasterized.value();
        const std::uint64_t tightRowPitch =
            static_cast<std::uint64_t>(bitmap.width) * 4U;
        const std::uint64_t precedingRows =
            bitmap.height == 0 ? 0 : static_cast<std::uint64_t>(bitmap.height - 1U);
        const std::uint64_t maximum =
            (std::numeric_limits<std::uint64_t>::max)();
        const bool byteCountOverflows =
            precedingRows != 0 &&
            static_cast<std::uint64_t>(bitmap.rowPitch) >
                (maximum - tightRowPitch) / precedingRows;
        const std::uint64_t requiredBytes = byteCountOverflows
            ? maximum
            : precedingRows * static_cast<std::uint64_t>(bitmap.rowPitch) +
                  tightRowPitch;
        if (bitmap.width == 0 || bitmap.height == 0 ||
            static_cast<std::uint64_t>(bitmap.rowPitch) < tightRowPitch ||
            byteCountOverflows || requiredBytes > bitmap.rgba8.size()) {
            error = "text rasterizer returned an invalid RGBA8 bitmap";
            return nullptr;
        }

        const Engine::Render::ImageView image{
            bitmap.width,
            bitmap.height,
            bitmap.rowPitch,
            std::span<const std::byte>{bitmap.rgba8.data(), bitmap.rgba8.size()},
            Engine::Render::TextureColorSpace::Linear,
        };
        auto uploaded = renderDevice->CreateTexture(image);
        if (!uploaded) {
            error = "failed to upload Object_FPS UI text: " +
                    uploaded.error().message;
            return nullptr;
        }

        auto [inserted, wasInserted] = textCache.emplace(
            key,
            TextResource{uploaded.value(), bitmap.width, bitmap.height});
        if (!wasInserted) {
            static_cast<void>(renderDevice->ReleaseTexture(uploaded.value()));
        }
        return &inserted->second;
    }

    [[nodiscard]] bool LoadTexture(
        const Engine::Asset::AssetId& id,
        std::string& error) {
        if (!id.IsValid() || textures.contains(id)) {
            return id.IsValid();
        }

        const auto load = assets->Load(
            id,
            Engine::Asset::AssetRequest::WithTypeHint(
                Engine::Asset::AssetType::Texture()));
        if (!load) {
            error = "failed to load texture asset '" + AssetName(id) + "': " +
                    load.error().message;
            return false;
        }

        const Engine::Asset::AssetHandle assetHandle = load.value();
        const auto texture =
            assets->GetSharedConst<Engine::Asset::Loaders::TextureAsset>(assetHandle);
        if (!texture || texture->width == 0 || texture->height == 0 ||
            texture->rgba.size() !=
                static_cast<std::size_t>(texture->width) * texture->height * 4U) {
            assets->Release(assetHandle);
            error = "texture asset '" + AssetName(id) +
                    "' has no valid decoded RGBA payload";
            return false;
        }

        const std::span<const std::uint8_t> rgba(texture->rgba);
        const Engine::Render::ImageView image{
            texture->width,
            texture->height,
            texture->width * 4U,
            std::as_bytes(rgba),
            Engine::Render::TextureColorSpace::SRgb,
        };
        auto create = renderDevice->CreateTexture(image);
        if (!create) {
            assets->Release(assetHandle);
            error = "failed to upload texture asset '" + AssetName(id) + "': " +
                    create.error().message;
            return false;
        }

        textures.emplace(
            id,
            TextureResource{assetHandle, create.value(), texture->width, texture->height});
        return true;
    }

    [[nodiscard]] const TextureResource* FindTexture(
        const Engine::Asset::AssetId& id) const noexcept {
        const auto found = textures.find(id);
        return found == textures.end() ? nullptr : &found->second;
    }

    [[nodiscard]] bool Submit(
        const Engine::Render::MeshSubmission& submission,
        std::string& error) {
        const auto result = queue.Submit(submission);
        if (!result) {
            error = "Object_FPS produced an invalid GYO mesh submission: " +
                    result.error().message;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool Submit(
        const Engine::Render::SpriteSubmission& submission,
        std::string& error) {
        const auto result = queue.Submit(submission);
        if (!result) {
            error = "Object_FPS produced an invalid GYO sprite submission: " +
                    result.error().message;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool SubmitUiQuad(
        const UiQuadCommand& quad,
        std::string& error) {
        if (quad.bounds.width <= 0.0F || quad.bounds.height <= 0.0F ||
            quad.color.alpha <= 0.0F) {
            return true;
        }
        Engine::Render::SpriteSubmission submission;
        submission.destinationPixels = {
            quad.bounds.x,
            quad.bounds.y,
            quad.bounds.width,
            quad.bounds.height,
        };
        submission.tint = {
            quad.color.red,
            quad.color.green,
            quad.color.blue,
            quad.color.alpha,
        };
        return Submit(submission, error);
    }

    [[nodiscard]] bool SubmitUiText(
        const UiTextCommand& text,
        std::string& error) {
        if (text.text.empty() || text.bounds.width <= 0.0F ||
            text.bounds.height <= 0.0F || text.color.alpha <= 0.0F) {
            return true;
        }
        const TextResource* resource = ResolveText(text, error);
        if (resource == nullptr) {
            return false;
        }

        const float width = static_cast<float>(resource->width);
        const float height = static_cast<float>(resource->height);
        float x = text.bounds.x;
        switch (text.alignment) {
        case UiTextAlignment::Left:
            break;
        case UiTextAlignment::Center:
            x += (text.bounds.width - width) * 0.5F;
            break;
        case UiTextAlignment::Right:
            x += text.bounds.width - width;
            break;
        }
        const float y = text.bounds.y + (text.bounds.height - height) * 0.5F;

        Engine::Render::SpriteSubmission submission;
        submission.texture = resource->gpu;
        submission.destinationPixels = {x, y, width, height};
        submission.tint = {
            text.color.red,
            text.color.green,
            text.color.blue,
            text.color.alpha,
        };
        return Submit(submission, error);
    }

    [[nodiscard]] bool SubmitUi(
        const GameSessionSnapshot& snapshot,
        std::string& error) {
        const ObjectFpsUiFrame ui = ObjectFpsUi::Build(
            snapshot,
            {0.0F, 0.0F, config.viewportWidth, config.viewportHeight});

        std::size_t quadIndex = 0;
        std::size_t textIndex = 0;
        while (quadIndex < ui.quads.size() || textIndex < ui.texts.size()) {
            const bool quadIsNext = textIndex == ui.texts.size() ||
                (quadIndex < ui.quads.size() &&
                 ui.quads[quadIndex].drawOrder < ui.texts[textIndex].drawOrder);
            if (quadIsNext) {
                if (!SubmitUiQuad(ui.quads[quadIndex], error)) {
                    return false;
                }
                ++quadIndex;
            } else {
                if (!SubmitUiText(ui.texts[textIndex], error)) {
                    return false;
                }
                ++textIndex;
            }
        }
        return true;
    }

    [[nodiscard]] bool SubmitWorld(
        const GameSessionSnapshot& snapshot,
        std::string& error) {
        if (!snapshot.activeStage || !snapshot.player) {
            return true;
        }

        const CampaignStageContent* stage =
            content->FindStage(snapshot.activeStage->levelId);
        if (stage == nullptr || snapshot.activeStage->ordinal >= stageGeometry.size()) {
            error = "snapshot references campaign stage content that is not loaded";
            return false;
        }

        const TextureResource* floor = FindTexture(config.floorTexture);
        const TextureResource* wall = FindTexture(config.wallTexture);
        if (floor == nullptr || wall == nullptr) {
            error = "Object_FPS world textures are not initialized";
            return false;
        }

        const PlayerSnapshot& player = *snapshot.player;
        queue.SetCamera({
            {player.position.x, player.eyeHeight, player.position.z},
            {player.pitchRadians, player.yawRadians, 0.0F},
            std::numbers::pi_v<float> / 3.0F,
            0.05F,
            100.0F,
        });

        const TextureResource* sky = FindTexture(config.skyTexture);
        if (sky != nullptr &&
            !Submit({
                skySphere,
                sky->gpu,
                {{player.position.x, player.eyeHeight, player.position.z}, {}, {60, 60, 60}},
                {},
                {},
                Engine::Render::SurfaceMode::Sky,
                Engine::Render::SamplerMode::LinearWrap,
                true,
            }, error)) {
            return false;
        }

        const MapGeometry& geometry = stageGeometry[snapshot.activeStage->ordinal];
        for (const SurfaceInstance& surface : geometry.surfaces) {
            Engine::Render::MeshSubmission submission;
            submission.transform = ConvertTransform(surface.transform);
            submission.sampler = Engine::Render::SamplerMode::LinearWrap;
            submission.doubleSided = true;
            switch (surface.type) {
            case SurfaceType::Floor:
                submission.mesh = quadXz;
                submission.texture = floor->gpu;
                break;
            case SurfaceType::Wall:
                submission.mesh = quadXy;
                submission.texture = wall->gpu;
                break;
            case SurfaceType::Door:
                if (!snapshot.activeStage->doorVisible) {
                    continue;
                }
                submission.mesh = cube;
                submission.texture = wall->gpu;
                break;
            }
            if (!Submit(submission, error)) {
                return false;
            }
        }

        for (const EnemySnapshot& enemy : snapshot.enemies) {
            const EnemyDefinition* definition =
                content->Data().enemies.FindById(enemy.definitionId);
            if (definition == nullptr) {
                continue;
            }
            const TextureResource* texture = FindTexture(definition->textureAssetId);
            if (texture == nullptr) {
                continue;
            }
            const EnemyBillboardPose pose = ResolveEnemyBillboardPose(
                *definition, enemy.position, player.position);
            Engine::Render::UvTransform uv;
            const EnemyAnimationClipDefinition& clip =
                GetEnemyAnimationClip(*definition, enemy.state);
            const auto frame = ResolveEnemyAnimationFrame(
                clip, enemy.state, enemy.stateElapsedSeconds);
            if (frame) {
                const auto atlas = ResolveEnemyAtlasUv(
                    clip,
                    definition->frameWidthPixels,
                    definition->frameHeightPixels,
                    *frame,
                    texture->width,
                    texture->height);
                if (atlas) {
                    uv.scale = {atlas->scaleX, atlas->scaleY};
                    uv.offset = {atlas->offsetX, atlas->offsetY};
                }
            }
            const float flash = enemy.hitFlashRemainingSeconds > 0.0F ? 1.5F : 1.0F;
            if (!Submit({
                quadXy,
                texture->gpu,
                {
                    {enemy.position.x, pose.centerY, enemy.position.z},
                    {0.0F, pose.yawRadians, 0.0F},
                    {pose.width, pose.height, 1.0F},
                },
                {flash, flash, flash, 1.0F},
                uv,
                Engine::Render::SurfaceMode::AlphaMasked,
                Engine::Render::SamplerMode::LinearClamp,
                true,
            }, error)) {
                return false;
            }
        }

        for (const ProjectileSnapshot& projectile : snapshot.projectiles) {
            const float diameter = projectile.radius * 2.0F;
            if (!Submit({
                cube,
                {},
                {
                    {projectile.position.x, projectile.position.y, projectile.position.z},
                    {},
                    {diameter, diameter, diameter},
                },
                projectile.kind == ProjectileKind::EnemyBullet
                    ? Engine::Render::Color{1.0F, 0.25F, 0.1F, 1.0F}
                    : Engine::Render::Color{1.0F, 0.9F, 0.2F, 1.0F},
            }, error)) {
                return false;
            }
        }

        const WeaponDefinition* weapon =
            content->Data().weapons.FindById(snapshot.weapon.weaponId);
        if (weapon != nullptr) {
            const TextureResource* texture = FindTexture(weapon->textureAssetId);
            if (texture != nullptr) {
                const float width = config.viewportWidth * 0.34F;
                const float height = config.viewportHeight * 0.44F;
                if (!Submit({
                    texture->gpu,
                    {
                        (config.viewportWidth - width) * 0.5F,
                        config.viewportHeight - height,
                        width,
                        height,
                    },
                    {},
                    {},
                    0.0F,
                    {},
                }, error)) {
                    return false;
                }
            }
        }
        return true;
    }
};

ObjectFpsPresentation::ObjectFpsPresentation() noexcept
    : impl_(std::make_unique<Impl>()) {}

ObjectFpsPresentation::~ObjectFpsPresentation() = default;
ObjectFpsPresentation::ObjectFpsPresentation(ObjectFpsPresentation&&) noexcept = default;
ObjectFpsPresentation& ObjectFpsPresentation::operator=(
    ObjectFpsPresentation&&) noexcept = default;

bool ObjectFpsPresentation::Initialize(
    Engine::Render::IRenderDevice& renderDevice,
    Engine::Text::ITextRasterizer& textRasterizer,
    Engine::Asset::AssetManager& assets,
    std::shared_ptr<const CampaignContent> content,
    const ObjectFpsPresentationConfig& config,
    std::string& error) {
    error.clear();
    impl_->Reset();
    if (!content || content->Stages().empty() || config.viewportWidth <= 0.0F ||
        config.viewportHeight <= 0.0F ||
        !std::isfinite(config.world.cellSize) || config.world.cellSize <= 0.0F ||
        !std::isfinite(config.world.wallHeight) || config.world.wallHeight <= 0.0F) {
        error = "ObjectFpsPresentation requires content, a positive viewport, and valid world scale";
        return false;
    }
    impl_->renderDevice = &renderDevice;
    impl_->textRasterizer = &textRasterizer;
    impl_->assets = &assets;
    impl_->content = std::move(content);
    impl_->config = config;

    if (!impl_->LoadFont(config.uiFont, error)) {
        impl_->Reset();
        return false;
    }

    const auto sphere = Engine::Render::MakeUvSphere(16, 32);
    if (!sphere) {
        error = "failed to generate Object_FPS sky sphere: " +
                sphere.error().message;
        impl_->Reset();
        return false;
    }
    if (!impl_->CreateMesh(
            Engine::Render::MakeUnitQuadXY(), impl_->quadXy, "XY quad", error) ||
        !impl_->CreateMesh(
            Engine::Render::MakeUnitQuadXZ(), impl_->quadXz, "XZ quad", error) ||
        !impl_->CreateMesh(
            Engine::Render::MakeUnitCube(), impl_->cube, "unit cube", error) ||
        !impl_->CreateMesh(
            sphere.value(), impl_->skySphere, "sky sphere", error)) {
        impl_->Reset();
        return false;
    }

    std::vector<Engine::Asset::AssetId> textureIds{
        config.floorTexture,
        config.wallTexture,
        config.skyTexture,
    };
    for (const EnemyDefinition& enemy :
         impl_->content->Data().enemies.GetDefinitions()) {
        textureIds.push_back(enemy.textureAssetId);
    }
    for (const WeaponDefinition& weapon :
         impl_->content->Data().weapons.GetDefinitions()) {
        textureIds.push_back(weapon.textureAssetId);
    }
    for (const Engine::Asset::AssetId& id : textureIds) {
        if (!impl_->LoadTexture(id, error)) {
            impl_->Reset();
            return false;
        }
    }

    try {
        impl_->stageGeometry.reserve(impl_->content->Stages().size());
        for (const CampaignStageContent& stage : impl_->content->Stages()) {
            impl_->stageGeometry.push_back(
                MapGeometryGenerator::Generate(stage.map, impl_->config.world));
        }
    } catch (const std::exception& exception) {
        error = "failed to generate Object_FPS map presentation: ";
        error += exception.what();
        impl_->Reset();
        return false;
    }
    impl_->initialized = true;
    return true;
}

bool ObjectFpsPresentation::Present(
    const GameSessionSnapshot& snapshot,
    std::string& error) {
    error.clear();
    impl_->lastVisibleSubmissionCount = 0;
    if (!impl_->initialized) {
        error = "ObjectFpsPresentation is not initialized";
        return false;
    }

    Engine::Render::FrameDescription frame;
    switch (snapshot.screen) {
    case GameScreen::MainMenu:
        frame.clearColor = {0.03F, 0.04F, 0.07F, 1.0F};
        break;
    case GameScreen::Controls:
        frame.clearColor = {0.05F, 0.05F, 0.08F, 1.0F};
        break;
    case GameScreen::Paused:
        frame.clearColor = {0.02F, 0.02F, 0.02F, 1.0F};
        break;
    case GameScreen::Results:
        frame.clearColor = snapshot.campaignOutcome == CampaignOutcome::Completed
            ? Engine::Render::Color{0.02F, 0.10F, 0.04F, 1.0F}
            : Engine::Render::Color{0.10F, 0.02F, 0.02F, 1.0F};
        break;
    case GameScreen::Playing:
        frame.clearColor = {0.05F, 0.07F, 0.10F, 1.0F};
        break;
    }
    impl_->queue.Reset(frame);
    if ((snapshot.screen == GameScreen::Playing ||
         snapshot.screen == GameScreen::Paused) &&
        !impl_->SubmitWorld(snapshot, error)) {
        return false;
    }

    if (!impl_->SubmitUi(snapshot, error)) {
        return false;
    }

    if (snapshot.fadeOpacity > 0.0F &&
        !impl_->Submit({
            {},
            {0.0F, 0.0F, impl_->config.viewportWidth, impl_->config.viewportHeight},
            {},
            {},
            0.0F,
            {0.0F, 0.0F, 0.0F, std::clamp(snapshot.fadeOpacity, 0.0F, 1.0F)},
        }, error)) {
        return false;
    }

    impl_->lastVisibleSubmissionCount =
        impl_->queue.Meshes().size() + impl_->queue.Sprites().size();

    auto result = impl_->renderDevice->Render(impl_->queue);
    if (!result) {
        error = "GYO render device failed to present Object_FPS: " +
                result.error().message;
        return false;
    }
    return true;
}

bool ObjectFpsPresentation::IsInitialized() const noexcept {
    return impl_->initialized;
}

std::size_t ObjectFpsPresentation::LastVisibleSubmissionCount() const noexcept {
    return impl_->lastVisibleSubmissionCount;
}

} // namespace fps
