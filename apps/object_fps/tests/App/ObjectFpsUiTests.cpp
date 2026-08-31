#include "../TestSupport.hpp"

#include "RetroFPS/App/ObjectFpsUi.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>

namespace fps::tests {
namespace {

constexpr UiRect kViewport{0.0F, 0.0F, 1280.0F, 720.0F};

[[nodiscard]] bool HasText(
    const ObjectFpsUiFrame& frame,
    const std::string_view value) {
    return std::any_of(
        frame.texts.begin(),
        frame.texts.end(),
        [value](const UiTextCommand& command) {
            return command.text.find(value) != std::string::npos;
        });
}

[[nodiscard]] const UiTextCommand* FindText(
    const ObjectFpsUiFrame& frame,
    const std::string_view value) {
    const auto found = std::find_if(
        frame.texts.begin(),
        frame.texts.end(),
        [value](const UiTextCommand& command) {
            return command.text.find(value) != std::string::npos;
        });
    return found == frame.texts.end() ? nullptr : &*found;
}

void TestEveryScreenBuildsCommands(TestContext& context) {
    GameSessionSnapshot snapshot;

    snapshot.screen = GameScreen::MainMenu;
    ObjectFpsUiFrame mainMenu = ObjectFpsUi::Build(snapshot, kViewport);
    context.Expect(
        !mainMenu.quads.empty() && !mainMenu.texts.empty(),
        "ordinary startup MainMenu frame has visible UI commands");
    context.Expect(HasText(mainMenu, "OBJECT FPS"), "main menu owns its title policy");
    context.Expect(HasText(mainMenu, "START GAME"), "main menu owns its actions");
    context.Expect(HasText(mainMenu, "QUIT"), "main menu uses the GameFlow quit action label");

    snapshot.screen = GameScreen::Controls;
    ObjectFpsUiFrame controls = ObjectFpsUi::Build(snapshot, kViewport);
    context.Expect(
        !controls.quads.empty() && HasText(controls, "MOVE FORWARD"),
        "controls screen builds its instructions");
    context.Expect(HasText(controls, "BACK"), "controls screen exposes its back action");

    snapshot.screen = GameScreen::Playing;
    ObjectFpsUiFrame playing = ObjectFpsUi::Build(snapshot, kViewport);
    context.Expect(
        !playing.quads.empty() && HasText(playing, "HP"),
        "playing screen builds a HUD without renderer dependencies");

    snapshot.screen = GameScreen::Paused;
    ObjectFpsUiFrame paused = ObjectFpsUi::Build(snapshot, kViewport);
    context.Expect(
        !paused.quads.empty() && HasText(paused, "PAUSED"),
        "paused screen builds HUD and pause overlay commands");
    context.Expect(HasText(paused, "RESUME"), "pause menu owns its resume action");
    context.Expect(HasText(paused, "QUIT"), "pause menu uses the GameFlow quit action label");
    const UiTextCommand* hudText = FindText(paused, "HP");
    const UiTextCommand* pauseTitle = FindText(paused, "PAUSED");
    const auto overlay = std::find_if(
        paused.quads.begin(),
        paused.quads.end(),
        [](const UiQuadCommand& command) {
            return command.bounds.width == kViewport.width &&
                   command.bounds.height == kViewport.height;
        });
    context.Expect(
        hudText != nullptr && pauseTitle != nullptr &&
            overlay != paused.quads.end() &&
            hudText->drawOrder < overlay->drawOrder &&
            overlay->drawOrder < pauseTitle->drawOrder,
        "pause draw order keeps the HUD below its dim overlay and menu above it");

    snapshot.screen = GameScreen::Results;
    snapshot.campaignOutcome = CampaignOutcome::Completed;
    snapshot.campaignRooms.push_back({"stage_01", "STAGE 01", 3, true});
    ObjectFpsUiFrame results = ObjectFpsUi::Build(snapshot, kViewport);
    context.Expect(
        !results.quads.empty() && HasText(results, "RUN COMPLETE"),
        "results screen builds an outcome summary");
    context.Expect(HasText(results, "3 KILLS"), "results screen reports room stats");
}

void TestHudPolicy(TestContext& context) {
    GameSessionSnapshot snapshot;
    snapshot.screen = GameScreen::Playing;
    snapshot.player = PlayerSnapshot{{}, 1.6F, 0.0F, 0.0F, 72.0F, 100.0F};
    snapshot.weapon.magazineAmmo = 7;
    snapshot.weapon.reserveAmmo = 35;
    snapshot.weapon.reloading = true;
    snapshot.weapon.reloadProgress = 0.5F;
    snapshot.weapon.crosshairExpansion = 6.0F;
    snapshot.activeStage = ActiveStageSnapshot{
        "stage_01", "REACTOR", 0, 4, false};

    const ObjectFpsUiFrame frame = ObjectFpsUi::Build(snapshot, kViewport);
    context.Expect(HasText(frame, "HP 72"), "HUD formats player health");
    context.Expect(HasText(frame, "7 / 35"), "HUD formats magazine and reserve ammo");
    context.Expect(HasText(frame, "RELOADING"), "HUD exposes reload state");
    context.Expect(
        HasText(frame, "STAGE 1 / 4  REACTOR"),
        "HUD converts the zero-based runtime ordinal to player-facing numbering");

    context.Expect(frame.quads.size() >= 4, "HUD emits four crosshair quads");
    if (frame.quads.size() >= 4) {
        const auto firstCrosshair = frame.quads.end() - 4;
        const bool horizontal = firstCrosshair[0].bounds.width >
                                firstCrosshair[0].bounds.height;
        const bool vertical = firstCrosshair[2].bounds.height >
                              firstCrosshair[2].bounds.width;
        context.Expect(horizontal && vertical, "crosshair has horizontal and vertical arms");
    }
}

void TestMenuHitTesting(TestContext& context) {
    context.Expect(
        ObjectFpsUi::HitTest(GameScreen::MainMenu, 640.0F, 366.0F, kViewport) == 0,
        "main-menu first item center maps to Start");
    context.Expect(
        ObjectFpsUi::HitTest(GameScreen::MainMenu, 640.0F, 432.0F, kViewport) == 1,
        "main-menu second item center maps to Controls");
    context.Expect(
        ObjectFpsUi::HitTest(GameScreen::MainMenu, 640.0F, 498.0F, kViewport) == 2,
        "main-menu third item center maps to Exit");
    context.Expect(
        ObjectFpsUi::HitTest(GameScreen::Paused, 640.0F, 326.0F, kViewport) == 0 &&
            ObjectFpsUi::HitTest(GameScreen::Paused, 640.0F, 392.0F, kViewport) == 1 &&
            ObjectFpsUi::HitTest(GameScreen::Paused, 640.0F, 458.0F, kViewport) == 2,
        "pause hit-test preserves GameFlow item order");
    context.Expect(
        ObjectFpsUi::HitTest(GameScreen::Controls, 640.0F, 620.0F, kViewport) == 0 &&
            ObjectFpsUi::HitTest(GameScreen::Results, 640.0F, 618.0F, kViewport) == 0,
        "single-action screens share their visible menu layouts");
    context.Expect(
        !ObjectFpsUi::HitTest(GameScreen::Playing, 640.0F, 360.0F, kViewport),
        "playing HUD has no menu hit target");
    context.Expect(
        !ObjectFpsUi::HitTest(GameScreen::MainMenu, 459.99F, 366.0F, kViewport) &&
            !ObjectFpsUi::HitTest(GameScreen::MainMenu, 820.0F, 366.0F, kViewport),
        "menu rectangles use half-open horizontal edges");
    context.Expect(
        !ObjectFpsUi::HitTest(
            GameScreen::MainMenu,
            std::numeric_limits<float>::quiet_NaN(),
            366.0F,
            kViewport),
        "non-finite pointer coordinates are rejected");

    constexpr UiRect halfViewport{0.0F, 0.0F, 640.0F, 360.0F};
    context.Expect(
        ObjectFpsUi::HitTest(GameScreen::MainMenu, 320.0F, 216.0F, halfViewport) == 1,
        "hit-test and layout scale together with the viewport");
}

void TestInvalidViewport(TestContext& context) {
    GameSessionSnapshot snapshot;
    const ObjectFpsUiFrame frame = ObjectFpsUi::Build(
        snapshot,
        {0.0F, 0.0F, 0.0F, 720.0F});
    context.Expect(
        frame.quads.empty() && frame.texts.empty(),
        "invalid viewport produces no malformed UI commands");
    context.Expect(
        !ObjectFpsUi::HitTest(
            GameScreen::MainMenu,
            640.0F,
            360.0F,
            {0.0F, 0.0F, 1280.0F, -1.0F}),
        "invalid viewport has no hit targets");
}

} // namespace

void RunObjectFpsUiTests(TestContext& context) {
    TestEveryScreenBuildsCommands(context);
    TestHudPolicy(context);
    TestMenuHitTesting(context);
    TestInvalidViewport(context);
}

} // namespace fps::tests
