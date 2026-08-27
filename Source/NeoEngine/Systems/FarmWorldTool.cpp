#include "FarmWorldTool.h"

#include "TrustSafetySystem.h"

#include <algorithm>
#include <array>
#include <limits>

namespace NeoEngine {
namespace {

constexpr uint32_t kMagic = 0x4C4F4F54U; // TOOL
constexpr uint16_t kVersion = 2;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

template <typename T>
void Append(std::vector<uint8_t>& output, T value) {
    for (size_t byte = 0; byte < sizeof(T); ++byte) {
        output.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (byte * 8U)) & 0xFFU));
    }
}

template <typename T>
bool Read(std::span<const uint8_t> input, size_t& offset, T& value) {
    if (offset + sizeof(T) > input.size()) return false;
    uint64_t raw = 0;
    for (size_t byte = 0; byte < sizeof(T); ++byte) {
        raw |= static_cast<uint64_t>(input[offset + byte]) << (byte * 8U);
    }
    value = static_cast<T>(raw);
    offset += sizeof(T);
    return true;
}

void HashU64(uint64_t& hash, uint64_t value) {
    for (uint8_t byte = 0; byte < 8; ++byte) {
        hash ^= static_cast<uint8_t>(value & 0xFFU);
        hash *= kHashPrime;
        value >>= 8U;
    }
}

uint8_t PolicyBit(FarmGovernmentPolicy policy) {
    return static_cast<uint8_t>(1U << static_cast<uint8_t>(policy));
}

} // namespace

bool FarmWorldTool::Initialize(FarmSystem& farm, TrustSafetySystem& trustSafety, std::string playerId, const FarmWorldConfig& config) {
    initialized_ = false;
    farm_ = nullptr;
    trustSafety_ = nullptr;
    playerId_.clear();
    buildingGrid_.clear();
    buildings_.clear();
    npcs_.clear();
    quests_.clear();
    permits_.clear();
    governmentLedger_.clear();
    nextBuildingId_ = 1;
    nextQuestId_ = 1;
    nextPermitId_ = 1;
    simulationTick_ = 0;
    policyMask_ = 0;
    lastError_ = FarmWorldError::None;

    const size_t tiles = static_cast<size_t>(config.worldWidth) * config.worldHeight;
    if (!farm.IsReady() || playerId.empty() || playerId.size() > 96 || config.worldWidth == 0 || config.worldHeight == 0 ||
        tiles > FarmSystem::kMaxTiles || farm.Width() != config.worldWidth || farm.Height() != config.worldHeight ||
        config.npcCount == 0 || config.npcCount > kMaxNpcs || config.maxBuildings == 0 || config.maxBuildings > kMaxBuildings ||
        config.maxQuests == 0 || config.maxQuests > kMaxQuests) {
        return Fail(FarmWorldError::InvalidConfiguration);
    }

    farm_ = &farm;
    trustSafety_ = &trustSafety;
    playerId_ = std::move(playerId);
    config_ = config;
    character_ = {0, 0, 1};
    buildingGrid_.assign(tiles, 0);
    buildings_.reserve(config.maxBuildings);
    npcs_.reserve(config.npcCount);
    quests_.reserve(config.maxQuests);
    permits_.reserve(kMaxPermits);
    governmentLedger_.reserve(kMaxGovernmentLedgerEvents);

    for (uint16_t index = 0; index < config.npcCount; ++index) {
        const FarmNpcRole role = static_cast<FarmNpcRole>(index % 5U);
        npcs_.push_back({
            static_cast<uint32_t>(index + 1U),
            role,
            FarmNpcGoal::Rest,
            static_cast<uint16_t>((static_cast<uint32_t>(index) * 7U + 3U) % config.worldWidth),
            static_cast<uint16_t>((static_cast<uint32_t>(index) * 11U + 5U) % config.worldHeight),
        });
    }
    initialized_ = true;
    return true;
}

bool FarmWorldTool::Tick(uint32_t ticks) {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (ticks == 0 || ticks > std::numeric_limits<uint64_t>::max() - simulationTick_) {
        return Fail(FarmWorldError::InvalidConfiguration);
    }
    if (!farm_->Tick(ticks)) return Fail(FarmWorldError::FarmActionRejected);
    simulationTick_ += ticks;
    UpdateNpcBrains();
    if (scene_ != nullptr && !SyncScene()) return Fail(FarmWorldError::SceneSyncFailed);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::SetCharacterState(FarmCharacterState state) {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (!ValidCoordinate(state.x, state.z) || state.level == 0) return Fail(FarmWorldError::InvalidCoordinate);
    character_ = state;
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::SetGovernmentPolicy(FarmGovernmentPolicy policy, bool enabled) {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (!ValidPolicy(policy)) return Fail(FarmWorldError::InvalidGovernmentEvent);
    const uint8_t bit = PolicyBit(policy);
    policyMask_ = enabled ? static_cast<uint8_t>(policyMask_ | bit) : static_cast<uint8_t>(policyMask_ & ~bit);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::IssueBuildingPermit(FarmBuildingType type, uint64_t& permitId) {
    permitId = 0;
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (!CanPlayerAct()) return false;
    if (!ValidBuildingType(type)) return Fail(FarmWorldError::InvalidBuilding);
    if (!IsGovernmentPolicyEnabled(FarmGovernmentPolicy::ConstructionPermits)) return Fail(FarmWorldError::PolicyDisabled);
    if (permits_.size() >= kMaxPermits || nextPermitId_ == 0) return Fail(FarmWorldError::GovernmentLedgerCapacity);
    permits_.push_back({nextPermitId_, type, false});
    permitId = nextPermitId_++;
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlaceBuilding(uint64_t permitId, uint16_t x, uint16_t z, uint32_t& buildingId) {
    buildingId = 0;
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (!CanPlayerAct()) return false;
    if (!ValidCoordinate(x, z)) return Fail(FarmWorldError::InvalidCoordinate);
    if (buildings_.size() >= config_.maxBuildings) return Fail(FarmWorldError::BuildingCapacity);
    if (buildingGrid_[GridIndex(x, z)] != 0) return Fail(FarmWorldError::TileOccupied);
    const auto permit = std::find_if(permits_.begin(), permits_.end(), [permitId](const Permit& value) { return value.id == permitId; });
    if (permit == permits_.end() || permit->consumed || !ValidBuildingType(permit->type)) return Fail(FarmWorldError::InvalidPermit);
    if (nextBuildingId_ == 0) return Fail(FarmWorldError::BuildingCapacity);
    SceneEntity sceneEntity{};
    if (scene_ != nullptr && !CreateBoundSceneEntity(x, z, sceneEntity)) return Fail(FarmWorldError::SceneCapacity);
    buildings_.push_back({nextBuildingId_, permit->type, x, z});
    buildingGrid_[GridIndex(x, z)] = nextBuildingId_;
    permit->consumed = true;
    if (scene_ != nullptr) buildingSceneEntities_.push_back(sceneEntity);
    buildingId = nextBuildingId_++;
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::RemoveBuilding(uint32_t buildingId) {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (!CanPlayerAct()) return false;
    const auto building = std::find_if(buildings_.begin(), buildings_.end(), [buildingId](const FarmWorldBuilding& value) {
        return value.id == buildingId;
    });
    if (building == buildings_.end()) return Fail(FarmWorldError::InvalidBuilding);
    const size_t buildingIndex = static_cast<size_t>(std::distance(buildings_.begin(), building));
    if (scene_ != nullptr) {
        if (buildingIndex >= buildingSceneEntities_.size() || !scene_->Destroy(buildingSceneEntities_[buildingIndex])) return Fail(FarmWorldError::SceneSyncFailed);
        buildingSceneEntities_.erase(buildingSceneEntities_.begin() + static_cast<std::ptrdiff_t>(buildingIndex));
    }
    buildingGrid_[GridIndex(building->x, building->z)] = 0;
    buildings_.erase(building);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::RecordGovernmentTreasurySnapshot(uint64_t eventId) {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (eventId == 0 || governmentLedger_.size() >= kMaxGovernmentLedgerEvents ||
        std::any_of(governmentLedger_.begin(), governmentLedger_.end(), [eventId](const FarmGovernmentLedgerEvent& event) { return event.id == eventId; })) {
        return Fail(FarmWorldError::InvalidGovernmentEvent);
    }
    governmentLedger_.push_back({eventId, simulationTick_, farm_->Coins()});
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlayerTill(uint16_t x, uint16_t z) {
    if (!CanPlayerAct()) return false;
    if (!farm_->Till(x, z)) return Fail(FarmWorldError::FarmActionRejected);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlayerPlant(uint16_t x, uint16_t z, FarmCrop crop) {
    if (!CanPlayerAct()) return false;
    if (!farm_->Plant(x, z, crop)) return Fail(FarmWorldError::FarmActionRejected);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlayerWater(uint16_t x, uint16_t z) {
    if (!CanPlayerAct()) return false;
    if (!farm_->Water(x, z)) return Fail(FarmWorldError::FarmActionRejected);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlayerHarvest(uint16_t x, uint16_t z, uint32_t& harvestedUnits) {
    harvestedUnits = 0;
    if (!CanPlayerAct()) return false;
    if (!farm_->Harvest(x, z, harvestedUnits)) return Fail(FarmWorldError::FarmActionRejected);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlayerSell(uint64_t saleId, FarmItem item, uint32_t units, int64_t pricePerUnit) {
    if (!CanPlayerAct()) return false;
    if (!farm_->Sell(saleId, item, units, pricePerUnit)) return Fail(FarmWorldError::FarmActionRejected);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PlayerApplyVerifiedTopUp(const VerifiedTopUpReceipt& receipt) {
    if (!CanPlayerAct()) return false;
    if (!farm_->ApplyVerifiedTopUp(receipt)) return Fail(FarmWorldError::FarmActionRejected);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::CompleteQuest(uint32_t questId) {
    if (!CanPlayerAct()) return false;
    FarmWorldQuest* quest = FindQuest(questId);
    if (quest == nullptr) return Fail(FarmWorldError::InvalidQuest);
    if (quest->completed) return Fail(FarmWorldError::QuestAlreadyCompleted);
    uint32_t available = 0;
    switch (quest->objective) {
        case FarmQuestObjective::Harvest:
            available = farm_->ItemCount(FarmItem::WheatProduce) + farm_->ItemCount(FarmItem::CornProduce) + farm_->ItemCount(FarmItem::TomatoProduce);
            break;
        case FarmQuestObjective::Construct:
            available = static_cast<uint32_t>(buildings_.size());
            break;
        case FarmQuestObjective::Trade:
            for (const FarmEvent& event : farm_->RecentEvents()) {
                available += event.type == FarmEventType::Sold ? 1U : 0U;
            }
            break;
    }
    if (available < quest->requiredAmount) return Fail(FarmWorldError::QuestNotReady);
    quest->progress = quest->requiredAmount;
    quest->completed = true;
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::PopulateScene(SceneWorld& scene) {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (scene_ != nullptr) return Fail(FarmWorldError::SceneSyncFailed);
    const uint32_t required = 1U + static_cast<uint32_t>(buildings_.size()) + static_cast<uint32_t>(npcs_.size());
    if (required > SceneWorld::kCapacity - scene.AliveCount()) return Fail(FarmWorldError::SceneCapacity);
    std::vector<SceneEntity> created;
    created.reserve(required);
    auto place = [&scene, &created](float x, float z, SceneEntity& entity) {
        if (!scene.Create(entity) || !scene.SetTransform(entity, {x, 0.0F, z, 0, 0, 0, 1, 1, 1})) return false;
        created.push_back(entity);
        return true;
    };
    SceneEntity characterEntity{};
    std::vector<SceneEntity> buildingEntities;
    std::vector<SceneEntity> npcEntities;
    buildingEntities.reserve(buildings_.size());
    npcEntities.reserve(npcs_.size());
    if (!place(character_.x, character_.z, characterEntity)) return Fail(FarmWorldError::SceneCapacity);
    for (const FarmWorldBuilding& building : buildings_) {
        SceneEntity entity{};
        if (!place(building.x, building.z, entity)) { for (SceneEntity createdEntity : created) scene.Destroy(createdEntity); return Fail(FarmWorldError::SceneCapacity); }
        buildingEntities.push_back(entity);
    }
    for (const FarmWorldNpc& npc : npcs_) {
        SceneEntity entity{};
        if (!place(npc.x, npc.z, entity)) { for (SceneEntity createdEntity : created) scene.Destroy(createdEntity); return Fail(FarmWorldError::SceneCapacity); }
        npcEntities.push_back(entity);
    }
    scene_ = &scene;
    characterSceneEntity_ = characterEntity;
    buildingSceneEntities_ = std::move(buildingEntities);
    npcSceneEntities_ = std::move(npcEntities);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::AdoptTopologyPreservingSceneBinding(const FarmWorldTool& activeWorld) {
    if (!initialized_ || !activeWorld.initialized_ || activeWorld.scene_ == nullptr || config_.worldWidth != activeWorld.config_.worldWidth ||
        config_.worldHeight != activeWorld.config_.worldHeight || config_.npcCount != activeWorld.config_.npcCount ||
        config_.maxBuildings != activeWorld.config_.maxBuildings || config_.maxQuests != activeWorld.config_.maxQuests ||
        buildings_.size() != activeWorld.buildings_.size() || npcs_.size() != activeWorld.npcs_.size() ||
        activeWorld.buildingSceneEntities_.size() != activeWorld.buildings_.size() || activeWorld.npcSceneEntities_.size() != activeWorld.npcs_.size() ||
        activeWorld.scene_->GetTransform(activeWorld.characterSceneEntity_) == nullptr) {
        return Fail(FarmWorldError::SceneSyncFailed);
    }
    for (size_t index = 0; index < buildings_.size(); ++index) {
        if (buildings_[index].id != activeWorld.buildings_[index].id || activeWorld.scene_->GetTransform(activeWorld.buildingSceneEntities_[index]) == nullptr) {
            return Fail(FarmWorldError::SceneSyncFailed);
        }
    }
    for (size_t index = 0; index < npcs_.size(); ++index) {
        if (npcs_[index].id != activeWorld.npcs_[index].id || activeWorld.scene_->GetTransform(activeWorld.npcSceneEntities_[index]) == nullptr) {
            return Fail(FarmWorldError::SceneSyncFailed);
        }
    }
    scene_ = activeWorld.scene_;
    characterSceneEntity_ = activeWorld.characterSceneEntity_;
    buildingSceneEntities_ = activeWorld.buildingSceneEntities_;
    npcSceneEntities_ = activeWorld.npcSceneEntities_;
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::SyncScene() {
    if (!initialized_ || scene_ == nullptr || buildingSceneEntities_.size() != buildings_.size() || npcSceneEntities_.size() != npcs_.size() || !SetBoundSceneTransform(characterSceneEntity_, character_.x, character_.z)) return Fail(FarmWorldError::SceneSyncFailed);
    for (size_t index = 0; index < buildings_.size(); ++index) if (!SetBoundSceneTransform(buildingSceneEntities_[index], buildings_[index].x, buildings_[index].z)) return Fail(FarmWorldError::SceneSyncFailed);
    for (size_t index = 0; index < npcs_.size(); ++index) if (!SetBoundSceneTransform(npcSceneEntities_[index], npcs_[index].x, npcs_[index].z)) return Fail(FarmWorldError::SceneSyncFailed);
    lastError_ = FarmWorldError::None;
    return true;
}

const SceneEntity* FarmWorldTool::BuildingSceneEntity(uint32_t buildingId) const {
    const auto building = std::find_if(buildings_.begin(), buildings_.end(), [buildingId](const FarmWorldBuilding& value) { return value.id == buildingId; });
    if (scene_ == nullptr || building == buildings_.end()) return nullptr;
    const size_t index = static_cast<size_t>(std::distance(buildings_.begin(), building));
    return index < buildingSceneEntities_.size() ? &buildingSceneEntities_[index] : nullptr;
}

const SceneEntity* FarmWorldTool::NpcSceneEntity(uint32_t npcId) const {
    const auto npc = std::find_if(npcs_.begin(), npcs_.end(), [npcId](const FarmWorldNpc& value) { return value.id == npcId; });
    if (scene_ == nullptr || npc == npcs_.end()) return nullptr;
    const size_t index = static_cast<size_t>(std::distance(npcs_.begin(), npc));
    return index < npcSceneEntities_.size() ? &npcSceneEntities_[index] : nullptr;
}

FarmWorldSnapshot FarmWorldTool::Snapshot() const {
    FarmWorldSnapshot snapshot{};
    if (!initialized_) return snapshot;
    snapshot.worldWidth = config_.worldWidth;
    snapshot.worldHeight = config_.worldHeight;
    snapshot.buildings = static_cast<uint32_t>(buildings_.size());
    snapshot.npcs = static_cast<uint32_t>(npcs_.size());
    snapshot.quests = static_cast<uint32_t>(quests_.size());
    snapshot.completedQuests = static_cast<uint32_t>(std::count_if(quests_.begin(), quests_.end(), [](const FarmWorldQuest& quest) { return quest.completed; }));
    snapshot.unusedPermits = static_cast<uint32_t>(std::count_if(permits_.begin(), permits_.end(), [](const Permit& permit) { return !permit.consumed; }));
    snapshot.governmentLedgerEvents = static_cast<uint32_t>(governmentLedger_.size());
    snapshot.simulationTick = simulationTick_;
    snapshot.observedFarmCoins = farm_->Coins();
    snapshot.lastError = lastError_;
    return snapshot;
}

const FarmWorldBuilding* FarmWorldTool::Building(uint32_t buildingId) const { return FindBuilding(buildingId); }

const FarmWorldNpc* FarmWorldTool::Npc(uint32_t npcId) const {
    const auto npc = std::find_if(npcs_.begin(), npcs_.end(), [npcId](const FarmWorldNpc& value) { return value.id == npcId; });
    return npc == npcs_.end() ? nullptr : &*npc;
}

const FarmWorldQuest* FarmWorldTool::Quest(uint32_t questId) const { return FindQuest(questId); }

bool FarmWorldTool::IsGovernmentPolicyEnabled(FarmGovernmentPolicy policy) const {
    return ValidPolicy(policy) && (policyMask_ & PolicyBit(policy)) != 0;
}

uint64_t FarmWorldTool::DeterministicState() const {
    uint64_t hash = kHashOffset;
    HashU64(hash, initialized_ ? 1U : 0U);
    HashU64(hash, config_.worldWidth);
    HashU64(hash, config_.worldHeight);
    HashU64(hash, character_.x);
    HashU64(hash, character_.z);
    HashU64(hash, character_.level);
    HashU64(hash, policyMask_);
    HashU64(hash, simulationTick_);
    HashU64(hash, farm_ ? farm_->Coins() : 0);
    for (const FarmWorldBuilding& building : buildings_) {
        HashU64(hash, building.id); HashU64(hash, static_cast<uint8_t>(building.type)); HashU64(hash, building.x); HashU64(hash, building.z);
    }
    for (const FarmWorldNpc& npc : npcs_) {
        HashU64(hash, npc.id); HashU64(hash, static_cast<uint8_t>(npc.role)); HashU64(hash, static_cast<uint8_t>(npc.goal)); HashU64(hash, npc.x); HashU64(hash, npc.z);
    }
    for (const FarmWorldQuest& quest : quests_) {
        HashU64(hash, quest.id); HashU64(hash, quest.issuerNpcId); HashU64(hash, static_cast<uint8_t>(quest.objective)); HashU64(hash, quest.requiredAmount); HashU64(hash, quest.progress); HashU64(hash, quest.completed ? 1U : 0U);
    }
    for (const Permit& permit : permits_) {
        HashU64(hash, permit.id); HashU64(hash, static_cast<uint8_t>(permit.type)); HashU64(hash, permit.consumed ? 1U : 0U);
    }
    for (const FarmGovernmentLedgerEvent& event : governmentLedger_) {
        HashU64(hash, event.id); HashU64(hash, event.tick); HashU64(hash, static_cast<uint64_t>(event.observedFarmCoins));
    }
    return hash;
}

std::vector<uint8_t> FarmWorldTool::Serialize() const {
    if (!initialized_) return {};
    std::vector<uint8_t> output;
    output.reserve(128 + buildings_.size() * 12 + npcs_.size() * 10 + quests_.size() * 12);
    Append<uint32_t>(output, kMagic);
    Append<uint16_t>(output, kVersion);
    Append<uint16_t>(output, config_.worldWidth);
    Append<uint16_t>(output, config_.worldHeight);
    Append<uint16_t>(output, config_.npcCount);
    Append<uint16_t>(output, config_.maxBuildings);
    Append<uint16_t>(output, config_.maxQuests);
    Append<uint16_t>(output, character_.x); Append<uint16_t>(output, character_.z); Append<uint16_t>(output, character_.level);
    Append<uint8_t>(output, policyMask_); Append<uint64_t>(output, simulationTick_);
    Append<uint32_t>(output, nextBuildingId_); Append<uint32_t>(output, nextQuestId_); Append<uint64_t>(output, nextPermitId_);
    Append<uint32_t>(output, static_cast<uint32_t>(buildings_.size()));
    for (const FarmWorldBuilding& building : buildings_) { Append<uint32_t>(output, building.id); Append<uint8_t>(output, static_cast<uint8_t>(building.type)); Append<uint16_t>(output, building.x); Append<uint16_t>(output, building.z); }
    Append<uint32_t>(output, static_cast<uint32_t>(npcs_.size()));
    for (const FarmWorldNpc& npc : npcs_) { Append<uint32_t>(output, npc.id); Append<uint8_t>(output, static_cast<uint8_t>(npc.role)); Append<uint8_t>(output, static_cast<uint8_t>(npc.goal)); Append<uint16_t>(output, npc.x); Append<uint16_t>(output, npc.z); }
    Append<uint32_t>(output, static_cast<uint32_t>(quests_.size()));
    for (const FarmWorldQuest& quest : quests_) { Append<uint32_t>(output, quest.id); Append<uint32_t>(output, quest.issuerNpcId); Append<uint8_t>(output, static_cast<uint8_t>(quest.objective)); Append<uint16_t>(output, quest.requiredAmount); Append<uint16_t>(output, quest.progress); Append<uint8_t>(output, quest.completed ? 1U : 0U); }
    Append<uint32_t>(output, static_cast<uint32_t>(permits_.size()));
    for (const Permit& permit : permits_) { Append<uint64_t>(output, permit.id); Append<uint8_t>(output, static_cast<uint8_t>(permit.type)); Append<uint8_t>(output, permit.consumed ? 1U : 0U); }
    Append<uint32_t>(output, static_cast<uint32_t>(governmentLedger_.size()));
    for (const FarmGovernmentLedgerEvent& event : governmentLedger_) { Append<uint64_t>(output, event.id); Append<uint64_t>(output, event.tick); Append<int64_t>(output, event.observedFarmCoins); }
    const std::vector<uint8_t> farmBytes = farm_->Serialize();
    Append<uint32_t>(output, static_cast<uint32_t>(farmBytes.size()));
    output.insert(output.end(), farmBytes.begin(), farmBytes.end());
    return output;
}

bool FarmWorldTool::Deserialize(std::span<const uint8_t> bytes) {
    if (!initialized_ || farm_ == nullptr) return Fail(FarmWorldError::NotInitialized);
    size_t offset = 0;
    uint32_t magic = 0; uint16_t version = 0; FarmWorldConfig config{}; uint8_t policyMask = 0; FarmCharacterState character{};
    uint64_t tick = 0; uint32_t nextBuilding = 0; uint32_t nextQuest = 0; uint64_t nextPermit = 0;
    if (!Read(bytes, offset, magic) || !Read(bytes, offset, version) || !Read(bytes, offset, config.worldWidth) || !Read(bytes, offset, config.worldHeight) ||
        !Read(bytes, offset, config.npcCount) || !Read(bytes, offset, config.maxBuildings) || !Read(bytes, offset, config.maxQuests) ||
        !Read(bytes, offset, character.x) || !Read(bytes, offset, character.z) || !Read(bytes, offset, character.level) || !Read(bytes, offset, policyMask) ||
        !Read(bytes, offset, tick) || !Read(bytes, offset, nextBuilding) || !Read(bytes, offset, nextQuest) || !Read(bytes, offset, nextPermit) ||
        magic != kMagic || version != kVersion || config.worldWidth != config_.worldWidth || config.worldHeight != config_.worldHeight ||
        config.npcCount == 0 || config.npcCount > kMaxNpcs || config.maxBuildings == 0 || config.maxBuildings > kMaxBuildings || config.maxQuests == 0 || config.maxQuests > kMaxQuests ||
        character.level == 0 || !ValidCoordinate(character.x, character.z) || nextBuilding == 0 || nextQuest == 0 || nextPermit == 0 || (policyMask & ~0x07U) != 0) {
        return Fail(FarmWorldError::CorruptPersistence);
    }
    std::vector<FarmWorldBuilding> buildings;
    std::vector<FarmWorldNpc> npcs;
    std::vector<FarmWorldQuest> quests;
    std::vector<Permit> permits;
    std::vector<FarmGovernmentLedgerEvent> ledger;
    uint32_t count = 0;
    if (!Read(bytes, offset, count) || count > config.maxBuildings) return Fail(FarmWorldError::CorruptPersistence);
    buildings.reserve(count);
    std::vector<uint32_t> grid(static_cast<size_t>(config.worldWidth) * config.worldHeight, 0);
    for (uint32_t index = 0; index < count; ++index) {
        FarmWorldBuilding building{}; uint8_t type = 0;
        if (!Read(bytes, offset, building.id) || !Read(bytes, offset, type) || !Read(bytes, offset, building.x) || !Read(bytes, offset, building.z) || building.id == 0 || type > static_cast<uint8_t>(FarmBuildingType::TownHall) || !ValidCoordinate(building.x, building.z)) return Fail(FarmWorldError::CorruptPersistence);
        building.type = static_cast<FarmBuildingType>(type);
        const uint32_t gridIndex = GridIndex(building.x, building.z);
        if (grid[gridIndex] != 0) return Fail(FarmWorldError::CorruptPersistence);
        grid[gridIndex] = building.id; buildings.push_back(building);
    }
    if (!Read(bytes, offset, count) || count != config.npcCount) return Fail(FarmWorldError::CorruptPersistence);
    npcs.reserve(count);
    for (uint32_t index = 0; index < count; ++index) {
        FarmWorldNpc npc{}; uint8_t role = 0; uint8_t goal = 0;
        if (!Read(bytes, offset, npc.id) || !Read(bytes, offset, role) || !Read(bytes, offset, goal) || !Read(bytes, offset, npc.x) || !Read(bytes, offset, npc.z) || npc.id == 0 || role > static_cast<uint8_t>(FarmNpcRole::Ranger) || goal > static_cast<uint8_t>(FarmNpcGoal::Patrol) || !ValidCoordinate(npc.x, npc.z)) return Fail(FarmWorldError::CorruptPersistence);
        npc.role = static_cast<FarmNpcRole>(role); npc.goal = static_cast<FarmNpcGoal>(goal); npcs.push_back(npc);
    }
    if (!Read(bytes, offset, count) || count > config.maxQuests) return Fail(FarmWorldError::CorruptPersistence);
    quests.reserve(count);
    for (uint32_t index = 0; index < count; ++index) {
        FarmWorldQuest quest{}; uint8_t objective = 0; uint8_t completed = 0;
        if (!Read(bytes, offset, quest.id) || !Read(bytes, offset, quest.issuerNpcId) || !Read(bytes, offset, objective) || !Read(bytes, offset, quest.requiredAmount) || !Read(bytes, offset, quest.progress) || !Read(bytes, offset, completed) || quest.id == 0 || quest.issuerNpcId == 0 || objective > static_cast<uint8_t>(FarmQuestObjective::Trade) || quest.requiredAmount == 0 || quest.progress > quest.requiredAmount || completed > 1 || (completed != 0 && quest.progress != quest.requiredAmount)) return Fail(FarmWorldError::CorruptPersistence);
        quest.objective = static_cast<FarmQuestObjective>(objective); quest.completed = completed != 0; quests.push_back(quest);
    }
    if (!Read(bytes, offset, count) || count > kMaxPermits) return Fail(FarmWorldError::CorruptPersistence);
    permits.reserve(count);
    for (uint32_t index = 0; index < count; ++index) {
        Permit permit{}; uint8_t type = 0; uint8_t consumed = 0;
        if (!Read(bytes, offset, permit.id) || !Read(bytes, offset, type) || !Read(bytes, offset, consumed) || permit.id == 0 || type > static_cast<uint8_t>(FarmBuildingType::TownHall) || consumed > 1) return Fail(FarmWorldError::CorruptPersistence);
        permit.type = static_cast<FarmBuildingType>(type); permit.consumed = consumed != 0; permits.push_back(permit);
    }
    if (!Read(bytes, offset, count) || count > kMaxGovernmentLedgerEvents) return Fail(FarmWorldError::CorruptPersistence);
    ledger.reserve(count);
    for (uint32_t index = 0; index < count; ++index) {
        FarmGovernmentLedgerEvent event{};
        if (!Read(bytes, offset, event.id) || !Read(bytes, offset, event.tick) || !Read(bytes, offset, event.observedFarmCoins) || event.id == 0 || event.observedFarmCoins < 0) return Fail(FarmWorldError::CorruptPersistence);
        ledger.push_back(event);
    }
    uint32_t farmByteCount = 0;
    if (!Read(bytes, offset, farmByteCount) || farmByteCount == 0 || farmByteCount > bytes.size() - offset || offset + farmByteCount != bytes.size() || !farm_->Deserialize(bytes.subspan(offset, farmByteCount))) {
        return Fail(FarmWorldError::CorruptPersistence);
    }
    config_ = config; character_ = character; policyMask_ = policyMask; simulationTick_ = tick; nextBuildingId_ = nextBuilding; nextQuestId_ = nextQuest; nextPermitId_ = nextPermit;
    buildingGrid_ = std::move(grid); buildings_ = std::move(buildings); npcs_ = std::move(npcs); quests_ = std::move(quests); permits_ = std::move(permits); governmentLedger_ = std::move(ledger);
    lastError_ = FarmWorldError::None;
    return true;
}

bool FarmWorldTool::Fail(FarmWorldError error) { lastError_ = error; return false; }

bool FarmWorldTool::CanPlayerAct() {
    if (!initialized_) return Fail(FarmWorldError::NotInitialized);
    if (trustSafety_->IsBanned(playerId_)) return Fail(FarmWorldError::Banned);
    return true;
}

bool FarmWorldTool::ValidCoordinate(uint16_t x, uint16_t z) const { return initialized_ && x < config_.worldWidth && z < config_.worldHeight; }
bool FarmWorldTool::ValidBuildingType(FarmBuildingType type) { return type <= FarmBuildingType::TownHall; }
bool FarmWorldTool::ValidRole(FarmNpcRole role) { return role <= FarmNpcRole::Ranger; }
bool FarmWorldTool::ValidGoal(FarmNpcGoal goal) { return goal <= FarmNpcGoal::Patrol; }
bool FarmWorldTool::ValidObjective(FarmQuestObjective objective) { return objective <= FarmQuestObjective::Trade; }
bool FarmWorldTool::ValidPolicy(FarmGovernmentPolicy policy) { return policy <= FarmGovernmentPolicy::WaterConservation; }
uint32_t FarmWorldTool::GridIndex(uint16_t x, uint16_t z) const { return static_cast<uint32_t>(z) * config_.worldWidth + x; }
bool FarmWorldTool::CreateBoundSceneEntity(float x, float z, SceneEntity& entity) { return scene_ != nullptr && scene_->Create(entity) && SetBoundSceneTransform(entity, x, z); }
bool FarmWorldTool::SetBoundSceneTransform(SceneEntity entity, float x, float z) { return scene_ != nullptr && scene_->SetTransform(entity, {x, 0.0F, z, 0, 0, 0, 1, 1, 1}); }

void FarmWorldTool::UpdateNpcBrains() {
    const FarmTelemetrySnapshot farmSnapshot = farm_->Snapshot();
    for (FarmWorldNpc& npc : npcs_) {
        switch (npc.role) {
            case FarmNpcRole::Farmer: npc.goal = (farmSnapshot.growingTiles + farmSnapshot.harvestableTiles) > 0 ? FarmNpcGoal::TendCrops : FarmNpcGoal::Rest; break;
            case FarmNpcRole::Builder: npc.goal = buildings_.size() < config_.maxBuildings ? FarmNpcGoal::Build : FarmNpcGoal::Rest; break;
            case FarmNpcRole::Merchant: npc.goal = FarmNpcGoal::Trade; break;
            case FarmNpcRole::QuestGiver: npc.goal = FarmNpcGoal::IssueQuest; MaybeIssueNpcQuest(npc); break;
            case FarmNpcRole::Ranger: npc.goal = FarmNpcGoal::Patrol; break;
        }
        UpdateNpcPosition(npc);
    }
}

void FarmWorldTool::UpdateNpcPosition(FarmWorldNpc& npc) {
    const uint16_t targetX = static_cast<uint16_t>((static_cast<uint64_t>(npc.id) * 17U + simulationTick_ * 3U) % config_.worldWidth);
    const uint16_t targetZ = static_cast<uint16_t>((static_cast<uint64_t>(npc.id) * 29U + simulationTick_ * 5U) % config_.worldHeight);
    if (npc.x < targetX) ++npc.x; else if (npc.x > targetX) --npc.x;
    if (npc.z < targetZ) ++npc.z; else if (npc.z > targetZ) --npc.z;
}

void FarmWorldTool::MaybeIssueNpcQuest(FarmWorldNpc& npc) {
    if (simulationTick_ == 0 || simulationTick_ % 5U != 0 || quests_.size() >= config_.maxQuests || nextQuestId_ == 0) return;
    const uint32_t selector = static_cast<uint32_t>(quests_.size() % 3U);
    quests_.push_back({nextQuestId_++, npc.id, static_cast<FarmQuestObjective>(selector), static_cast<uint16_t>(5U + selector * 5U)});
}

FarmWorldBuilding* FarmWorldTool::FindBuilding(uint32_t buildingId) {
    const auto building = std::find_if(buildings_.begin(), buildings_.end(), [buildingId](const FarmWorldBuilding& value) { return value.id == buildingId; });
    return building == buildings_.end() ? nullptr : &*building;
}

const FarmWorldBuilding* FarmWorldTool::FindBuilding(uint32_t buildingId) const {
    const auto building = std::find_if(buildings_.begin(), buildings_.end(), [buildingId](const FarmWorldBuilding& value) { return value.id == buildingId; });
    return building == buildings_.end() ? nullptr : &*building;
}

FarmWorldQuest* FarmWorldTool::FindQuest(uint32_t questId) {
    const auto quest = std::find_if(quests_.begin(), quests_.end(), [questId](const FarmWorldQuest& value) { return value.id == questId; });
    return quest == quests_.end() ? nullptr : &*quest;
}

const FarmWorldQuest* FarmWorldTool::FindQuest(uint32_t questId) const {
    const auto quest = std::find_if(quests_.begin(), quests_.end(), [questId](const FarmWorldQuest& value) { return value.id == questId; });
    return quest == quests_.end() ? nullptr : &*quest;
}

} // namespace NeoEngine
