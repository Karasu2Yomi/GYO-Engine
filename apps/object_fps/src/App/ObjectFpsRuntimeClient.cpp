#include "RetroFPS/App/ObjectFpsRuntimeClient.hpp"

#include "RetroFPS/App/ObjectFpsUi.hpp"

#include "engine/asset/AssetManager.hpp"
#include "engine/input/InputActionMap.hpp"
#include "input/backend/sdl/SdlInput.hpp"
#include "platform/sdl/SdlPlatform.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace fps {
namespace {

using Engine::Input::InputActionId;
using Engine::Input::InputAxisId;

struct ObjectFpsBindings final {
    InputAxisId moveForward{InputAxisId::FromString("object_fps.move.forward")};
    InputAxisId moveRight{InputAxisId::FromString("object_fps.move.right")};
    InputAxisId lookX{InputAxisId::FromString("object_fps.look.x")};
    InputAxisId lookY{InputAxisId::FromString("object_fps.look.y")};
    InputActionId fire{InputActionId::FromString("object_fps.fire")};
    InputActionId reload{InputActionId::FromString("object_fps.reload")};
    InputActionId menuPrevious{
        InputActionId::FromString("object_fps.menu.previous")};
    InputActionId menuNext{InputActionId::FromString("object_fps.menu.next")};
    InputActionId confirm{InputActionId::FromString("object_fps.menu.confirm")};
    InputActionId back{InputActionId::FromString("object_fps.menu.back")};
};

[[nodiscard]] Engine::Input::InputActionMap MakeActionMap(
    const ObjectFpsBindings& bindings) {
    using Engine::Input::Key;
    using Engine::Input::MouseButton;
    using Engine::Input::PointerAxis;

    Engine::Input::InputActionMap map;
    map.BindDigitalAxis(bindings.moveForward, Key::S, Key::W);
    map.BindDigitalAxis(bindings.moveRight, Key::A, Key::D);
    // Preserve raw pointer deltas at the GYO action boundary. The game's
    // PlayerSettings owns look sensitivity policy.
    map.BindPointerAxis(bindings.lookX, PointerAxis::DeltaX);
    map.BindPointerAxis(bindings.lookY, PointerAxis::DeltaY);
    map.Bind(bindings.fire, MouseButton::Left);
    map.Bind(bindings.reload, Key::R);
    map.Bind(bindings.menuPrevious, Key::W);
    map.Bind(bindings.menuPrevious, Key::Up);
    map.Bind(bindings.menuNext, Key::S);
    map.Bind(bindings.menuNext, Key::Down);
    map.Bind(bindings.confirm, Key::Enter);
    map.Bind(bindings.back, Key::Escape);
    return map;
}

} // namespace

struct ObjectFpsRuntimeClient::Impl final {
    Engine::Platform::Sdl::SdlPlatform* platform{};
    Engine::Input::Backend::Sdl::SdlInput* input{};
    Engine::Asset::AssetManager* assets{};
    ObjectFpsPresentation* presentation{};
    ObjectFpsRuntimeClientConfig config;
    ObjectFpsBindings bindings;
    Engine::Input::InputActionMap actionMap{MakeActionMap(bindings)};
    GameSession session;
    std::vector<GameSessionCommand> pendingCommands;
    std::string lastError;
    int exitCode{};
    bool initialized{};
    bool previousFocused{};
    float previousPointerX{};
    float previousPointerY{};
    bool hasPreviousAbsolutePointer{};

    [[nodiscard]] GameFrameInput TranslateInput(
        const Engine::Input::InputActionFrame& actions,
        const Engine::Input::PhysicalInputFrame& physical) noexcept {
        const Engine::Input::InputActionState fire = actions.Action(bindings.fire);
        const bool pointerPrimaryPressed =
            physical.Get(Engine::Input::MouseButton::Left).pressed;
        std::optional<std::size_t> hoveredMenuItem;
        if (!physical.pointer.relativeMode && physical.windowFocused) {
            const bool pointerMoved = !hasPreviousAbsolutePointer ||
                physical.pointer.x != previousPointerX ||
                physical.pointer.y != previousPointerY;
            if (pointerMoved || pointerPrimaryPressed) {
                hoveredMenuItem = ObjectFpsUi::HitTest(
                    session.Snapshot().screen,
                    physical.pointer.x,
                    physical.pointer.y,
                    {0.0F, 0.0F, config.viewportWidth, config.viewportHeight});
            }
            previousPointerX = physical.pointer.x;
            previousPointerY = physical.pointer.y;
            hasPreviousAbsolutePointer = true;
        } else {
            hasPreviousAbsolutePointer = false;
        }
        GameFrameInput translated;
        translated.moveForward = actions.Axis(bindings.moveForward);
        translated.moveRight = actions.Axis(bindings.moveRight);
        translated.lookDeltaX = actions.Axis(bindings.lookX);
        translated.lookDeltaY = actions.Axis(bindings.lookY);
        translated.lookEnabled =
            physical.pointer.relativeMode && physical.windowFocused;
        translated.fireHeld = fire.held;
        translated.firePressed = fire.pressed;
        translated.reloadPressed = actions.Action(bindings.reload).pressed;
        translated.menuPreviousPressed =
            actions.Action(bindings.menuPrevious).pressed;
        translated.menuNextPressed = actions.Action(bindings.menuNext).pressed;
        translated.confirmPressed = actions.Action(bindings.confirm).pressed;
        translated.backPressed = actions.Action(bindings.back).pressed;
        translated.pointerPrimaryPressed = pointerPrimaryPressed;
        translated.hoveredMenuItem = hoveredMenuItem;
        translated.focusLost = previousFocused && !physical.windowFocused;
        return translated;
    }
};

ObjectFpsRuntimeClient::ObjectFpsRuntimeClient(
    Engine::Platform::Sdl::SdlPlatform& platform,
    Engine::Input::Backend::Sdl::SdlInput& input,
    Engine::Asset::AssetManager& assets,
    ObjectFpsPresentation& presentation,
    const ObjectFpsRuntimeClientConfig config) noexcept
    : impl_(std::make_unique<Impl>()) {
    impl_->platform = &platform;
    impl_->input = &input;
    impl_->assets = &assets;
    impl_->presentation = &presentation;
    impl_->config = config;
    impl_->previousFocused = input.Snapshot().windowFocused;
}

ObjectFpsRuntimeClient::~ObjectFpsRuntimeClient() = default;

bool ObjectFpsRuntimeClient::Initialize(
    std::shared_ptr<const CampaignContent> content,
    const GameSessionConfig& config,
    std::string& error) {
    error.clear();
    impl_->lastError.clear();
    if (!impl_->presentation->IsInitialized()) {
        error = "ObjectFpsRuntimeClient requires initialized GYO presentation";
        return false;
    }
    if (!impl_->session.Initialize(std::move(content), config, error)) {
        return false;
    }
    if (impl_->config.startCampaignImmediately) {
        impl_->pendingCommands.emplace_back(StartCampaignCommand{});
    }
    impl_->initialized = true;
    return true;
}

Engine::Runtime::RuntimeControl ObjectFpsRuntimeClient::ProcessEvents(
    const Engine::Runtime::FrameContext&) {
    if (!impl_->initialized) {
        impl_->lastError = "ObjectFpsRuntimeClient is not initialized";
        impl_->exitCode = 1;
        return Engine::Runtime::RuntimeControl::Stop;
    }
    impl_->input->BeginFrame();
    const Engine::Runtime::RuntimeControl control = impl_->platform->PumpEvents(
        [this](const SDL_Event& event) { impl_->input->HandleEvent(event); });
    impl_->input->EndFrame();
    return control;
}

Engine::Runtime::RuntimeControl ObjectFpsRuntimeClient::Update(
    const Engine::Runtime::FrameContext& frame) {
    const Engine::Input::PhysicalInputFrame& physical = impl_->input->Snapshot();
    const Engine::Input::InputActionFrame actions =
        impl_->actionMap.Evaluate(physical);
    const GameFrameInput gameInput = impl_->TranslateInput(actions, physical);

    impl_->assets->BeginFrame(frame.frameIndex);
    impl_->assets->Update();

    const float deltaSeconds = static_cast<float>(
        std::clamp(frame.deltaSeconds, 0.0, 0.05));
    if (!impl_->session.Advance(
            deltaSeconds,
            gameInput,
            std::span<const GameSessionCommand>(impl_->pendingCommands),
            impl_->lastError)) {
        impl_->pendingCommands.clear();
        impl_->exitCode = 1;
        return Engine::Runtime::RuntimeControl::Stop;
    }
    impl_->pendingCommands.clear();

    const bool wantsRelativeMouse =
        impl_->session.Snapshot().screen == GameScreen::Playing &&
        physical.windowFocused;
    const auto relativeResult =
        impl_->input->SetRelativeMouseMode(wantsRelativeMouse);
    if (!relativeResult) {
        impl_->lastError = relativeResult.error().message;
        if (!relativeResult.error().detail.empty()) {
            impl_->lastError += ": " + relativeResult.error().detail;
        }
        impl_->exitCode = 1;
        return Engine::Runtime::RuntimeControl::Stop;
    }
    impl_->previousFocused = physical.windowFocused;
    return impl_->session.Snapshot().quitRequested
        ? Engine::Runtime::RuntimeControl::Stop
        : Engine::Runtime::RuntimeControl::Continue;
}

Engine::Runtime::RuntimeControl ObjectFpsRuntimeClient::Render(
    const Engine::Runtime::FrameContext& frame) {
    if (!impl_->presentation->Present(impl_->session.Snapshot(), impl_->lastError)) {
        impl_->exitCode = 1;
        return Engine::Runtime::RuntimeControl::Stop;
    }
    static_cast<void>(frame);
    if (impl_->config.stopAfterFirstMenuFrame &&
        impl_->session.Snapshot().screen == GameScreen::MainMenu) {
        if (impl_->presentation->LastVisibleSubmissionCount() == 0) {
            impl_->lastError =
                "ordinary Object_FPS startup produced no visible MainMenu submission";
            impl_->exitCode = 1;
        }
        return Engine::Runtime::RuntimeControl::Stop;
    }
    return impl_->config.stopAfterFirstPlayingFrame &&
            impl_->session.Snapshot().screen == GameScreen::Playing
        ? Engine::Runtime::RuntimeControl::Stop
        : Engine::Runtime::RuntimeControl::Continue;
}

const GameSessionSnapshot& ObjectFpsRuntimeClient::Query() const noexcept {
    return impl_->session.Snapshot();
}

void ObjectFpsRuntimeClient::Submit(GameSessionCommand command) {
    impl_->pendingCommands.push_back(std::move(command));
}

std::span<const GameSessionEvent> ObjectFpsRuntimeClient::Events() const noexcept {
    return impl_->session.Events();
}

const std::string& ObjectFpsRuntimeClient::LastError() const noexcept {
    return impl_->lastError;
}

int ObjectFpsRuntimeClient::ExitCode() const noexcept {
    return impl_->exitCode;
}

} // namespace fps
