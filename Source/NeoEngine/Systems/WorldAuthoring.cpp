#include "WorldAuthoring.h"

#include <algorithm>
#include <cstring>

namespace NeoEngine {
namespace {
constexpr uint32_t kMagic = 0x574F524CU, kVersion = 1U;
template <typename T> void Put(std::vector<uint8_t>& out, const T& value) { const auto* p = reinterpret_cast<const uint8_t*>(&value); out.insert(out.end(), p, p + sizeof(T)); }
template <typename T> bool Get(std::span<const uint8_t> input, size_t& at, T& value) { if (at > input.size() || input.size() - at < sizeof(T)) return false; std::memcpy(&value, input.data() + at, sizeof(T)); at += sizeof(T); return true; }
uint64_t Fnv(std::span<const uint8_t> bytes) { uint64_t value = 1469598103934665603ULL; for (uint8_t byte : bytes) { value ^= byte; value *= 1099511628211ULL; } return value; }
}
bool WorldAuthoring::Fail(WorldAuthoringError error) { lastError_ = error; return false; }
uint32_t WorldAuthoring::Index(GridCell cell) const { return static_cast<uint32_t>(cell.z) * config_.side + cell.x; }
bool WorldAuthoring::Valid(GridCell cell) const { return generated_ && cell.x < config_.side && cell.z < config_.side; }
uint64_t WorldAuthoring::HashCell(uint16_t x, uint16_t z) const { uint64_t v = config_.seed ^ (static_cast<uint64_t>(x) << 32U) ^ static_cast<uint64_t>(z); v ^= v >> 30U; v *= 0xBF58476D1CE4E5B9ULL; v ^= v >> 27U; v *= 0x94D049BB133111EBULL; return v ^ (v >> 31U); }
bool WorldAuthoring::Generate(const WorldAuthoringConfig& config) {
    if (config.side < kMinSide || config.side > kMaxSide || config.seed == 0 || config.waterPermille > 800 || config.forestPermille > 1000 || config.treePermille > 1000 || static_cast<uint32_t>(config.waterPermille) + config.forestPermille > 1000) return Fail(WorldAuthoringError::InvalidConfiguration);
    config_ = config; generated_ = true; scene_ = nullptr; entities_.clear(); biomes_.assign(static_cast<size_t>(config.side) * config.side, WorldBiome::Meadow); occupied_.assign(biomes_.size(), 0); trees_.clear(); buildings_.clear(); if (!navigation_.Initialize(config.side)) return Fail(WorldAuthoringError::InvalidConfiguration);
    for (uint16_t z = 0; z < config.side; ++z) for (uint16_t x = 0; x < config.side; ++x) { const GridCell cell{x, z}; const uint16_t biome = static_cast<uint16_t>(HashCell(x, z) % 1000U); WorldBiome value = biome < config.waterPermille ? WorldBiome::Water : biome < static_cast<uint32_t>(config.waterPermille) + config.forestPermille ? WorldBiome::Forest : biome > 920 ? WorldBiome::Stone : WorldBiome::Meadow; biomes_[Index(cell)] = value; if (value == WorldBiome::Water) navigation_.SetBlocked(cell, true); if (value == WorldBiome::Forest && (HashCell(static_cast<uint16_t>(x + 101), static_cast<uint16_t>(z + 211)) % 1000U) < config.treePermille && trees_.size() < kMaxTrees) { trees_.push_back(cell); if (config.treesBlockNavigation) navigation_.SetBlocked(cell, true); } }
    lastError_ = WorldAuthoringError::None; return true;
}
WorldBiome WorldAuthoring::BiomeAt(GridCell cell) const { return Valid(cell) ? biomes_[Index(cell)] : WorldBiome::Water; }
bool WorldAuthoring::IsOccupied(GridCell cell) const { return !Valid(cell) || occupied_[Index(cell)] != 0; }
bool WorldAuthoring::MarkBuilding(const WorldBuildingPlacement& p, bool blocked) { for (uint16_t dz = 0; dz < p.depth; ++dz) for (uint16_t dx = 0; dx < p.width; ++dx) { const GridCell cell{static_cast<uint16_t>(p.x + dx), static_cast<uint16_t>(p.z + dz)}; occupied_[Index(cell)] = blocked ? 1 : 0; if (!navigation_.SetBlocked(cell, blocked)) return false; } return true; }
bool WorldAuthoring::RebuildNavigation() {
    GridNavigation rebuilt; if (!rebuilt.Initialize(config_.side)) return false;
    for (uint16_t z = 0; z < config_.side; ++z) for (uint16_t x = 0; x < config_.side; ++x) if (biomes_[static_cast<size_t>(z) * config_.side + x] == WorldBiome::Water && !rebuilt.SetBlocked({x, z}, true)) return false;
    if (config_.treesBlockNavigation) for (const GridCell tree : trees_) if (!rebuilt.SetBlocked(tree, true)) return false;
    for (const WorldBuildingPlacement& building : buildings_) for (uint16_t dz = 0; dz < building.depth; ++dz) for (uint16_t dx = 0; dx < building.width; ++dx) if (!rebuilt.SetBlocked({static_cast<uint16_t>(building.x + dx), static_cast<uint16_t>(building.z + dz)}, true)) return false;
    navigation_ = std::move(rebuilt); return true;
}
bool WorldAuthoring::PlaceBuilding(WorldBuildingPlacement p) {
    if (!generated_) return Fail(WorldAuthoringError::NotGenerated); if (p.buildingDefinitionId == 0 || p.width == 0 || p.depth == 0 || p.width > 16 || p.depth > 16 || buildings_.size() >= kMaxBuildings) return Fail(WorldAuthoringError::InvalidConfiguration); if (p.x >= config_.side || p.z >= config_.side || static_cast<uint32_t>(p.x) + p.width > config_.side || static_cast<uint32_t>(p.z) + p.depth > config_.side) return Fail(WorldAuthoringError::OutOfBounds);
    for (uint16_t dz = 0; dz < p.depth; ++dz) for (uint16_t dx = 0; dx < p.width; ++dx) { const GridCell cell{static_cast<uint16_t>(p.x + dx), static_cast<uint16_t>(p.z + dz)}; if (BiomeAt(cell) == WorldBiome::Water) return Fail(WorldAuthoringError::Water); if (IsOccupied(cell)) return Fail(WorldAuthoringError::Occupied); if (config_.treesBlockNavigation && std::find(trees_.begin(), trees_.end(), cell) != trees_.end()) return Fail(WorldAuthoringError::TreeBlocked); }
    if (!MarkBuilding(p, true)) return Fail(WorldAuthoringError::OutOfBounds);
    if (scene_ != nullptr) {
        SceneEntity entity{};
        if (!scene_->Create(entity) || !scene_->SetTransform(entity, {static_cast<float>(p.x), 0.0F, static_cast<float>(p.z), 0, 0, 0, 1, 1, 1})) {
            MarkBuilding(p, false);
            return Fail(WorldAuthoringError::SceneSyncFailed);
        }
        entities_.push_back(entity);
    }
    buildings_.push_back(p); lastError_ = WorldAuthoringError::None; return true;
}
bool WorldAuthoring::BindScene(SceneWorld& scene) {
    if (!generated_ || scene_ != nullptr) return Fail(generated_ ? WorldAuthoringError::SceneSyncFailed : WorldAuthoringError::NotGenerated); const size_t needed = trees_.size() + buildings_.size(); if (needed > SceneWorld::kCapacity - scene.AliveCount()) return Fail(WorldAuthoringError::SceneCapacity); std::vector<SceneEntity> staged; staged.reserve(needed); auto add = [&](float x, float z) { SceneEntity entity{}; return scene.Create(entity) && scene.SetTransform(entity, {x, 0.0F, z, 0, 0, 0, 1, 1, 1}) ? (staged.push_back(entity), true) : false; }; for (const GridCell cell : trees_) if (!add(static_cast<float>(cell.x), static_cast<float>(cell.z))) { for (SceneEntity entity : staged) scene.Destroy(entity); return Fail(WorldAuthoringError::SceneCapacity); } for (const auto& building : buildings_) if (!add(static_cast<float>(building.x), static_cast<float>(building.z))) { for (SceneEntity entity : staged) scene.Destroy(entity); return Fail(WorldAuthoringError::SceneCapacity); } scene_ = &scene; entities_ = std::move(staged); lastError_ = WorldAuthoringError::None; return true;
}
std::vector<uint8_t> WorldAuthoring::Serialize() const { std::vector<uint8_t> out; Put(out, kMagic); Put(out, kVersion); Put(out, config_); const uint32_t biomeCount = static_cast<uint32_t>(biomes_.size()); Put(out, biomeCount); for (WorldBiome biome : biomes_) Put(out, biome); const uint32_t treeCount = static_cast<uint32_t>(trees_.size()); Put(out, treeCount); for (GridCell tree : trees_) Put(out, tree); const uint32_t buildingCount = static_cast<uint32_t>(buildings_.size()); Put(out, buildingCount); for (const auto& building : buildings_) Put(out, building); return out; }
bool WorldAuthoring::Deserialize(std::span<const uint8_t> bytes) { size_t at = 0; uint32_t magic = 0, version = 0; WorldAuthoringConfig config{}; if (!Get(bytes, at, magic) || !Get(bytes, at, version) || !Get(bytes, at, config) || magic != kMagic || version != kVersion) return Fail(WorldAuthoringError::CorruptPersistence); WorldAuthoring restored; if (!restored.Generate(config)) return Fail(WorldAuthoringError::CorruptPersistence); uint32_t biomeCount = 0; if (!Get(bytes, at, biomeCount) || biomeCount != restored.biomes_.size()) return Fail(WorldAuthoringError::CorruptPersistence); for (auto& biome : restored.biomes_) if (!Get(bytes, at, biome) || biome > WorldBiome::Water) return Fail(WorldAuthoringError::CorruptPersistence); uint32_t treeCount = 0; if (!Get(bytes, at, treeCount) || treeCount > kMaxTrees) return Fail(WorldAuthoringError::CorruptPersistence); restored.trees_.clear(); std::vector<uint8_t> treeCells(restored.biomes_.size(), 0U); for (uint32_t i = 0; i < treeCount; ++i) { GridCell tree{}; if (!Get(bytes, at, tree) || tree.x >= config.side || tree.z >= config.side || restored.biomes_[restored.Index(tree)] != WorldBiome::Forest || treeCells[restored.Index(tree)] != 0U) return Fail(WorldAuthoringError::CorruptPersistence); treeCells[restored.Index(tree)] = 1U; restored.trees_.push_back(tree); } uint32_t buildingCount = 0; if (!Get(bytes, at, buildingCount) || buildingCount > kMaxBuildings) return Fail(WorldAuthoringError::CorruptPersistence); for (uint32_t i = 0; i < buildingCount; ++i) { WorldBuildingPlacement building{}; if (!Get(bytes, at, building) || !restored.PlaceBuilding(building)) return Fail(WorldAuthoringError::CorruptPersistence); } if (at != bytes.size() || !restored.RebuildNavigation()) return Fail(WorldAuthoringError::CorruptPersistence); *this = std::move(restored); lastError_ = WorldAuthoringError::None; return true; }
uint64_t WorldAuthoring::DeterministicState() const { const std::vector<uint8_t> bytes = Serialize(); return Fnv(bytes); }
} // namespace NeoEngine
