#include "Runtime/AtomicSaveFile.h"
#include "Systems/FarmCommerceCheckpointFile.h"
#include "Systems/FarmCommerceEntitlementLedger.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>
#include <filesystem>
#include <vector>

namespace {

bool InitializeWorld(NeoEngine::FarmSystem& farm, NeoEngine::TrustSafetySystem& trust, const char* playerId, NeoEngine::FarmWorldTool& world) {
    farm.SetTrustSafety(&trust, playerId);
    farm.SetReceiptVerifier([](const NeoEngine::VerifiedTopUpReceipt& receipt) { return receipt.authorityPayload == "provider-ok"; });
    NeoEngine::FarmWorldConfig config{};
    config.worldWidth = 4U;
    config.worldHeight = 4U;
    config.npcCount = 2U;
    return world.Initialize(farm, trust, playerId, config);
}

} // namespace

int main() {
    using namespace NeoEngine;
    constexpr const char* kPlayer = "commerce-file-player";
    const auto verifier = [](const FarmProviderReceipt& receipt) { return receipt.authorityPayload == "provider-ok"; };
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "neo_farm_commerce_checkpoint_file_smoke";
    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);

    FarmWorldTool emptyWorld;
    FarmCommerceEntitlementLedger emptyLedger;
    FarmCommerceCheckpointFileError fileError = FarmCommerceCheckpointFileError::None;
    if (FarmCommerceCheckpointFile::Save(root, "empty", emptyWorld, emptyLedger, fileError) || fileError != FarmCommerceCheckpointFileError::CheckpointRejected) return 1;

    FarmSystem sourceFarm(4U, 4U, 100);
    TrustSafetySystem sourceTrust;
    FarmWorldTool sourceWorld;
    FarmCommerceEntitlementLedger sourceLedger;
    FarmCommerceAuditReceipt audit{};
    if (!InitializeWorld(sourceFarm, sourceTrust, kPlayer, sourceWorld) || !sourceLedger.Initialize(sourceWorld, kPlayer, verifier) ||
        !sourceLedger.Apply({91U, kPlayer, 30, "provider-ok", false}, audit) ||
        !FarmCommerceCheckpointFile::Save(root, "commerce", sourceWorld, sourceLedger, fileError) || fileError != FarmCommerceCheckpointFileError::None) return 2;

    FarmSystem restoredFarm(4U, 4U, 100);
    TrustSafetySystem restoredTrust;
    FarmWorldTool restoredWorld;
    FarmCommerceEntitlementLedger restoredLedger;
    if (!InitializeWorld(restoredFarm, restoredTrust, kPlayer, restoredWorld) || !restoredLedger.Initialize(restoredWorld, kPlayer, verifier) ||
        !FarmCommerceCheckpointFile::Load(root, "commerce", restoredWorld, restoredLedger, fileError) || fileError != FarmCommerceCheckpointFileError::None ||
        restoredFarm.Coins() != 130 || restoredLedger.AcceptedReceiptCount() != 1U) return 3;
    const int64_t restoredCoins = restoredFarm.Coins();
    if (restoredLedger.Apply({91U, kPlayer, 30, "provider-ok", false}, audit) || restoredLedger.LastError() != FarmCommerceError::Duplicate ||
        restoredFarm.Coins() != restoredCoins) return 4;
    const std::vector<uint8_t> stableWorld = restoredWorld.Serialize();
    const std::vector<uint8_t> stableLedger = restoredLedger.SerializeState();
    if (FarmCommerceCheckpointFile::Load(root, "missing", restoredWorld, restoredLedger, fileError) || fileError != FarmCommerceCheckpointFileError::StorageReadRejected ||
        restoredWorld.Serialize() != stableWorld || restoredLedger.SerializeState() != stableLedger) return 5;
    if (FarmCommerceCheckpointFile::Save(root, "../invalid", restoredWorld, restoredLedger, fileError) || fileError != FarmCommerceCheckpointFileError::StorageWriteRejected ||
        restoredWorld.Serialize() != stableWorld || restoredLedger.SerializeState() != stableLedger) return 6;

    std::vector<uint8_t> corrupted;
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Read(root, "commerce", corrupted, storageError) || corrupted.empty()) return 7;
    corrupted.back() ^= 0x80U;
    if (!AtomicSaveFile::Write(root, "commerce", corrupted, storageError) ||
        FarmCommerceCheckpointFile::Load(root, "commerce", restoredWorld, restoredLedger, fileError) || fileError != FarmCommerceCheckpointFileError::CheckpointRejected ||
        restoredWorld.Serialize() != stableWorld || restoredLedger.SerializeState() != stableLedger) return 7;

    std::filesystem::remove_all(root, cleanupError);
    std::printf("FARM_COMMERCE_CHECKPOINT_FILE_SMOKE_OK file_restore=1 replay_rejected=1 fail_closed=1 atomic=1\n");
    return 0;
}
