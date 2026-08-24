#include "Templates/RpgSandboxGame.h"

#include <cstdio>
#include <cstring>

namespace {

bool PopulateAndValidate(NeoEngine::RpgSandboxGame& sandbox, uint64_t& state) {
    if (!sandbox.Initialize()) return false;
    const auto initial = sandbox.Snapshot();
    if (initial.worldWidth != 1024 || initial.worldHeight != 1024 || initial.cities < 3 ||
        initial.players != 10'000 || initial.npcs != 10'000 || initial.quests != 10'000 ||
        initial.levelCap != 1'000 || initial.monsters != 100'000 || initial.liveMonsters != 100'000) {
        return false;
    }
    if (sandbox.MonsterCapacity(1) != 100 || sandbox.MonsterCapacity(1'000) != 100 ||
        sandbox.LiveMonsterCount(1) != 100 || sandbox.LiveMonsterCount(1'000) != 100) {
        return false;
    }

    NeoEngine::RpgMonsterDrop drop{};
    for (uint16_t level = 1; level <= 1'000; ++level) {
        for (uint8_t slot = 0; slot < 100; ++slot) {
            if (!sandbox.DefeatMonster(0, level, slot, drop) || drop.source != NeoEngine::RpgItemSource::MonsterDrop ||
                drop.grade > NeoEngine::RpgItemGrade::RareBlue) {
                return false;
            }
        }
        if (sandbox.LiveMonsterCount(level) != 0 || sandbox.MonsterCapacity(level) != 100) return false;
    }
    if (!sandbox.Advance(4)) return false;
    for (uint16_t level = 1; level <= 1'000; ++level) {
        if (sandbox.LiveMonsterCount(level) != 0) return false;
    }
    if (!sandbox.Advance(1)) return false;
    for (uint16_t level = 1; level <= 1'000; ++level) {
        if (sandbox.LiveMonsterCount(level) != 100 || sandbox.MonsterCapacity(level) != 100) return false;
    }
    if (sandbox.Snapshot().liveMonsters != 100'000) {
        return false;
    }

    const uint32_t whiteBefore = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::CommonWhite);
    const uint32_t greenBefore = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::UncommonGreen);
    if (!sandbox.UpgradeItems(0, NeoEngine::RpgItemGrade::CommonWhite) ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::CommonWhite) != whiteBefore - 10 ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::UncommonGreen) != greenBefore + 1) {
        return false;
    }
    const uint32_t greenBeforeSecond = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::UncommonGreen);
    const uint32_t blueBefore = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::RareBlue);
    if (!sandbox.UpgradeItems(0, NeoEngine::RpgItemGrade::UncommonGreen) ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::UncommonGreen) != greenBeforeSecond - 20 ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::RareBlue) != blueBefore + 1) {
        return false;
    }
    const uint32_t blueBeforeSecond = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::RareBlue);
    const uint32_t yellowBefore = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::EpicYellow);
    if (!sandbox.UpgradeItems(0, NeoEngine::RpgItemGrade::RareBlue) ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::RareBlue) != blueBeforeSecond - 25 ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::EpicYellow) != yellowBefore + 1) {
        return false;
    }
    if (!sandbox.GrantBundleItems(0, NeoEngine::RpgItemGrade::EpicYellow, 20)) return false;
    const uint32_t yellowBeforeUpgrade = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::EpicYellow);
    const uint32_t orangeBefore = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::LegendaryOrange);
    if (!sandbox.UpgradeItems(0, NeoEngine::RpgItemGrade::EpicYellow) ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::EpicYellow) != yellowBeforeUpgrade - 20 ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::LegendaryOrange) != orangeBefore + 1 ||
        !sandbox.GrantBundleItems(0, NeoEngine::RpgItemGrade::LegendaryOrange, 15)) {
        return false;
    }
    const uint32_t orangeBeforeUpgrade = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::LegendaryOrange);
    const uint32_t redBefore = sandbox.ItemCount(0, NeoEngine::RpgItemGrade::MythicRed);
    if (!sandbox.UpgradeItems(0, NeoEngine::RpgItemGrade::LegendaryOrange) ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::LegendaryOrange) != orangeBeforeUpgrade - 15 ||
        sandbox.ItemCount(0, NeoEngine::RpgItemGrade::MythicRed) != redBefore + 1 ||
        sandbox.GrantBundleItems(0, NeoEngine::RpgItemGrade::RareBlue, 1)) {
        return false;
    }
    if (sandbox.ItemCount(0, NeoEngine::RpgItemGrade::MythicRed) == 0) return false;

    uint32_t equipmentId = 0;
    if (!sandbox.CreateEquipment(0, NeoEngine::RpgItemGrade::MythicRed, equipmentId) || equipmentId == 0 ||
        sandbox.EnhancementChancePermille(NeoEngine::RpgItemGrade::RareBlue, 0) <=
            sandbox.EnhancementChancePermille(NeoEngine::RpgItemGrade::RareBlue, 10)) {
        return false;
    }
    NeoEngine::RpgEnhancementResult enhancement{};
    if (!sandbox.EnhanceEquipment(0, equipmentId, enhancement) || !enhancement.attempted || enhancement.chancePermille == 0 ||
        std::strcmp(NeoEngine::RpgSandboxGame::GradeName(NeoEngine::RpgItemGrade::MythicRed), "Mythic (Red)") != 0) {
        return false;
    }
    state = sandbox.DeterministicState();
    return state != 0;
}

} // namespace

int main() {
    NeoEngine::RpgSandboxGame invalid;
    NeoEngine::RpgSandboxConfig invalidConfig{};
    invalidConfig.worldWidth = 1025;
    if (invalid.Initialize(invalidConfig)) return 1;

    NeoEngine::RpgSandboxGame first;
    NeoEngine::RpgSandboxGame second;
    uint64_t firstState = 0;
    uint64_t secondState = 0;
    if (!PopulateAndValidate(first, firstState) || !PopulateAndValidate(second, secondState) || firstState != secondState) {
        return 1;
    }
    const auto snapshot = first.Snapshot();
    std::printf(
        "RPG_SANDBOX_SMOKE_OK grid=%ux%u cities=%u players=%u npcs=%u quests=%u levels=%u monsters=%u hash=%llu\n",
        snapshot.worldWidth,
        snapshot.worldHeight,
        snapshot.cities,
        snapshot.players,
        snapshot.npcs,
        snapshot.quests,
        snapshot.levelCap,
        snapshot.monsters,
        static_cast<unsigned long long>(firstState));
    return 0;
}
