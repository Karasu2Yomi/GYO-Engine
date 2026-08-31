#pragma once

#include "engine/base/Result.hpp"
#include "render/RenderError.hpp"
#include "render/RenderQueue.hpp"
#include "render/RenderTypes.hpp"

namespace Engine::Render {

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    [[nodiscard]] virtual Base::Result<MeshHandle, RenderError> CreateMesh(
        const MeshView& mesh) = 0;
    [[nodiscard]] virtual Base::Result<TextureHandle, RenderError> CreateTexture(
        const ImageView& image) = 0;

    [[nodiscard]] virtual Base::Result<void, RenderError> ReleaseMesh(
        MeshHandle handle) = 0;
    [[nodiscard]] virtual Base::Result<void, RenderError> ReleaseTexture(
        TextureHandle handle) = 0;

    [[nodiscard]] virtual Base::Result<PresentStatus, RenderError> Render(
        const RenderQueue& queue) = 0;
};

} // namespace Engine::Render
