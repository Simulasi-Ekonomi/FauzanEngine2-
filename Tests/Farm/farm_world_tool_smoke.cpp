#include "Runtime/SceneWorld.h"
#include "Systems/FarmTelemetryAdapter.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TelemetryOutbox.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>
#include <string>

namespace {

bool CreateHarvest(NeoEngine::FarmWorldTool& tool, uint16_t x, uint16_t z) {
    uint32_t harvested = 0;
    return tool.PlayerTill(x, z) && tool.PlayerPlant(x, z, NeoEngine::FarmCrop::Wheat) && tool.PlayerWater(x, z) &&
           tool.Tick(12) && tool.PlayerHarvest(x, z, harvested) && harvested == 2;
}

} // namespace

int main() {
    const auto fail = [](const char* stage) {
        std::fprintf(stderr, "FARM_WORLD_TOOL_SMOKE_FAIL stage=%s\n", stage);
        return 1;
    };
    constexpr char kPlayerId[] = "farm-tool-player";
    NeoEngine::FarmSystem farm(20, 20, 100);
    NeoEngine::TrustSafetySystem trust;
    farm.SetTrustSafety(&trust, kPlayerId);
    farm.SetReceiptVerifier([](const NeoEngine::VerifiedTopUpReceipt& receipt) { return receipt.authorityPayload == "verified"; });

    NeoEngine::FarmWorldTool tool;
    NeoEngine::FarmWorldConfig config{};
    config.worldWidth = 20;
    config.worldHeight = 20;
    config.npcCount = 10;
    config.maxBuildings = 16;
    config.maxQuests = 32;
    if (!tool.Initialize(farm, trust, kPlayerId, config)) return fail("initialize");

    uint64_t permit = 0;
    if (tool.IssueBuildingPermit(NeoEngine::FarmBuildingType::TownHall, permit) ||
        tool.LastError() != NeoEngine::FarmWorldError::PolicyDisabled ||
        !tool.SetGovernmentPolicy(NeoEngine::FarmGovernmentPolicy::ConstructionPermits, true) ||
        !tool.SetGovernmentPolicy(NeoEngine::FarmGovernmentPolicy::MarketFairness, true) ||
        !tool.IssueBuildingPermit(NeoEngine::FarmBuildingType::TownHall, permit) || permit == 0) {
        return fail("policy-permit");
    }
    uint32_t townHall = 0;
    uint32_t duplicateBuilding = 0;
    if (!tool.PlaceBuilding(permit, 1, 1, townHall) || townHall == 0 || tool.PlaceBuilding(permit, 1, 1, duplicateBuilding)) return fail("town-hall");
    uint64_t farmPermit = 0;
    uint32_t farmhouse = 0;
    if (!tool.IssueBuildingPermit(NeoEngine::FarmBuildingType::Farmhouse, farmPermit) || !tool.PlaceBuilding(farmPermit, 2, 1, farmhouse) ||
        !tool.Building(townHall) || !tool.Building(farmhouse)) {
        return fail("farmhouse");
    }

    if (!tool.SetCharacterState({3, 3, 2}) || !CreateHarvest(tool, 3, 3) || !CreateHarvest(tool, 4, 3) ||
        !tool.PlayerSell(801, NeoEngine::FarmItem::WheatProduce, 2, 4) || !tool.RecordGovernmentTreasurySnapshot(1) ||
        tool.RecordGovernmentTreasurySnapshot(1)) {
        return fail("farm-economy");
    }
    if (!tool.PlayerApplyVerifiedTopUp({91, 25, "verified"})) return fail("top-up");

    if (!tool.Tick(1) || !tool.Quest(1) || tool.Quest(1)->objective != NeoEngine::FarmQuestObjective::Harvest) return fail("npc-tick");
    if (!CreateHarvest(tool, 5, 3) || !CreateHarvest(tool, 6, 3) || !tool.CompleteQuest(1)) return fail("quest-complete");
    const auto snapshot = tool.Snapshot();
    if (snapshot.worldWidth != 20 || snapshot.worldHeight != 20 || snapshot.buildings != 2 || snapshot.npcs != 10 ||
        snapshot.quests != 2 || snapshot.completedQuests != 1 || snapshot.governmentLedgerEvents != 1 || snapshot.observedFarmCoins != 133 ||
        !tool.Npc(4) || tool.Npc(4)->goal != NeoEngine::FarmNpcGoal::IssueQuest) {
        return fail("npc-snapshot");
    }
    NeoEngine::FarmTelemetryAdapter telemetry({"farm-world-source", "farm-world", "neo-test", kPlayerId});
    NeoEngine::TelemetryOutbox outbox;
    std::string envelope;
    if (!telemetry.BuildWorldEnvelope(farm, tool, 1'000, envelope) || envelope.find("\"world\":{\"width\":20") == std::string::npos ||
        !outbox.Enqueue("farm-world-envelope-1", envelope) || outbox.Pending().size() != 1) {
        return fail("world-telemetry");
    }

    NeoEngine::SceneWorld scene;
    if (!tool.PopulateScene(scene) || scene.AliveCount() != 13) return fail("scene");
    const uint64_t beforeSave = tool.DeterministicState();
    const auto saved = tool.Serialize();
    if (saved.empty()) return fail("serialize");

    NeoEngine::FarmSystem restoredFarm(20, 20, 0);
    restoredFarm.SetTrustSafety(&trust, kPlayerId);
    restoredFarm.SetReceiptVerifier([](const NeoEngine::VerifiedTopUpReceipt& receipt) { return receipt.authorityPayload == "verified"; });
    NeoEngine::FarmWorldTool restored;
    if (!restored.Initialize(restoredFarm, trust, kPlayerId, config) || !restored.Deserialize(saved) ||
        restored.DeterministicState() != beforeSave || restored.Snapshot().observedFarmCoins != 133) {
        return fail("deserialize");
    }

    if (!trust.Report(kPlayerId, "tool-fraud-1", NeoEngine::FraudSignal::LedgerMismatch) ||
        !trust.Report(kPlayerId, "tool-fraud-2", NeoEngine::FraudSignal::LedgerMismatch) || !trust.IsBanned(kPlayerId) ||
        restored.PlayerTill(5, 5) || restored.LastError() != NeoEngine::FarmWorldError::Banned) {
        return fail("ban-gate");
    }
    std::printf(
        "FARM_WORLD_TOOL_SMOKE_OK buildings=%u npcs=%u quests=%u scenes=%u coins=%lld bytes=%zu state=%llu\n",
        snapshot.buildings,
        snapshot.npcs,
        snapshot.quests,
        scene.AliveCount(),
        static_cast<long long>(snapshot.observedFarmCoins),
        saved.size(),
        static_cast<unsigned long long>(beforeSave));
    return 0;
}
