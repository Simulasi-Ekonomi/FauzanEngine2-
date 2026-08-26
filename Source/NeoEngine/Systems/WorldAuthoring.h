#pragma once

#include "Runtime/SceneWorld.h"
#include "Systems/GridNavigation.h"

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class WorldBiome : uint8_t { Meadow, Forest, Stone, Water };
enum class WorldAuthoringError : uint8_t { None, InvalidConfiguration, NotGenerated, OutOfBounds, Occupied, Water, TreeBlocked, Capacity, SceneCapacity, SceneSyncFailed, CorruptPersistence };
struct WorldAuthoringConfig { uint16_t side = 32; uint64_t seed = 1; uint16_t waterPermille = 80; uint16_t forestPermille = 420; uint16_t treePermille = 260; bool treesBlockNavigation = true; };
struct WorldBuildingPlacement { uint32_t buildingDefinitionId = 0; uint16_t x = 0; uint16_t z = 0; uint16_t width = 0; uint16_t depth = 0; };

class WorldAuthoring {
public:
    static constexpr uint16_t kMinSide = 16, kMaxSide = 128, kMaxTrees = 4096, kMaxBuildings = 256;
    bool Generate(const WorldAuthoringConfig& config);
    bool PlaceBuilding(WorldBuildingPlacement placement);
    bool BindScene(SceneWorld& scene);
    [[nodiscard]] WorldBiome BiomeAt(GridCell cell) const;
    [[nodiscard]] bool IsOccupied(GridCell cell) const;
    [[nodiscard]] const GridNavigation& Navigation() const { return navigation_; }
    [[nodiscard]] uint32_t TreeCount() const { return static_cast<uint32_t>(trees_.size()); }
    [[nodiscard]] uint32_t BuildingCount() const { return static_cast<uint32_t>(buildings_.size()); }
    [[nodiscard]] uint32_t BoundEntityCount() const { return static_cast<uint32_t>(entities_.size()); }
    [[nodiscard]] std::vector<uint8_t> Serialize() const;
    bool Deserialize(std::span<const uint8_t> bytes);
    [[nodiscard]] uint64_t DeterministicState() const;
    [[nodiscard]] WorldAuthoringError LastError() const { return lastError_; }
private:
    bool Valid(GridCell cell) const; uint32_t Index(GridCell cell) const; bool Fail(WorldAuthoringError error); uint64_t HashCell(uint16_t x, uint16_t z) const; bool MarkBuilding(const WorldBuildingPlacement& placement, bool blocked); bool RebuildNavigation();
    WorldAuthoringConfig config_{}; GridNavigation navigation_{}; std::vector<WorldBiome> biomes_; std::vector<uint8_t> occupied_; std::vector<GridCell> trees_; std::vector<WorldBuildingPlacement> buildings_; std::vector<SceneEntity> entities_; SceneWorld* scene_ = nullptr; bool generated_ = false; WorldAuthoringError lastError_ = WorldAuthoringError::None;
};

} // namespace NeoEngine
