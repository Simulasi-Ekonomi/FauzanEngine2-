#include "Systems/WorldAuthoring.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    WorldAuthoringConfig config{}; config.side = 32; config.seed = 0xA11CE123ULL; config.waterPermille = 0; config.forestPermille = 1000; config.treePermille = 120; config.treesBlockNavigation = true;
    WorldAuthoring world; if (!world.Generate(config) || world.TreeCount() == 0) return 1;
    WorldAuthoring repeat; if (!repeat.Generate(config) || world.DeterministicState() != repeat.DeterministicState()) return 1;
    GridCell buildCell{}; bool found = false; for (uint16_t z = 0; z < config.side && !found; ++z) for (uint16_t x = 0; x < config.side && !found; ++x) if (!world.Navigation().IsBlocked({x, z})) { buildCell = {x, z}; found = true; }
    if (!found || !world.PlaceBuilding({100, buildCell.x, buildCell.z, 1, 1}) || !world.IsOccupied(buildCell) || !world.Navigation().IsBlocked(buildCell)) return 1;
    if (world.PlaceBuilding({101, buildCell.x, buildCell.z, 1, 1}) || world.LastError() != WorldAuthoringError::Occupied) return 1;
    SceneWorld scene; if (!world.BindScene(scene) || world.BoundEntityCount() != world.TreeCount() + world.BuildingCount()) return 1;
    GridCell extra{}; bool extraFound = false; for (uint16_t z = 0; z < config.side && !extraFound; ++z) for (uint16_t x = 0; x < config.side && !extraFound; ++x) if (!world.Navigation().IsBlocked({x, z})) { extra = {x, z}; extraFound = true; }
    if (!extraFound || !world.PlaceBuilding({102, extra.x, extra.z, 1, 1}) || world.BoundEntityCount() != world.TreeCount() + world.BuildingCount()) return 1;
    const uint64_t state = world.DeterministicState(); const auto bytes = world.Serialize(); WorldAuthoring restored; if (!restored.Deserialize(bytes) || restored.DeterministicState() != state || restored.Deserialize(std::span<const uint8_t>(bytes.data(), bytes.size() - 1)) || restored.LastError() != WorldAuthoringError::CorruptPersistence) return 1;
    GridCell altered{}; bool alteredFound = false; for (uint16_t z = 0; z < config.side && !alteredFound; ++z) for (uint16_t x = 0; x < config.side && !alteredFound; ++x) if (!world.Navigation().IsBlocked({x, z})) { altered = {x, z}; alteredFound = true; }
    std::vector<uint8_t> alteredBytes = bytes; const size_t biomeOffset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(WorldAuthoringConfig) + sizeof(uint32_t) + static_cast<size_t>(altered.z) * config.side + altered.x; if (!alteredFound || biomeOffset >= alteredBytes.size()) return 1; alteredBytes[biomeOffset] = static_cast<uint8_t>(WorldBiome::Water); WorldAuthoring alteredWorld; if (!alteredWorld.Deserialize(alteredBytes) || alteredWorld.BiomeAt(altered) != WorldBiome::Water || !alteredWorld.Navigation().IsBlocked(altered)) return 1;
    std::printf("WORLD_AUTHORING_SMOKE_OK side=32 trees=%u buildings=1 navigationRebuild=1 scene=%u state=%llu\n", world.TreeCount(), world.BoundEntityCount(), static_cast<unsigned long long>(state));
    return 0;
}
