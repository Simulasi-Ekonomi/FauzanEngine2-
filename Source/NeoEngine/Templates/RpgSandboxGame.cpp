#include "RpgSandboxGame.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
namespace {

constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

void HashU64(uint64_t& hash, uint64_t value) {
    for (uint8_t byte = 0; byte < 8; ++byte) {
        hash ^= static_cast<uint8_t>(value & 0xFFU);
        hash *= kHashPrime;
        value >>= 8;
    }
}

} // namespace

bool RpgSandboxGame::Initialize(const RpgSandboxConfig& config) {
    initialized_ = false;
    lastError_ = RpgSandboxError::None;
    worldGrid_.clear();
    players_.clear();
    npcs_.clear();
    quests_.clear();
    monsters_.clear();
    deadMonsterIndices_.clear();
    equipment_.clear();
    nextEquipmentId_ = 1;
    simulationSeconds_ = 0;

    if (config.worldWidth != kRequiredWorldSide || config.worldHeight != kRequiredWorldSide ||
        config.cityCount < 3 || config.cityCount > kMaxCities || config.playerCount == 0 ||
        config.playerCount > kMaxPlayers || config.npcCount == 0 || config.npcCount > kMaxNpcs ||
        config.questCount == 0 || config.questCount > kMaxQuests || config.levelCap == 0 ||
        config.levelCap > kMaxLevels || config.seed == 0) {
        return Fail(RpgSandboxError::InvalidConfig);
    }

    config_ = config;
    randomState_ = config.seed;
    const size_t worldCells = static_cast<size_t>(config.worldWidth) * config.worldHeight;
    worldGrid_.assign(worldCells, 0);
    players_.reserve(config.playerCount);
    npcs_.reserve(config.npcCount);
    quests_.reserve(config.questCount);
    monsters_.assign(static_cast<size_t>(config.levelCap) * kMonstersPerLevel, Monster{});
    deadMonsterIndices_.reserve(monsters_.size());
    equipment_.reserve(std::min<uint32_t>(kMaxEquipment, config.playerCount * 2U));

    for (uint16_t city = 0; city < config.cityCount; ++city) {
        const uint16_t x = static_cast<uint16_t>((static_cast<uint32_t>(city + 1) * config.worldWidth) / (config.cityCount + 1));
        const uint16_t y = static_cast<uint16_t>((static_cast<uint32_t>(city + 1) * 683U) % config.worldHeight);
        for (int16_t offsetY = -1; offsetY <= 1; ++offsetY) {
            for (int16_t offsetX = -1; offsetX <= 1; ++offsetX) {
                const int32_t tileX = static_cast<int32_t>(x) + offsetX;
                const int32_t tileY = static_cast<int32_t>(y) + offsetY;
                if (tileX >= 0 && tileY >= 0 && tileX < config.worldWidth && tileY < config.worldHeight) {
                    worldGrid_[static_cast<size_t>(tileY) * config.worldWidth + static_cast<uint16_t>(tileX)] = static_cast<uint16_t>(city + 1);
                }
            }
        }
    }

    for (uint32_t player = 0; player < config.playerCount; ++player) {
        players_.push_back({
            static_cast<uint16_t>(NextRandom() % config.worldWidth),
            static_cast<uint16_t>(NextRandom() % config.worldHeight),
            static_cast<uint16_t>((player % config.levelCap) + 1),
            {},
        });
    }
    for (uint32_t npc = 0; npc < config.npcCount; ++npc) {
        npcs_.push_back({
            static_cast<uint16_t>(NextRandom() % config.worldWidth),
            static_cast<uint16_t>(NextRandom() % config.worldHeight),
            static_cast<uint16_t>((npc % config.levelCap) + 1),
            static_cast<uint16_t>(npc % config.cityCount),
        });
    }
    for (uint32_t quest = 0; quest < config.questCount; ++quest) {
        quests_.push_back({
            quest + 1,
            static_cast<uint16_t>((quest % config.levelCap) + 1),
            static_cast<uint16_t>(quest % config.cityCount),
        });
    }

    initialized_ = true;
    return true;
}

bool RpgSandboxGame::Advance(uint32_t seconds) {
    if (!initialized_) {
        return Fail(RpgSandboxError::NotInitialized);
    }
    if (seconds > std::numeric_limits<uint64_t>::max() - simulationSeconds_) {
        return Fail(RpgSandboxError::InvalidConfig);
    }
    simulationSeconds_ += seconds;
    size_t writeIndex = 0;
    for (const uint32_t index : deadMonsterIndices_) {
        Monster& monster = monsters_[index];
        if (monster.respawnAtSecond <= simulationSeconds_) {
            monster.alive = true;
            monster.respawnAtSecond = 0;
        } else {
            deadMonsterIndices_[writeIndex++] = index;
        }
    }
    deadMonsterIndices_.resize(writeIndex);
    lastError_ = RpgSandboxError::None;
    return true;
}

bool RpgSandboxGame::DefeatMonster(uint32_t playerId, uint16_t level, uint8_t slot, RpgMonsterDrop& drop) {
    if (!initialized_) return Fail(RpgSandboxError::NotInitialized);
    if (!ValidPlayer(playerId)) return Fail(RpgSandboxError::InvalidPlayer);
    if (!ValidLevel(level)) return Fail(RpgSandboxError::InvalidLevel);
    if (slot >= kMonstersPerLevel) return Fail(RpgSandboxError::InvalidMonsterSlot);

    Monster& monster = monsters_[MonsterIndex(level, slot)];
    if (!monster.alive) return Fail(RpgSandboxError::MonsterNotAlive);
    monster.alive = false;
    monster.respawnAtSecond = simulationSeconds_ + kMonsterRespawnSeconds;
    deadMonsterIndices_.push_back(MonsterIndex(level, slot));

    const uint32_t roll = static_cast<uint32_t>(NextRandom() % 1'000U);
    if (roll < 700U) {
        drop.grade = RpgItemGrade::CommonWhite;
    } else if (roll < 950U) {
        drop.grade = RpgItemGrade::UncommonGreen;
    } else {
        drop.grade = RpgItemGrade::RareBlue;
    }
    drop.source = RpgItemSource::MonsterDrop;
    ++players_[playerId].items[GradeIndex(drop.grade)];
    lastError_ = RpgSandboxError::None;
    return true;
}

bool RpgSandboxGame::GrantBundleItems(uint32_t playerId, RpgItemGrade grade, uint32_t count) {
    if (!initialized_) return Fail(RpgSandboxError::NotInitialized);
    if (!ValidPlayer(playerId)) return Fail(RpgSandboxError::InvalidPlayer);
    if (!ValidGrade(grade) || grade < RpgItemGrade::EpicYellow || count == 0) {
        return Fail(RpgSandboxError::InvalidBundleGrade);
    }
    uint32_t& inventory = players_[playerId].items[GradeIndex(grade)];
    if (count > std::numeric_limits<uint32_t>::max() - inventory) {
        return Fail(RpgSandboxError::ItemCapacity);
    }
    inventory += count;
    lastError_ = RpgSandboxError::None;
    return true;
}

bool RpgSandboxGame::UpgradeItems(uint32_t playerId, RpgItemGrade sourceGrade) {
    if (!initialized_) return Fail(RpgSandboxError::NotInitialized);
    if (!ValidPlayer(playerId)) return Fail(RpgSandboxError::InvalidPlayer);
    const uint32_t cost = UpgradeCost(sourceGrade);
    if (cost == 0) return Fail(RpgSandboxError::UpgradeUnavailable);

    const uint32_t sourceIndex = GradeIndex(sourceGrade);
    const RpgItemGrade targetGrade = NextGrade(sourceGrade);
    uint32_t& source = players_[playerId].items[sourceIndex];
    uint32_t& target = players_[playerId].items[GradeIndex(targetGrade)];
    if (source < cost) return Fail(RpgSandboxError::InsufficientItems);
    if (target == std::numeric_limits<uint32_t>::max()) return Fail(RpgSandboxError::ItemCapacity);
    source -= cost;
    ++target;
    lastError_ = RpgSandboxError::None;
    return true;
}

bool RpgSandboxGame::CreateEquipment(uint32_t playerId, RpgItemGrade grade, uint32_t& equipmentId) {
    equipmentId = 0;
    if (!initialized_) return Fail(RpgSandboxError::NotInitialized);
    if (!ValidPlayer(playerId)) return Fail(RpgSandboxError::InvalidPlayer);
    if (!ValidGrade(grade)) return Fail(RpgSandboxError::InvalidItemGrade);
    if (equipment_.size() >= kMaxEquipment || nextEquipmentId_ == 0) return Fail(RpgSandboxError::ItemCapacity);
    uint32_t& inventory = players_[playerId].items[GradeIndex(grade)];
    if (inventory == 0) return Fail(RpgSandboxError::InsufficientItems);
    --inventory;
    equipment_.push_back({nextEquipmentId_, playerId, grade, 0});
    equipmentId = nextEquipmentId_++;
    lastError_ = RpgSandboxError::None;
    return true;
}

bool RpgSandboxGame::EnhanceEquipment(uint32_t playerId, uint32_t equipmentId, RpgEnhancementResult& result) {
    result = {};
    if (!initialized_) return Fail(RpgSandboxError::NotInitialized);
    Equipment* equipment = FindEquipment(playerId, equipmentId);
    if (equipment == nullptr) return Fail(RpgSandboxError::InvalidEquipment);
    result.attempted = true;
    result.chancePermille = EnhancementChancePermille(equipment->grade, equipment->enhancementLevel);
    result.succeeded = static_cast<uint16_t>(NextRandom() % 1'000U) < result.chancePermille;
    if (result.succeeded && equipment->enhancementLevel < std::numeric_limits<uint16_t>::max()) {
        ++equipment->enhancementLevel;
    }
    result.enhancementLevel = equipment->enhancementLevel;
    lastError_ = RpgSandboxError::None;
    return true;
}

RpgSandboxSnapshot RpgSandboxGame::Snapshot() const {
    RpgSandboxSnapshot snapshot{};
    if (!initialized_) return snapshot;
    snapshot.worldWidth = config_.worldWidth;
    snapshot.worldHeight = config_.worldHeight;
    snapshot.cities = config_.cityCount;
    snapshot.players = static_cast<uint32_t>(players_.size());
    snapshot.npcs = static_cast<uint32_t>(npcs_.size());
    snapshot.quests = static_cast<uint32_t>(quests_.size());
    snapshot.levelCap = config_.levelCap;
    snapshot.monsters = static_cast<uint32_t>(monsters_.size());
    snapshot.liveMonsters = snapshot.monsters - static_cast<uint32_t>(deadMonsterIndices_.size());
    snapshot.simulationSeconds = simulationSeconds_;
    return snapshot;
}

uint32_t RpgSandboxGame::MonsterCapacity(uint16_t level) const {
    return initialized_ && ValidLevel(level) ? kMonstersPerLevel : 0;
}

uint32_t RpgSandboxGame::LiveMonsterCount(uint16_t level) const {
    if (!initialized_ || !ValidLevel(level)) return 0;
    uint32_t count = 0;
    const uint32_t first = MonsterIndex(level, 0);
    for (uint8_t slot = 0; slot < kMonstersPerLevel; ++slot) {
        count += monsters_[first + slot].alive ? 1U : 0U;
    }
    return count;
}

uint32_t RpgSandboxGame::ItemCount(uint32_t playerId, RpgItemGrade grade) const {
    if (!initialized_ || !ValidPlayer(playerId) || !ValidGrade(grade)) return 0;
    return players_[playerId].items[GradeIndex(grade)];
}

uint16_t RpgSandboxGame::EnhancementChancePermille(RpgItemGrade grade, uint16_t enhancementLevel) const {
    if (!ValidGrade(grade)) return 0;
    const uint32_t rankPenalty = GradeIndex(grade) * 80U;
    const uint32_t levelPenalty = static_cast<uint32_t>(enhancementLevel) * 45U;
    const uint32_t penalty = std::min<uint32_t>(875U, rankPenalty + levelPenalty);
    return static_cast<uint16_t>(900U - penalty);
}

uint64_t RpgSandboxGame::DeterministicState() const {
    uint64_t hash = kHashOffset;
    HashU64(hash, initialized_ ? 1U : 0U);
    HashU64(hash, config_.worldWidth);
    HashU64(hash, config_.worldHeight);
    HashU64(hash, config_.cityCount);
    HashU64(hash, players_.size());
    HashU64(hash, npcs_.size());
    HashU64(hash, quests_.size());
    HashU64(hash, monsters_.size());
    HashU64(hash, simulationSeconds_);
    HashU64(hash, randomState_);
    for (const Player& player : players_) {
        HashU64(hash, player.x);
        HashU64(hash, player.y);
        HashU64(hash, player.level);
        for (const uint32_t itemCount : player.items) HashU64(hash, itemCount);
    }
    for (const Monster& monster : monsters_) {
        HashU64(hash, monster.alive ? 1U : 0U);
        HashU64(hash, monster.respawnAtSecond);
    }
    for (const Equipment& item : equipment_) {
        HashU64(hash, item.id);
        HashU64(hash, item.owner);
        HashU64(hash, GradeIndex(item.grade));
        HashU64(hash, item.enhancementLevel);
    }
    return hash;
}

const char* RpgSandboxGame::GradeName(RpgItemGrade grade) {
    switch (grade) {
        case RpgItemGrade::CommonWhite: return "Common (White)";
        case RpgItemGrade::UncommonGreen: return "Uncommon (Green)";
        case RpgItemGrade::RareBlue: return "Rare (Blue)";
        case RpgItemGrade::EpicYellow: return "Epic (Yellow)";
        case RpgItemGrade::LegendaryOrange: return "Legendary (Orange)";
        case RpgItemGrade::MythicRed: return "Mythic (Red)";
    }
    return "Invalid";
}

bool RpgSandboxGame::Fail(RpgSandboxError error) {
    lastError_ = error;
    return false;
}

bool RpgSandboxGame::ValidPlayer(uint32_t playerId) const {
    return playerId < players_.size();
}

bool RpgSandboxGame::ValidLevel(uint16_t level) const {
    return level > 0 && level <= config_.levelCap;
}

bool RpgSandboxGame::ValidGrade(RpgItemGrade grade) {
    return grade <= RpgItemGrade::MythicRed;
}

uint32_t RpgSandboxGame::GradeIndex(RpgItemGrade grade) {
    return static_cast<uint32_t>(grade);
}

uint32_t RpgSandboxGame::UpgradeCost(RpgItemGrade sourceGrade) {
    switch (sourceGrade) {
        case RpgItemGrade::CommonWhite: return 10;
        case RpgItemGrade::UncommonGreen: return 20;
        case RpgItemGrade::RareBlue: return 25;
        case RpgItemGrade::EpicYellow: return 20;
        case RpgItemGrade::LegendaryOrange: return 15;
        case RpgItemGrade::MythicRed: return 0;
    }
    return 0;
}

RpgItemGrade RpgSandboxGame::NextGrade(RpgItemGrade sourceGrade) {
    return static_cast<RpgItemGrade>(GradeIndex(sourceGrade) + 1U);
}

uint64_t RpgSandboxGame::NextRandom() {
    randomState_ ^= randomState_ >> 12U;
    randomState_ ^= randomState_ << 25U;
    randomState_ ^= randomState_ >> 27U;
    return randomState_ * 2685821657736338717ULL;
}

uint32_t RpgSandboxGame::MonsterIndex(uint16_t level, uint8_t slot) const {
    return (static_cast<uint32_t>(level) - 1U) * kMonstersPerLevel + slot;
}

RpgSandboxGame::Equipment* RpgSandboxGame::FindEquipment(uint32_t playerId, uint32_t equipmentId) {
    const auto found = std::find_if(equipment_.begin(), equipment_.end(), [playerId, equipmentId](const Equipment& value) {
        return value.owner == playerId && value.id == equipmentId;
    });
    return found == equipment_.end() ? nullptr : &*found;
}

const RpgSandboxGame::Equipment* RpgSandboxGame::FindEquipment(uint32_t playerId, uint32_t equipmentId) const {
    const auto found = std::find_if(equipment_.begin(), equipment_.end(), [playerId, equipmentId](const Equipment& value) {
        return value.owner == playerId && value.id == equipmentId;
    });
    return found == equipment_.end() ? nullptr : &*found;
}

} // namespace NeoEngine
