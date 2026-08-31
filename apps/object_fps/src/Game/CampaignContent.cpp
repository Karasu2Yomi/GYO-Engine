#include "RetroFPS/Game/CampaignContent.hpp"

#include <cstddef>
#include <unordered_set>
#include <utility>

namespace fps {

CampaignContentBuildResult CampaignContent::Build(
    GameDataCatalog catalog,
    std::vector<GridMap> orderedMaps) {
    const std::span<const LevelDefinition> definitions =
        catalog.levels.GetDefinitions();
    if (definitions.empty()) {
        return {std::nullopt, "campaign content requires at least one stage"};
    }
    if (orderedMaps.size() != definitions.size()) {
        return {
            std::nullopt,
            "campaign map count must match the data-defined stage count",
        };
    }

    std::unordered_set<LevelDefinitionId> identifiers;
    std::vector<CampaignStageContent> stages;
    stages.reserve(definitions.size());
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        const LevelDefinition& definition = definitions[index];
        if (definition.id.empty() || !definition.mapAssetId.IsValid()) {
            return {
                std::nullopt,
                "campaign stage requires a level ID and a valid map AssetId",
            };
        }
        if (!identifiers.insert(definition.id).second) {
            return {std::nullopt, "campaign stage IDs must be unique"};
        }
        stages.push_back({definition, std::move(orderedMaps[index])});
    }

    CampaignContent content;
    content.data_ = std::move(catalog);
    content.stages_ = std::move(stages);
    return {std::move(content), {}};
}

const CampaignStageContent* CampaignContent::FindStage(
    const std::string_view levelId) const noexcept {
    for (const CampaignStageContent& stage : stages_) {
        if (stage.definition.id == levelId) {
            return &stage;
        }
    }
    return nullptr;
}

} // namespace fps
