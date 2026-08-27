#include "Systems/FarmCommerceCheckpoint.h"
#include "Systems/FarmCommerceEntitlementLedger.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>
#include <vector>

namespace {

bool ConfigureFarm(NeoEngine::FarmSystem& farm, NeoEngine::TrustSafetySystem& trust, const char* playerId) {
    farm.SetTrustSafety(&trust, playerId);
    farm.SetReceiptVerifier([](const NeoEngine::VerifiedTopUpReceipt& receipt) { return receipt.authorityPayload == "provider-ok"; });
    return true;
}

bool InitializeWorld(NeoEngine::FarmSystem& farm, NeoEngine::TrustSafetySystem& trust, const char* playerId, NeoEngine::FarmWorldTool& world) {
    NeoEngine::FarmWorldConfig config{};
    config.worldWidth = 4U;
    config.worldHeight = 4U;
    config.npcCount = 2U;
    return ConfigureFarm(farm, trust, playerId) && world.Initialize(farm, trust, playerId, config);
}

} // namespace

int main() {
    using namespace NeoEngine;
    constexpr const char* kPlayer = "commerce-player";
    const auto verifier = [](const FarmProviderReceipt& receipt) { return receipt.authorityPayload == "provider-ok"; };

    FarmSystem farm(4U, 4U, 100);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmCommerceEntitlementLedger ledger;
    FarmCommerceAuditReceipt audit{};
    if (!InitializeWorld(farm, trust, kPlayer, world) || !ledger.Initialize(world, kPlayer, verifier)) return 1;
    const int64_t initialCoins = farm.Coins();
    const FarmProviderReceipt receipt{71U, kPlayer, 25, "provider-ok", false};
    if (!ledger.Apply(receipt, audit) || farm.Coins() != initialCoins + 25 || ledger.AcceptedReceiptCount() != 1U) return 2;
    std::vector<uint8_t> checkpoint;
    if (!FarmCommerceCheckpoint::Save(world, ledger, checkpoint) || checkpoint.empty()) return 3;

    FarmSystem restoredFarm(4U, 4U, 100);
    TrustSafetySystem restoredTrust;
    FarmWorldTool restoredWorld;
    FarmCommerceEntitlementLedger restoredLedger;
    if (!InitializeWorld(restoredFarm, restoredTrust, kPlayer, restoredWorld) || !restoredLedger.Initialize(restoredWorld, kPlayer, verifier) ||
        !FarmCommerceCheckpoint::Load(checkpoint, restoredWorld, restoredLedger) || restoredFarm.Coins() != initialCoins + 25 || restoredLedger.AcceptedReceiptCount() != 1U) return 4;
    const int64_t restoredCoins = restoredFarm.Coins();
    if (restoredLedger.Apply(receipt, audit) || restoredLedger.LastError() != FarmCommerceError::Duplicate || restoredFarm.Coins() != restoredCoins) return 5;
    if (!restoredLedger.Apply({72U, kPlayer, 5, "provider-ok", false}, audit) || restoredFarm.Coins() != restoredCoins + 5 ||
        restoredLedger.AcceptedReceiptCount() != 2U || !restoredLedger.Reconcile(71U, 25, audit)) return 6;

    const std::vector<uint8_t> baselineWorld = restoredWorld.Serialize();
    const std::vector<uint8_t> baselineLedger = restoredLedger.SerializeState();
    std::vector<uint8_t> trailing = checkpoint;
    trailing.push_back(0U);
    if (FarmCommerceCheckpoint::Load(trailing, restoredWorld, restoredLedger) || restoredWorld.Serialize() != baselineWorld ||
        restoredLedger.SerializeState() != baselineLedger) return 7;
    std::vector<uint8_t> corrupt = checkpoint;
    corrupt.back() ^= 0x80U;
    if (FarmCommerceCheckpoint::Load(corrupt, restoredWorld, restoredLedger) || restoredWorld.Serialize() != baselineWorld ||
        restoredLedger.SerializeState() != baselineLedger) return 8;

    FarmCommerceEntitlementLedger wrongPlayerLedger;
    if (!wrongPlayerLedger.Initialize(restoredWorld, "other-commerce-player", verifier) || wrongPlayerLedger.RestoreState(baselineLedger) ||
        wrongPlayerLedger.LastError() != FarmCommerceError::CorruptState || wrongPlayerLedger.AcceptedReceiptCount() != 0U) return 9;
    std::vector<uint8_t> corruptedLedger = baselineLedger;
    corruptedLedger.back() ^= 0x01U;
    if (restoredLedger.RestoreState(corruptedLedger) || restoredLedger.LastError() != FarmCommerceError::CorruptState ||
        restoredWorld.Serialize() != baselineWorld || restoredLedger.SerializeState() != baselineLedger) return 10;

    std::printf("FARM_COMMERCE_CHECKPOINT_SMOKE_OK restored=1 replay_rejected=1 checksum=1 atomic=1\n");
    return 0;
}
