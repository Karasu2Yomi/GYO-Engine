#pragma once

#include <memory>

#include "engine/base/Result.hpp"
#include "platform/sdl/SdlPlatform.hpp"
#include "render/IRenderDevice.hpp"

namespace Engine::Render::Backend::SdlGpu {

struct SdlGpuOptions final {
    bool vsync{true};
#if defined(NDEBUG)
    bool debugMode{false};
#else
    bool debugMode{true};
#endif
};

// Windows/MSVC SDL_GPU implementation. SDL_GPU and D3D compiler objects are
// confined to the private implementation; consumers see only IRenderDevice.
class SdlGpuRenderDevice final : public IRenderDevice {
public:
    [[nodiscard]] static Base::Result<std::unique_ptr<SdlGpuRenderDevice>, RenderError>
    Create(
        Platform::Sdl::SdlPlatform& platform,
        const SdlGpuOptions& options = {});

    ~SdlGpuRenderDevice() override;

    SdlGpuRenderDevice(const SdlGpuRenderDevice&) = delete;
    SdlGpuRenderDevice& operator=(const SdlGpuRenderDevice&) = delete;
    SdlGpuRenderDevice(SdlGpuRenderDevice&&) = delete;
    SdlGpuRenderDevice& operator=(SdlGpuRenderDevice&&) = delete;

    [[nodiscard]] Base::Result<MeshHandle, RenderError> CreateMesh(
        const MeshView& mesh) override;
    [[nodiscard]] Base::Result<TextureHandle, RenderError> CreateTexture(
        const ImageView& image) override;
    [[nodiscard]] Base::Result<void, RenderError> ReleaseMesh(
        MeshHandle handle) override;
    [[nodiscard]] Base::Result<void, RenderError> ReleaseTexture(
        TextureHandle handle) override;
    [[nodiscard]] Base::Result<PresentStatus, RenderError> Render(
        const RenderQueue& queue) override;

private:
    struct Impl;

    explicit SdlGpuRenderDevice(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

} // namespace Engine::Render::Backend::SdlGpu
