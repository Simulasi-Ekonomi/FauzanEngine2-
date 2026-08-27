#include "Runtime/AtomicSaveFile.h"
#include "Systems/FarmAuthorityCheckpointFile.h"
#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>
#include <filesystem>
#include <vector>

int main() {
    using namespace NeoEngine;
    constexpr const char* kPlayer = "authority-file-player";
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "neo_farm_authority_checkpoint_file_smoke";
    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);

    FarmAuthorityCheckpointFileError fileError = FarmAuthorityCheckpointFileError::None;
    FarmWorldTool emptyWorld;
    FarmAuthoritativeService emptyService;
    if (FarmAuthorityCheckpointFile::Save(root, "empty", emptyWorld, emptyService, fileError) || fileError != FarmAuthorityCheckpointFileError::CheckpointRejected) return 1;

    FarmSystem sourceFarm(4U, 4U, 100);
    TrustSafetySystem sourceTrust;
    FarmWorldTool sourceWorld;
    FarmWorldConfig config{};
    config.worldWidth = 4U;
    config.worldHeight = 4U;
    config.npcCount = 2U;
    FarmAuthoritativeService sourceService;
    const AuthorityCommand command{kPlayer, "authority-file-session", "authority-file-command", "farm.till", 1U, 10U, {2, 0, 2, 0}};
    if (!sourceWorld.Initialize(sourceFarm, sourceTrust, kPlayer, config) || !sourceService.Initialize(sourceWorld, sourceTrust, kPlayer, "authority-file-session") ||
        !sourceService.Submit(command, 10U).Accepted() || !FarmAuthorityCheckpointFile::Save(root, "authority", sourceWorld, sourceService, fileError) || fileError != FarmAuthorityCheckpointFileError::None) return 2;

    FarmSystem restoredFarm(4U, 4U, 100);
    TrustSafetySystem restoredTrust;
    FarmWorldTool restoredWorld;
    FarmAuthoritativeService restoredService;
    if (!restoredWorld.Initialize(restoredFarm, restoredTrust, kPlayer, config) || !restoredService.Initialize(restoredWorld, restoredTrust, kPlayer, "temporary-session") ||
        !FarmAuthorityCheckpointFile::Load(root, "authority", restoredWorld, restoredService, fileError) || fileError != FarmAuthorityCheckpointFileError::None ||
        restoredWorld.DeterministicState() != sourceWorld.DeterministicState() || restoredService.Revision() != 1U || restoredFarm.TileStateAt(2U, 2U) != FarmTileState::Tilled) return 3;
    if (!restoredService.BindSession(kPlayer, "authority-file-rebound")) return 4;
    const AuthorityCommand replay{kPlayer, "authority-file-rebound", "authority-file-command", "farm.till", 1U, 10U, {2, 0, 2, 0}};
    const AuthorityDecision replayDecision = restoredService.Submit(replay, 10U);
    if (!replayDecision.Accepted() || !replayDecision.replayed || replayDecision.authoritativeRevision != 1U || restoredWorld.DeterministicState() != sourceWorld.DeterministicState()) return 5;

    const std::vector<uint8_t> stableWorld = restoredWorld.Serialize();
    const std::vector<uint8_t> stableLedger = restoredService.SerializeAuthorityLedger();
    if (FarmAuthorityCheckpointFile::Load(root, "missing", restoredWorld, restoredService, fileError) || fileError != FarmAuthorityCheckpointFileError::StorageReadRejected ||
        restoredWorld.Serialize() != stableWorld || restoredService.SerializeAuthorityLedger() != stableLedger) return 6;
    if (FarmAuthorityCheckpointFile::Save(root, "../invalid", restoredWorld, restoredService, fileError) || fileError != FarmAuthorityCheckpointFileError::StorageWriteRejected ||
        restoredWorld.Serialize() != stableWorld || restoredService.SerializeAuthorityLedger() != stableLedger) return 7;

    std::vector<uint8_t> corrupted;
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Read(root, "authority", corrupted, storageError) || corrupted.empty()) return 8;
    corrupted.back() ^= 0x80U;
    if (!AtomicSaveFile::Write(root, "authority", corrupted, storageError) ||
        FarmAuthorityCheckpointFile::Load(root, "authority", restoredWorld, restoredService, fileError) || fileError != FarmAuthorityCheckpointFileError::CheckpointRejected ||
        restoredWorld.Serialize() != stableWorld || restoredService.SerializeAuthorityLedger() != stableLedger) return 9;

    std::filesystem::remove_all(root, cleanupError);
    std::printf("FARM_AUTHORITY_CHECKPOINT_FILE_SMOKE_OK file_restore=1 replay=1 fail_closed=1 atomic=1\n");
    return 0;
}
