#include <string>
#include <utility>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <nlohmann/json.hpp>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

#include "engine/runtime/RuntimeLoop.hpp"
#include "platform/sdl/SdlPlatform.hpp"
#include "render/backend/sdl/SdlRenderer.hpp"

namespace {

using Engine::Platform::Sdl::SdlPlatform;
using Engine::Platform::Sdl::SdlPlatformOptions;
using Engine::Render::Backend::Sdl::Color;
using Engine::Render::Backend::Sdl::SdlRenderer;
using Engine::Runtime::FrameContext;
using Engine::Runtime::IRuntimeClient;
using Engine::Runtime::RuntimeControl;

template <typename Error>
void LogError(const Error& error) {
    SDL_LogError(
        SDL_LOG_CATEGORY_APPLICATION,
        "%s%s%s",
        error.message.c_str(),
        error.detail.empty() ? "" : ": ",
        error.detail.c_str());
}

class SandboxClient final : public IRuntimeClient {
public:
    SandboxClient(SdlPlatform& platform, SdlRenderer& renderer)
        : platform_(platform), renderer_(renderer) {
        document_["app"] = "gyo_sandbox";
        document_["window"] = {{"w", 1280}, {"h", 720}};
        document_["value"] = value_;
    }

    ~SandboxClient() override {
        if (rendererBackendInitialized_) {
            ImGui_ImplSDLRenderer3_Shutdown();
        }
        if (platformBackendInitialized_) {
            ImGui_ImplSDL3_Shutdown();
        }
        if (contextCreated_) {
            ImGui::DestroyContext();
        }
    }

    [[nodiscard]] bool Initialize() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        contextCreated_ = true;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForSDLRenderer(
                platform_.NativeWindow(), renderer_.NativeRenderer())) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "ImGui SDL3 platform backend initialization failed");
            return false;
        }
        platformBackendInitialized_ = true;

        if (!ImGui_ImplSDLRenderer3_Init(renderer_.NativeRenderer())) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "ImGui SDL renderer backend initialization failed");
            return false;
        }
        rendererBackendInitialized_ = true;
        return true;
    }

    RuntimeControl ProcessEvents(const FrameContext&) override {
        return platform_.PumpEvents([](const SDL_Event& event) {
            ImGui_ImplSDL3_ProcessEvent(&event);
        });
    }

    RuntimeControl Update(const FrameContext&) override {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Control");
        ImGui::Checkbox("Show ImGui Demo Window", &showDemoWindow_);
        ImGui::SliderFloat("value", &value_, 0.0F, 1.0F);

        document_["value"] = value_;
        const std::string jsonText = document_.dump(2);

        ImGui::Separator();
        ImGui::TextUnformatted("nlohmann/json dump:");
        ImGui::BeginChild("json", ImVec2(0, 200), true);
        ImGui::TextUnformatted(jsonText.c_str());
        ImGui::EndChild();
        ImGui::End();

        if (showDemoWindow_) {
            ImGui::ShowDemoWindow(&showDemoWindow_);
        }

        return RuntimeControl::Continue;
    }

    RuntimeControl Render(const FrameContext&) override {
        ImGui::Render();

        auto clearResult = renderer_.Clear(Color{20, 20, 22, 255});
        if (!clearResult) {
            LogError(clearResult.error());
            exitCode_ = 1;
            return RuntimeControl::Stop;
        }

        ImGui_ImplSDLRenderer3_RenderDrawData(
            ImGui::GetDrawData(), renderer_.NativeRenderer());

        auto presentResult = renderer_.Present();
        if (!presentResult) {
            LogError(presentResult.error());
            exitCode_ = 1;
            return RuntimeControl::Stop;
        }

        return RuntimeControl::Continue;
    }

    [[nodiscard]] int ExitCode() const noexcept {
        return exitCode_;
    }

private:
    SdlPlatform& platform_;
    SdlRenderer& renderer_;
    nlohmann::json document_;
    bool contextCreated_{};
    bool platformBackendInitialized_{};
    bool rendererBackendInitialized_{};
    bool showDemoWindow_{true};
    float value_{0.5F};
    int exitCode_{};
};

} // namespace

int main(int, char*[]) {
    SdlPlatformOptions platformOptions;
    platformOptions.title = "GYO Sandbox";

    auto platformResult = SdlPlatform::Create(platformOptions);
    if (!platformResult) {
        LogError(platformResult.error());
        return 1;
    }
    auto platform = std::move(platformResult).value();

    auto rendererResult = SdlRenderer::Create(*platform);
    if (!rendererResult) {
        LogError(rendererResult.error());
        return 1;
    }
    auto renderer = std::move(rendererResult).value();

    SandboxClient client(*platform, *renderer);
    if (!client.Initialize()) {
        return 1;
    }

    Engine::Runtime::RuntimeLoop loop(client);
    loop.Run();
    return client.ExitCode();
}
