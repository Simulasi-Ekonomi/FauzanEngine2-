#include "Runtime/FarmRuntimeSession.h"
#include "Runtime/RuntimeTimeSystem.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/TextureStaging.h"
#include "Systems/AgricultureCurriculum.h"
#include "Systems/CurriculumSystem.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <array>
#include <cstdint>
#include <vector>

namespace {
std::vector<uint8_t> Ppm(uint8_t red, uint8_t green, uint8_t blue) { return {'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', red, green, blue}; }
}

int main() {
    using namespace NeoEngine;
    const std::array<std::string, 16> ids{"tile.empty", "tile.tilled", "tile.growing", "tile.harvestable", "building.farmhouse", "building.barn", "building.silo", "building.market", "building.workshop", "building.townhall", "npc.farmer", "npc.builder", "npc.merchant", "npc.quest", "npc.ranger", "player"};
    AssetRegistry assets;
    for (size_t index = 0U; index < ids.size(); ++index) if (!assets.ImportBytes(ids[index], AssetKind::Texture, {}, Ppm(static_cast<uint8_t>(20U + index), static_cast<uint8_t>(40U + index), static_cast<uint8_t>(60U + index))) || !assets.MarkReady(ids[index])) return 1;
    FarmSystem farm(4U, 4U, 100000);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig worldConfig{};
    worldConfig.worldWidth = 4U;
    worldConfig.worldHeight = 4U;
    worldConfig.npcCount = 1U;
    if (!world.Initialize(farm, trust, "curriculum-session-player", worldConfig) || !world.SetCharacterState({1U, 1U, 2U})) return 2;
    FarmSpriteAssetSet set{ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], ids[6], ids[7], ids[8], ids[9], ids[10], ids[11], ids[12], ids[13], ids[14], ids[15]};
    TextureStagingStore textures;
    SoftwareRenderer renderer;
    InputState input;
    if (!renderer.Initialize(96U, 96U) || !input.Bind("farm_move_up", 1) || !input.Bind("farm_move_down", 2) || !input.Bind("farm_move_left", 3) || !input.Bind("farm_move_right", 4) || !input.Bind("farm_interact", 5)) return 3;
    RuntimeTimeSystem time;
    if (!time.Initialize({60U, 1440U, 360U, 1080U, 1000U, 4000U, 32U})) return 4;
    CurriculumGraph graph;
    CurriculumSystem curriculum;
    if (!BuildAgricultureCurriculum(graph) || !curriculum.Initialize(graph)) return 5;
    if (!farm.Till(0U, 0U) || !farm.Plant(0U, 0U, FarmCrop::Wheat) || !farm.Water(0U, 0U) || !farm.AddAnimal(FarmAnimal::Hen)) return 6;

    FarmRuntimeSession session;
    if (!session.Initialize(farm, world, set, assets, textures, renderer, &time, &curriculum) || !session.Frame(input, 1U)) return 7;
    const FarmRuntimeFrameReceipt first = session.LastFrameReceipt();
    if (first.time.totalGameMinutes != 60U || first.telemetry.simulationTick != 1U || first.curriculum.completedLessons != 3U || session.LastCurriculumEvents().size() != 3U) return 8;
    std::vector<uint8_t> progressCheckpoint;
    uint64_t checkpointRevision = 0U;
    if (!session.SaveProgressCheckpoint(7U, progressCheckpoint) || progressCheckpoint.empty()) return 9;
    if (!time.SetTimeScalePermille(2000U) || !session.Frame(input, 1U) || time.Snapshot().totalGameMinutes != 180U || farm.SimulationTick() != 3U) return 10;
    if (!session.RestoreProgressCheckpoint(progressCheckpoint, checkpointRevision) || checkpointRevision != 7U || time.Snapshot().totalGameMinutes != first.time.totalGameMinutes || farm.SimulationTick() != first.telemetry.simulationTick || curriculum.LastReceipt().completedLessons != first.curriculum.completedLessons) return 11;
    const RuntimeTimeSnapshot preservedTime = time.Snapshot();
    const uint64_t preservedFarmTick = farm.SimulationTick();
    const uint64_t preservedCurriculumRevision = curriculum.LastReceipt().revision;
    std::vector<uint8_t> corruptCheckpoint = progressCheckpoint;
    corruptCheckpoint.back() ^= 0xA5U;
    if (session.RestoreProgressCheckpoint(corruptCheckpoint, checkpointRevision) || session.LastError() != FarmRuntimeSessionError::CheckpointDecodeFailed || time.Snapshot().totalGameMinutes != preservedTime.totalGameMinutes || farm.SimulationTick() != preservedFarmTick || curriculum.LastReceipt().revision != preservedCurriculumRevision) return 12;

    if (!time.SetPaused(true)) return 13;
    const uint64_t pausedFarmTick = farm.SimulationTick();
    if (!session.Frame(input, 1U) || session.LastFrameReceipt().time.totalGameMinutes != 60U || farm.SimulationTick() != pausedFarmTick) return 14;
    if (!time.SetPaused(false) || !time.SetTimeScalePermille(500U)) return 15;
    if (!session.Frame(input, 1U) || farm.SimulationTick() != pausedFarmTick || session.LastFrameReceipt().time.totalGameMinutes != 90U) return 16;
    if (!session.Frame(input, 1U) || farm.SimulationTick() != pausedFarmTick + 1U || session.LastFrameReceipt().time.totalGameMinutes != 120U) return 17;
    return 0;
}
