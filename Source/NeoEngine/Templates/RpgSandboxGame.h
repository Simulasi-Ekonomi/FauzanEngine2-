#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class RpgItemGrade : uint8_t {
    CommonWhite = 0,
    UncommonGreen = 1,
    RareBlue = 2,
    EpicYellow = 3,
    LegendaryOrange = 4,
    MythicRed = 5,
};

enum class RpgItemSource : uint8_t { MonsterDrop, Bundle, Upgrade };

enum class RpgSandboxError : uint8_t {
    None,
    InvalidConfig,
    NotInitialized,
    InvalidPlayer,
    InvalidLevel,
    InvalidMonsterSlot,
    MonsterNotAlive,
    InvalidBundleGrade,
    InvalidItemGrade,
    InsufficientItems,
    UpgradeUnavailable,
    ItemCapacity,
    InvalidEquipment,
};

struct RpgSandboxConfig {
    uint16_t worldWidth = 1024;
    uint16_t worldHeight = 1024;
    uint16_t cityCount = 4;
    uint32_t playerCount = 10'000;
    uint32_t npcCount = 10'000;
    uint32_t questCount = 10'000;
    uint16_t levelCap = 1'000;
    uint64_t seed = 0x4E454F5250470001ULL;
};

struct RpgMonsterDrop {
    RpgItemGrade grade = RpgItemGrade::CommonWhite;
    RpgItemSource source = RpgItemSource::MonsterDrop;
};

struct RpgEnhancementResult {
    bool attempted = false;
    bool succeeded = false;
    uint16_t chancePermille = 0;
    uint16_t enhancementLevel = 0;
};

struct RpgSandboxSnapshot {
    uint16_t worldWidth = 0;
    uint16_t worldHeight = 0;
    uint16_t cities = 0;
    uint32_t players = 0;
    uint32_t npcs = 0;
    uint32_t quests = 0;
    uint16_t levelCap = 0;
    uint32_t monsters = 0;
    uint32_t liveMonsters = 0;
    uint64_t simulationSeconds = 0;
};

class RpgSandboxGame {
public:
    static constexpr uint16_t kRequiredWorldSide = 1024;
    static constexpr uint32_t kMaxPlayers = 10'000;
    static constexpr uint32_t kMaxNpcs = 10'000;
    static constexpr uint32_t kMaxQuests = 10'000;
    static constexpr uint16_t kMaxLevels = 1'000;
    static constexpr uint16_t kMaxCities = 64;
    static constexpr uint8_t kMonstersPerLevel = 100;
    static constexpr uint8_t kMonsterRespawnSeconds = 5;
    static constexpr uint32_t kMaxEquipment = 100'000;

    bool Initialize(const RpgSandboxConfig& config = {});
    bool Advance(uint32_t seconds);
    bool DefeatMonster(uint32_t playerId, uint16_t level, uint8_t slot, RpgMonsterDrop& drop);
    bool GrantBundleItems(uint32_t playerId, RpgItemGrade grade, uint32_t count);
    bool UpgradeItems(uint32_t playerId, RpgItemGrade sourceGrade);
    bool CreateEquipment(uint32_t playerId, RpgItemGrade grade, uint32_t& equipmentId);
    bool EnhanceEquipment(uint32_t playerId, uint32_t equipmentId, RpgEnhancementResult& result);

    [[nodiscard]] RpgSandboxSnapshot Snapshot() const;
    [[nodiscard]] uint32_t MonsterCapacity(uint16_t level) const;
    [[nodiscard]] uint32_t LiveMonsterCount(uint16_t level) const;
    [[nodiscard]] uint32_t ItemCount(uint32_t playerId, RpgItemGrade grade) const;
    [[nodiscard]] uint16_t EnhancementChancePermille(RpgItemGrade grade, uint16_t enhancementLevel) const;
    [[nodiscard]] uint64_t DeterministicState() const;
    [[nodiscard]] RpgSandboxError LastError() const { return lastError_; }
    [[nodiscard]] bool IsInitialized() const { return initialized_; }

    static const char* GradeName(RpgItemGrade grade);

private:
    struct Player {
        uint16_t x = 0;
        uint16_t y = 0;
        uint16_t level = 1;
        std::array<uint32_t, 6> items{};
    };

    struct Npc {
        uint16_t x = 0;
        uint16_t y = 0;
        uint16_t level = 1;
        uint16_t city = 0;
    };

    struct Quest {
        uint32_t id = 0;
        uint16_t requiredLevel = 1;
        uint16_t city = 0;
    };

    struct Monster {
        uint64_t respawnAtSecond = 0;
        bool alive = true;
    };

    struct Equipment {
        uint32_t id = 0;
        uint32_t owner = 0;
        RpgItemGrade grade = RpgItemGrade::CommonWhite;
        uint16_t enhancementLevel = 0;
    };

    bool Fail(RpgSandboxError error);
    bool ValidPlayer(uint32_t playerId) const;
    bool ValidLevel(uint16_t level) const;
    static bool ValidGrade(RpgItemGrade grade);
    static uint32_t GradeIndex(RpgItemGrade grade);
    static uint32_t UpgradeCost(RpgItemGrade sourceGrade);
    static RpgItemGrade NextGrade(RpgItemGrade sourceGrade);
    uint64_t NextRandom();
    uint32_t MonsterIndex(uint16_t level, uint8_t slot) const;
    Equipment* FindEquipment(uint32_t playerId, uint32_t equipmentId);
    const Equipment* FindEquipment(uint32_t playerId, uint32_t equipmentId) const;

    RpgSandboxConfig config_{};
    std::vector<uint16_t> worldGrid_;
    std::vector<Player> players_;
    std::vector<Npc> npcs_;
    std::vector<Quest> quests_;
    std::vector<Monster> monsters_;
    std::vector<uint32_t> deadMonsterIndices_;
    std::vector<Equipment> equipment_;
    uint64_t randomState_ = 0;
    uint64_t simulationSeconds_ = 0;
    uint32_t nextEquipmentId_ = 1;
    RpgSandboxError lastError_ = RpgSandboxError::None;
    bool initialized_ = false;
};

} // namespace NeoEngine
