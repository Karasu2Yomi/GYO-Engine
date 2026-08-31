#pragma once

#include "RetroFPS/Game/GameSession.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace fps {

// Object_FPS-owned presentation policy. These commands describe what the game
// wants to show without depending on a renderer, font library, or native API.
struct UiRect final {
    float x{};
    float y{};
    float width{};
    float height{};
};

struct UiColor final {
    float red{};
    float green{};
    float blue{};
    float alpha{1.0F};
};

enum class UiTextAlignment {
    Left,
    Center,
    Right,
};

struct UiQuadCommand final {
    UiRect bounds;
    UiColor color;
    std::size_t drawOrder{};
};

struct UiTextCommand final {
    std::string text;
    UiRect bounds;
    float sizePixels{};
    UiColor color;
    UiTextAlignment alignment{UiTextAlignment::Left};
    std::size_t drawOrder{};
};

struct ObjectFpsUiFrame final {
    std::vector<UiQuadCommand> quads;
    std::vector<UiTextCommand> texts;
};

class ObjectFpsUi final {
public:
    [[nodiscard]] static ObjectFpsUiFrame Build(
        const GameSessionSnapshot& snapshot,
        UiRect viewport);

    // Returns the GameFlow menu index at the supplied absolute pointer
    // position. Layout and hit testing intentionally share the same source.
    [[nodiscard]] static std::optional<std::size_t> HitTest(
        GameScreen screen,
        float x,
        float y,
        UiRect viewport) noexcept;
};

} // namespace fps
