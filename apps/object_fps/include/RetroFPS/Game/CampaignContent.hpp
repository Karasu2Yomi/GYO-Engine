#pragma once

#include "RetroFPS/Data/GameData.hpp"
#include "RetroFPS/World/GridMap.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fps {

struct CampaignStageContent final {
    LevelDefinition definition;
    GridMap map;
};

struct CampaignContentBuildResult;

// Immutable, engine-ready game content. Asset acquisition and decoding happen
// before this value is constructed; runtime simulation only sees validated
// game definitions and maps.
class CampaignContent final {
public:
    [[nodiscard]] static CampaignContentBuildResult Build(
        GameDataCatalog catalog,
        std::vector<GridMap> orderedMaps);

    [[nodiscard]] const GameDataCatalog& Data() const noexcept { return data_; }
    [[nodiscard]] std::span<const CampaignStageContent> Stages() const noexcept {
        return stages_;
    }
    [[nodiscard]] const CampaignStageContent* FindStage(
        std::string_view levelId) const noexcept;

private:
    GameDataCatalog data_;
    std::vector<CampaignStageContent> stages_;
};

struct CampaignContentBuildResult final {
    std::optional<CampaignContent> content;
    std::string error;

    [[nodiscard]] bool Succeeded() const noexcept { return content.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return Succeeded(); }
};

} // namespace fps
