#pragma once

#include "Runtime/SceneWorld.h"
#include "Systems/GridNavigation.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace NeoEngine {

enum class AuthoringError : uint8_t { None, InvalidDefinition, Capacity, DuplicateId, MissingReference, InvalidHierarchy, InvalidNarrative, InvalidScene, SceneAlreadyBound, SceneCapacity, SceneSyncFailed, NavigationFailed, CorruptPersistence };
enum class AuthoringItemClass : uint8_t { Resource, Equipment, Consumable, Quest };
enum class AuthoringActorKind : uint8_t { Character, Npc, Monster };
enum class AuthoringBehavior : uint8_t { Idle, Patrol, Wander, ChaseOrigin };
enum class AuthoringSceneObjectKind : uint8_t { Actor, Building };

struct CollisionMaterialDefinition { uint32_t id = 0; uint16_t hardnessPermille = 0; uint16_t frictionPermille = 0; uint16_t restitutionPermille = 0; uint16_t densityPermille = 0; };
struct SkeletalBoneDefinition { uint16_t boneId = 0; int16_t parentIndex = -1; uint16_t lengthMillimeters = 0; };
struct SkeletonDefinition { uint32_t id = 0; std::vector<SkeletalBoneDefinition> bones; };
struct CharacterDefinition { uint32_t id = 0; uint32_t skeletonId = 0; uint32_t materialId = 0; uint16_t health = 0; uint16_t stamina = 0; };
struct BuildingDefinition { uint32_t id = 0; uint32_t materialId = 0; uint16_t footprintWidth = 0; uint16_t footprintDepth = 0; uint16_t health = 0; };
struct ItemDefinition { uint32_t id = 0; AuthoringItemClass itemClass = AuthoringItemClass::Resource; uint32_t materialId = 0; uint16_t maxStack = 0; uint16_t weightGrams = 0; };
struct NarrativeEntryDefinition { uint32_t id = 0; std::string contentKey; std::vector<uint32_t> branchDestinationIds; };
struct ActorDefinition { uint32_t id = 0; AuthoringActorKind kind = AuthoringActorKind::Npc; uint32_t characterId = 0; uint32_t materialId = 0; AuthoringBehavior behavior = AuthoringBehavior::Idle; uint16_t moveTilesPerTick = 0; };
struct ScenePlacementDefinition { AuthoringSceneObjectKind kind = AuthoringSceneObjectKind::Actor; uint32_t definitionId = 0; int16_t x = 0; int16_t z = 0; };
struct SceneDefinition { uint32_t id = 0; std::vector<ScenePlacementDefinition> placements; };
struct ContactMaterialResponse { uint16_t hardnessPermille = 0; uint16_t frictionPermille = 0; uint16_t restitutionPermille = 0; };

class AuthoringCatalog {
public:
    static constexpr uint16_t kMaxMaterials = 64, kMaxSkeletons = 32, kMaxBonesPerSkeleton = 64, kMaxCharacters = 128, kMaxBuildings = 128, kMaxItems = 256, kMaxNarratives = 256, kMaxActors = 256, kMaxScenes = 16, kMaxScenePlacements = 512, kMaxNarrativeBranches = 4;
    bool AddMaterial(CollisionMaterialDefinition definition);
    bool AddSkeleton(SkeletonDefinition definition);
    bool AddCharacter(CharacterDefinition definition);
    bool AddBuilding(BuildingDefinition definition);
    bool AddItem(ItemDefinition definition);
    bool AddNarrative(NarrativeEntryDefinition definition);
    bool AddActor(ActorDefinition definition);
    bool AddScene(SceneDefinition definition);
    bool BindScene(uint32_t sceneId, SceneWorld& scene, int16_t worldHalfExtent = 128);
    bool SetActorGoal(uint32_t actorDefinitionId, GridCell goal, const GridNavigation& navigation);
    bool Tick(uint32_t ticks);
    bool Contact(uint32_t firstMaterialId, uint32_t secondMaterialId, ContactMaterialResponse& response) const;
    [[nodiscard]] std::vector<uint8_t> Serialize() const;
    bool Deserialize(std::span<const uint8_t> bytes);
    [[nodiscard]] uint64_t DeterministicState() const;
    [[nodiscard]] AuthoringError LastError() const { return lastError_; }
    [[nodiscard]] uint32_t BoundEntityCount() const { return static_cast<uint32_t>(bound_.size()); }
    [[nodiscard]] bool IsSceneBound() const { return scene_ != nullptr; }
    [[nodiscard]] const SceneEntity* BoundEntity(uint32_t definitionId) const;
    [[nodiscard]] const SceneEntity* BoundEntity(AuthoringSceneObjectKind kind, uint32_t definitionId) const;
private:
    struct BoundObject { SceneEntity entity{}; AuthoringSceneObjectKind kind = AuthoringSceneObjectKind::Actor; uint32_t definitionId = 0; int16_t x = 0; int16_t z = 0; int8_t patrolDirection = 1; std::vector<GridCell> route; size_t routeIndex = 0; };
    bool Fail(AuthoringError error); bool ValidMaterial(const CollisionMaterialDefinition&) const; bool ValidSkeleton(const SkeletonDefinition&) const; bool ValidKey(const std::string&) const; bool ValidBehavior(AuthoringBehavior) const; bool IsUnique(uint32_t id, const auto& values) const;
    const CollisionMaterialDefinition* Material(uint32_t id) const; const SkeletonDefinition* Skeleton(uint32_t id) const; const CharacterDefinition* Character(uint32_t id) const; const BuildingDefinition* Building(uint32_t id) const; const ActorDefinition* Actor(uint32_t id) const; const SceneDefinition* Scene(uint32_t id) const;
    std::vector<CollisionMaterialDefinition> materials_; std::vector<SkeletonDefinition> skeletons_; std::vector<CharacterDefinition> characters_; std::vector<BuildingDefinition> buildings_; std::vector<ItemDefinition> items_; std::vector<NarrativeEntryDefinition> narratives_; std::vector<ActorDefinition> actors_; std::vector<SceneDefinition> scenes_; std::vector<BoundObject> bound_; SceneWorld* scene_ = nullptr; int16_t worldHalfExtent_ = 0; uint64_t tick_ = 0; AuthoringError lastError_ = AuthoringError::None;
};

} // namespace NeoEngine
