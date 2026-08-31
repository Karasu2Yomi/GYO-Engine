#include "RetroFPS/App/ObjectFpsUi.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace fps {
namespace {

constexpr float kReferenceWidth = 1280.0F;
constexpr float kReferenceHeight = 720.0F;

constexpr UiColor kBackdrop{0.025F, 0.035F, 0.055F, 1.0F};
constexpr UiColor kPanel{0.055F, 0.075F, 0.11F, 0.94F};
constexpr UiColor kOverlay{0.01F, 0.015F, 0.025F, 0.78F};
constexpr UiColor kButton{0.11F, 0.14F, 0.19F, 0.96F};
constexpr UiColor kSelection{0.13F, 0.48F, 0.68F, 1.0F};
constexpr UiColor kAccent{0.25F, 0.82F, 0.95F, 1.0F};
constexpr UiColor kText{0.92F, 0.96F, 1.0F, 1.0F};
constexpr UiColor kMutedText{0.59F, 0.67F, 0.75F, 1.0F};
constexpr UiColor kDanger{0.93F, 0.25F, 0.22F, 1.0F};
constexpr UiColor kHealth{0.18F, 0.78F, 0.35F, 1.0F};
constexpr UiColor kBarTrack{0.08F, 0.10F, 0.13F, 0.96F};

struct UiLayout final {
    UiRect viewport;
    float scale{};
    float offsetX{};
    float offsetY{};
};

[[nodiscard]] bool IsFiniteRect(const UiRect rect) noexcept {
    return std::isfinite(rect.x) && std::isfinite(rect.y) &&
           std::isfinite(rect.width) && std::isfinite(rect.height) &&
           rect.width > 0.0F && rect.height > 0.0F;
}

[[nodiscard]] std::optional<UiLayout> MakeLayout(const UiRect viewport) noexcept {
    if (!IsFiniteRect(viewport)) {
        return std::nullopt;
    }

    const float scale = std::min(
        viewport.width / kReferenceWidth,
        viewport.height / kReferenceHeight);
    if (!std::isfinite(scale) || scale <= 0.0F) {
        return std::nullopt;
    }

    const float contentWidth = kReferenceWidth * scale;
    const float contentHeight = kReferenceHeight * scale;
    return UiLayout{
        viewport,
        scale,
        viewport.x + (viewport.width - contentWidth) * 0.5F,
        viewport.y + (viewport.height - contentHeight) * 0.5F,
    };
}

[[nodiscard]] UiRect Place(
    const UiLayout& layout,
    const float x,
    const float y,
    const float width,
    const float height) noexcept {
    return {
        layout.offsetX + x * layout.scale,
        layout.offsetY + y * layout.scale,
        width * layout.scale,
        height * layout.scale,
    };
}

[[nodiscard]] bool Contains(
    const UiRect rect,
    const float x,
    const float y) noexcept {
    return std::isfinite(x) && std::isfinite(y) && x >= rect.x && y >= rect.y &&
           x < rect.x + rect.width && y < rect.y + rect.height;
}

[[nodiscard]] std::size_t MenuItemCount(const GameScreen screen) noexcept {
    switch (screen) {
    case GameScreen::MainMenu:
    case GameScreen::Paused:
        return 3;
    case GameScreen::Controls:
    case GameScreen::Results:
        return 1;
    case GameScreen::Playing:
        return 0;
    }
    return 0;
}

[[nodiscard]] float MenuTop(const GameScreen screen) noexcept {
    switch (screen) {
    case GameScreen::MainMenu:
        return 340.0F;
    case GameScreen::Controls:
        return 594.0F;
    case GameScreen::Paused:
        return 300.0F;
    case GameScreen::Results:
        return 592.0F;
    case GameScreen::Playing:
        return 0.0F;
    }
    return 0.0F;
}

[[nodiscard]] std::vector<UiRect> MenuItemRects(
    const GameScreen screen,
    const UiLayout& layout) {
    constexpr float width = 360.0F;
    constexpr float height = 52.0F;
    constexpr float gap = 14.0F;
    const std::size_t count = MenuItemCount(screen);
    std::vector<UiRect> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const float y = MenuTop(screen) +
                        static_cast<float>(index) * (height + gap);
        result.push_back(Place(
            layout,
            (kReferenceWidth - width) * 0.5F,
            y,
            width,
            height));
    }
    return result;
}

void AddQuad(ObjectFpsUiFrame& frame, const UiRect bounds, const UiColor color) {
    const std::size_t drawOrder = frame.quads.size() + frame.texts.size();
    frame.quads.push_back({bounds, color, drawOrder});
}

void AddText(
    ObjectFpsUiFrame& frame,
    std::string text,
    const UiRect bounds,
    const float sizePixels,
    const UiColor color = kText,
    const UiTextAlignment alignment = UiTextAlignment::Left) {
    const std::size_t drawOrder = frame.quads.size() + frame.texts.size();
    frame.texts.push_back({
        std::move(text),
        bounds,
        sizePixels,
        color,
        alignment,
        drawOrder,
    });
}

void AddMenuItems(
    ObjectFpsUiFrame& frame,
    const GameSessionSnapshot& snapshot,
    const UiLayout& layout,
    const std::vector<std::string_view>& labels) {
    const std::vector<UiRect> rects = MenuItemRects(snapshot.screen, layout);
    const std::size_t count = std::min(rects.size(), labels.size());
    for (std::size_t index = 0; index < count; ++index) {
        AddQuad(
            frame,
            rects[index],
            index == snapshot.selectedMenuItem ? kSelection : kButton);
        AddText(
            frame,
            std::string{labels[index]},
            rects[index],
            24.0F * layout.scale,
            kText,
            UiTextAlignment::Center);
    }
}

void AddMainMenu(
    ObjectFpsUiFrame& frame,
    const GameSessionSnapshot& snapshot,
    const UiLayout& layout) {
    AddQuad(frame, layout.viewport, kBackdrop);
    AddQuad(frame, Place(layout, 360.0F, 100.0F, 560.0F, 500.0F), kPanel);
    AddText(
        frame,
        "OBJECT FPS",
        Place(layout, 360.0F, 150.0F, 560.0F, 68.0F),
        48.0F * layout.scale,
        kAccent,
        UiTextAlignment::Center);
    AddText(
        frame,
        "GYO RUNTIME CONFORMANCE GAME",
        Place(layout, 360.0F, 224.0F, 560.0F, 34.0F),
        18.0F * layout.scale,
        kMutedText,
        UiTextAlignment::Center);
    AddMenuItems(frame, snapshot, layout, {"START GAME", "CONTROLS", "QUIT"});
}

void AddControls(
    ObjectFpsUiFrame& frame,
    const GameSessionSnapshot& snapshot,
    const UiLayout& layout) {
    AddQuad(frame, layout.viewport, kBackdrop);
    AddQuad(frame, Place(layout, 270.0F, 70.0F, 740.0F, 580.0F), kPanel);
    AddText(
        frame,
        "CONTROLS",
        Place(layout, 310.0F, 112.0F, 660.0F, 60.0F),
        40.0F * layout.scale,
        kAccent,
        UiTextAlignment::Center);

    constexpr std::string_view labels[] = {
        "W / S    MOVE FORWARD / BACK",
        "A / D    STRAFE LEFT / RIGHT",
        "MOUSE    LOOK",
        "LMB      FIRE",
        "R        RELOAD",
        "ESC      PAUSE",
    };
    for (std::size_t index = 0; index < std::size(labels); ++index) {
        AddText(
            frame,
            std::string{labels[index]},
            Place(
                layout,
                390.0F,
                210.0F + static_cast<float>(index) * 50.0F,
                500.0F,
                34.0F),
            21.0F * layout.scale,
            kText,
            UiTextAlignment::Left);
    }
    AddMenuItems(frame, snapshot, layout, {"BACK"});
}

void AddHud(
    ObjectFpsUiFrame& frame,
    const GameSessionSnapshot& snapshot,
    const UiLayout& layout) {
    const float health = snapshot.player.has_value() ? snapshot.player->health : 0.0F;
    const float maximumHealth = snapshot.player.has_value()
        ? snapshot.player->maximumHealth
        : 0.0F;
    const float healthRatio = maximumHealth > 0.0F
        ? std::clamp(health / maximumHealth, 0.0F, 1.0F)
        : 0.0F;

    AddQuad(frame, Place(layout, 32.0F, 626.0F, 290.0F, 62.0F), kPanel);
    AddText(
        frame,
        "HP " + std::to_string(static_cast<std::uint32_t>(std::max(health, 0.0F))),
        Place(layout, 48.0F, 637.0F, 94.0F, 28.0F),
        20.0F * layout.scale);
    AddQuad(frame, Place(layout, 142.0F, 644.0F, 160.0F, 16.0F), kBarTrack);
    AddQuad(
        frame,
        Place(layout, 142.0F, 644.0F, 160.0F * healthRatio, 16.0F),
        healthRatio <= 0.25F ? kDanger : kHealth);

    AddQuad(frame, Place(layout, 1000.0F, 626.0F, 248.0F, 62.0F), kPanel);
    AddText(
        frame,
        std::to_string(snapshot.weapon.magazineAmmo) + " / " +
            std::to_string(snapshot.weapon.reserveAmmo),
        Place(layout, 1016.0F, 637.0F, 216.0F, 32.0F),
        25.0F * layout.scale,
        kText,
        UiTextAlignment::Right);

    if (snapshot.weapon.reloading) {
        const float progress = std::clamp(snapshot.weapon.reloadProgress, 0.0F, 1.0F);
        AddText(
            frame,
            "RELOADING",
            Place(layout, 500.0F, 624.0F, 280.0F, 30.0F),
            18.0F * layout.scale,
            kAccent,
            UiTextAlignment::Center);
        AddQuad(frame, Place(layout, 520.0F, 662.0F, 240.0F, 10.0F), kBarTrack);
        AddQuad(
            frame,
            Place(layout, 520.0F, 662.0F, 240.0F * progress, 10.0F),
            kAccent);
    }

    if (snapshot.activeStage.has_value()) {
        const ActiveStageSnapshot& stage = *snapshot.activeStage;
        AddQuad(frame, Place(layout, 28.0F, 24.0F, 430.0F, 48.0F), kPanel);
        AddText(
            frame,
            "STAGE " + std::to_string(stage.ordinal + 1U) + " / " +
                std::to_string(stage.stageCount) + "  " + stage.levelName,
            Place(layout, 44.0F, 32.0F, 398.0F, 32.0F),
            18.0F * layout.scale,
            kText,
            UiTextAlignment::Left);
    }

    const float expansion = std::clamp(snapshot.weapon.crosshairExpansion, 0.0F, 48.0F);
    const float gap = 8.0F + expansion;
    constexpr float armLength = 14.0F;
    constexpr float thickness = 3.0F;
    constexpr float centerX = kReferenceWidth * 0.5F;
    constexpr float centerY = kReferenceHeight * 0.5F;
    AddQuad(frame, Place(
        layout, centerX - gap - armLength, centerY - thickness * 0.5F,
        armLength, thickness), kText);
    AddQuad(frame, Place(
        layout, centerX + gap, centerY - thickness * 0.5F,
        armLength, thickness), kText);
    AddQuad(frame, Place(
        layout, centerX - thickness * 0.5F, centerY - gap - armLength,
        thickness, armLength), kText);
    AddQuad(frame, Place(
        layout, centerX - thickness * 0.5F, centerY + gap,
        thickness, armLength), kText);
}

void AddPause(
    ObjectFpsUiFrame& frame,
    const GameSessionSnapshot& snapshot,
    const UiLayout& layout) {
    AddHud(frame, snapshot, layout);
    AddQuad(frame, layout.viewport, kOverlay);
    AddQuad(frame, Place(layout, 390.0F, 120.0F, 500.0F, 440.0F), kPanel);
    AddText(
        frame,
        "PAUSED",
        Place(layout, 420.0F, 165.0F, 440.0F, 62.0F),
        42.0F * layout.scale,
        kAccent,
        UiTextAlignment::Center);
    AddMenuItems(frame, snapshot, layout, {"RESUME", "MAIN MENU", "QUIT"});
}

void AddResults(
    ObjectFpsUiFrame& frame,
    const GameSessionSnapshot& snapshot,
    const UiLayout& layout) {
    AddQuad(frame, layout.viewport, kBackdrop);
    AddQuad(frame, Place(layout, 250.0F, 54.0F, 780.0F, 620.0F), kPanel);

    std::string title = "RUN COMPLETE";
    UiColor titleColor = kAccent;
    if (snapshot.campaignOutcome == CampaignOutcome::PlayerDied) {
        title = "YOU DIED";
        titleColor = kDanger;
    } else if (snapshot.campaignOutcome == CampaignOutcome::InProgress) {
        title = "RUN ENDED";
        titleColor = kMutedText;
    }
    AddText(
        frame,
        std::move(title),
        Place(layout, 300.0F, 90.0F, 680.0F, 62.0F),
        42.0F * layout.scale,
        titleColor,
        UiTextAlignment::Center);

    std::size_t totalKills = 0;
    std::size_t visitedRooms = 0;
    for (const CampaignRoomStats& room : snapshot.campaignRooms) {
        totalKills += room.kills;
        visitedRooms += room.visited ? 1U : 0U;
    }
    AddText(
        frame,
        "ROOMS " + std::to_string(visitedRooms) + " / " +
            std::to_string(snapshot.campaignRooms.size()),
        Place(layout, 390.0F, 190.0F, 500.0F, 36.0F),
        22.0F * layout.scale,
        kText,
        UiTextAlignment::Center);
    AddText(
        frame,
        "KILLS " + std::to_string(totalKills),
        Place(layout, 390.0F, 232.0F, 500.0F, 36.0F),
        22.0F * layout.scale,
        kText,
        UiTextAlignment::Center);

    float rowY = 302.0F;
    for (const CampaignRoomStats& room : snapshot.campaignRooms) {
        if (!room.visited || rowY > 520.0F) {
            continue;
        }
        AddText(
            frame,
            room.levelName + "   " + std::to_string(room.kills) + " KILLS",
            Place(layout, 390.0F, rowY, 500.0F, 28.0F),
            17.0F * layout.scale,
            kMutedText,
            UiTextAlignment::Center);
        rowY += 34.0F;
    }
    AddMenuItems(frame, snapshot, layout, {"MAIN MENU"});
}

} // namespace

ObjectFpsUiFrame ObjectFpsUi::Build(
    const GameSessionSnapshot& snapshot,
    const UiRect viewport) {
    ObjectFpsUiFrame frame;
    const std::optional<UiLayout> layout = MakeLayout(viewport);
    if (!layout.has_value()) {
        return frame;
    }

    switch (snapshot.screen) {
    case GameScreen::MainMenu:
        AddMainMenu(frame, snapshot, *layout);
        break;
    case GameScreen::Controls:
        AddControls(frame, snapshot, *layout);
        break;
    case GameScreen::Playing:
        AddHud(frame, snapshot, *layout);
        break;
    case GameScreen::Paused:
        AddPause(frame, snapshot, *layout);
        break;
    case GameScreen::Results:
        AddResults(frame, snapshot, *layout);
        break;
    }
    return frame;
}

std::optional<std::size_t> ObjectFpsUi::HitTest(
    const GameScreen screen,
    const float x,
    const float y,
    const UiRect viewport) noexcept {
    const std::optional<UiLayout> layout = MakeLayout(viewport);
    if (!layout.has_value()) {
        return std::nullopt;
    }

    const std::vector<UiRect> rects = MenuItemRects(screen, *layout);
    for (std::size_t index = 0; index < rects.size(); ++index) {
        if (Contains(rects[index], x, y)) {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace fps
