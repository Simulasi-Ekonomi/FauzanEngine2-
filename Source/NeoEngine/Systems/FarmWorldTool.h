#pragma once

#include "FarmSystem.h"
#include "Runtime/SceneWorld.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace NeoEngine {

class TrustSafetySystem;

enum class FarmWorldError : uint8_t {
    None,
    InvalidConfiguration,
    NotInitialized,
    InvalidCoordinate,
    Banned,
    BuildingCapacity,
    TileOccupied,
    InvalidBuilding,
    InvalidPermit,
    PolicyDisabled,
    NpcCapacity,
    QuestCapacity,
    InvalidQuest,
    QuestNotReady,
    QuestAlreadyCompleted,
    GovernmentLedgerCapacity,
    InvalidGovernmentEvent,
    SceneCapacity,
    SceneSyncFailed,
    FarmActionRejected,
    CorruptPersistence,
};

enum class FarmBuildingType : uint8_t { Farmhouse, Barn, Silo, Market, Workshop, TownHall };
enum class FarmNpcRole : uint8_t { Farmer, Builder, Merchant, QuestGiver, Ranger };
enum class FarmNpcGoal : uint8_t { Rest, TendCrops, Build, Trade, IssueQuest, Patrol };
enum class FarmQuestObjective : uint8_t { Harvest, Construct, Trade };
enum class FarmGovernmentPolicy : uint8_t { ConstructionPermits, MarketFairness, WaterConservation };

struct FarmWorldConfig {
    uint16_t worldWidth = 20;
    uint16_t worldHeight = 20;
    uint16_t npcCount = 12;
    uint16_t maxBuildings = 128;
    uint16_t maxQuests = 256;
};

struct FarmCharacterState {
    uint16_t x = 0;
    uint16_t z = 0;
    uint16_t level = 1;
};

struct FarmWorldBuilding {
    uint32_t id = 0;
    FarmBuildingType type = FarmBuildingType::Farmhouse;
    uint16_t x = 0;
    uint16_t z = 0;
};

struct FarmWorldNpc {
    uint32_t id = 0;
    FarmNpcRole role = FarmNpcRole::Farmer;
    FarmNpcGoal goal = FarmNpcGoal::Rest;
    uint16_t x = 0;
    uint16_t z = 0;
};

struct FarmWorldQuest {
    uint32_t id = 0;
    uint32_t issuerNpcId = 0;
    FarmQuestObjective objective = FarmQuestObjective::Harvest;
    uint16_t requiredAmount = 0;
    uint16_t progress = 0;
    bool completed = false;
};

struct FarmGovernmentLedgerEvent {
    uint64_t id = 0;
    uint64_t tick = 0;
    int64_t observedFarmCoins = 0;
};

struct FarmWorldSnapshot {
    uint16_t worldWidth = 0;
    uint16_t worldHeight = 0;
    uint32_t buildings = 0;
    uint32_t npcs = 0;
    uint32_t quests = 0;
    uint32_t completedQuests = 0;
    uint32_t unusedPermits = 0;
    uint32_t governmentLedgerEvents = 0;
    uint64_t simulationTick = 0;
    int64_t observedFarmCoins = 0;
    FarmWorldError lastError = FarmWorldError::None;
};

class FarmWorldTool {
public:
    static constexpr uint16_t kMaxNpcs = 64;
    static constexpr uint16_t kMaxBuildings = 256;
    static constexpr uint16_t kMaxQuests = 512;
    static constexpr uint16_t kMaxPermits = 512;
    static constexpr uint16_t kMaxGovernmentLedgerEvents = 1024;

    bool Initialize(FarmSystem& farm, TrustSafetySystem& trustSafety, std::string playerId, const FarmWorldConfig& config = {});
    bool Tick(uint32_t ticks);

    bool SetCharacterState(FarmCharacterState state);
    bool SetGovernmentPolicy(FarmGovernmentPolicy policy, bool enabled);
    bool IssueBuildingPermit(FarmBuildingType type, uint64_t& permitId);
    bool PlaceBuilding(uint64_t permitId, uint16_t x, uint16_t z, uint32_t& buildingId);
    bool RemoveBuilding(uint32_t buildingId);
    bool RecordGovernmentTreasurySnapshot(uint64_t eventId);

    bool PlayerTill(uint16_t x, uint16_t z);
    bool PlayerPlant(uint16_t x, uint16_t z, FarmCrop crop);
    bool PlayerWater(uint16_t x, uint16_t z);
    bool PlayerHarvest(uint16_t x, uint16_t z, uint32_t& harvestedUnits);
    bool PlayerSell(uint64_t saleId, FarmItem item, uint32_t units, int64_t pricePerUnit);
    bool PlayerApplyVerifiedTopUp(const VerifiedTopUpReceipt& receipt);
    bool CompleteQuest(uint32_t questId);

    bool PopulateScene(SceneWorld& scene);
    bool SyncScene();
    [[nodiscard]] FarmWorldSnapshot Snapshot() const;
    [[nodiscard]] const FarmWorldBuilding* Building(uint32_t buildingId) const;
    [[nodiscard]] const FarmWorldNpc* Npc(uint32_t npcId) const;
    [[nodiscard]] const FarmWorldQuest* Quest(uint32_t questId) const;
    [[nodiscard]] const FarmCharacterState& Character() const { return character_; }
    [[nodiscard]] std::span<const FarmWorldBuilding> Buildings() const { return buildings_; }
    [[nodiscard]] std::span<const FarmWorldNpc> Npcs() const { return npcs_; }
    [[nodiscard]] const SceneEntity* CharacterSceneEntity() const { return scene_ == nullptr ? nullptr : &characterSceneEntity_; }
    [[nodiscard]] const SceneEntity* BuildingSceneEntity(uint32_t buildingId) const;
    [[nodiscard]] const SceneEntity* NpcSceneEntity(uint32_t npcId) const;
    [[nodiscard]] bool IsGovernmentPolicyEnabled(FarmGovernmentPolicy policy) const;
    [[nodiscard]] uint64_t DeterministicState() const;
    [[nodiscard]] std::vector<uint8_t> Serialize() const;
    bool Deserialize(std::span<const uint8_t> bytes);
    [[nodiscard]] FarmWorldError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }

private:
    struct Permit {
        uint64_t id = 0;
        FarmBuildingType type = FarmBuildingType::Farmhouse;
        bool consumed = false;
    };

    bool Fail(FarmWorldError error);
    bool CanPlayerAct();
    bool ValidCoordinate(uint16_t x, uint16_t z) const;
    static bool ValidBuildingType(FarmBuildingType type);
    static bool ValidRole(FarmNpcRole role);
    static bool ValidGoal(FarmNpcGoal goal);
    static bool ValidObjective(FarmQuestObjective objective);
    static bool ValidPolicy(FarmGovernmentPolicy policy);
    uint32_t GridIndex(uint16_t x, uint16_t z) const;
    void UpdateNpcBrains();
    void UpdateNpcPosition(FarmWorldNpc& npc);
    void MaybeIssueNpcQuest(FarmWorldNpc& npc);
    bool CreateBoundSceneEntity(float x, float z, SceneEntity& entity);
    bool SetBoundSceneTransform(SceneEntity entity, float x, float z);
    FarmWorldBuilding* FindBuilding(uint32_t buildingId);
    const FarmWorldBuilding* FindBuilding(uint32_t buildingId) const;
    FarmWorldQuest* FindQuest(uint32_t questId);
    const FarmWorldQuest* FindQuest(uint32_t questId) const;

    FarmSystem* farm_ = nullptr;
    TrustSafetySystem* trustSafety_ = nullptr;
    std::string playerId_;
    FarmWorldConfig config_{};
    FarmCharacterState character_{};
    std::vector<uint32_t> buildingGrid_;
    std::vector<FarmWorldBuilding> buildings_;
    std::vector<FarmWorldNpc> npcs_;
    std::vector<FarmWorldQuest> quests_;
    std::vector<Permit> permits_;
    std::vector<FarmGovernmentLedgerEvent> governmentLedger_;
    SceneWorld* scene_ = nullptr;
    SceneEntity characterSceneEntity_{};
    std::vector<SceneEntity> buildingSceneEntities_;
    std::vector<SceneEntity> npcSceneEntities_;
    uint8_t policyMask_ = 0;
    uint32_t nextBuildingId_ = 1;
    uint32_t nextQuestId_ = 1;
    uint64_t nextPermitId_ = 1;
    uint64_t simulationTick_ = 0;
    FarmWorldError lastError_ = FarmWorldError::None;
    bool initialized_ = false;
};

} // namespace NeoEngine
