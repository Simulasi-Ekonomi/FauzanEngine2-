#include "Systems/FarmAuthorityCheckpoint.h"
#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    FarmSystem farm(20, 20, 100); TrustSafetySystem trust; FarmWorldTool world; FarmWorldConfig config{};
    if (!world.Initialize(farm, trust, "checkpoint-player", config)) return 1;
    FarmAuthoritativeService service;
    if (!service.Initialize(world, trust, "checkpoint-player", "checkpoint-session")) return 1;
    AuthorityCommand till{"checkpoint-player", "checkpoint-session", "checkpoint-cmd", "farm.till", 1, 10, {2, 0, 2, 0}};
    if (!service.Submit(till, 10).Accepted()) return 1;
    std::vector<uint8_t> checkpoint;
    if (!FarmAuthorityCheckpoint::Save(world, service, checkpoint)) return 1;
    FarmSystem restoredFarm(20, 20, 100); TrustSafetySystem restoredTrust; FarmWorldTool restoredWorld; FarmAuthoritativeService restoredService;
    if (!restoredWorld.Initialize(restoredFarm, restoredTrust, "checkpoint-player", config) || !restoredService.Initialize(restoredWorld, restoredTrust, "checkpoint-player", "checkpoint-session") || !FarmAuthorityCheckpoint::Load(checkpoint, restoredWorld, restoredService) || restoredWorld.DeterministicState() != world.DeterministicState() || restoredService.Revision() != 1 || restoredFarm.TileStateAt(2, 2) != FarmTileState::Tilled) return 1;
    if (!restoredService.BindSession("checkpoint-player", "checkpoint-rebound")) return 1;
    const AuthorityCommand replay{"checkpoint-player", "checkpoint-rebound", "checkpoint-cmd", "farm.till", 1, 10, {2, 0, 2, 0}};
    const AuthorityDecision replayDecision = restoredService.Submit(replay, 10);
    if (!replayDecision.Accepted() || !replayDecision.replayed || replayDecision.authoritativeRevision != 1 || restoredWorld.DeterministicState() != world.DeterministicState()) return 1;
    const AuthorityCommand second{"checkpoint-player", "checkpoint-rebound", "checkpoint-cmd-2", "farm.till", 2, 11, {3, 0, 2, 0}};
    const AuthorityDecision secondDecision = restoredService.Submit(second, 11);
    if (!secondDecision.Accepted() || secondDecision.replayed || secondDecision.authoritativeRevision != 2 || restoredFarm.TileStateAt(3, 2) != FarmTileState::Tilled) return 1;

    const uint64_t stateBeforeFailedLoad = restoredWorld.DeterministicState();
    const uint64_t revisionBeforeFailedLoad = restoredService.Revision();
    const uint32_t worldLength = static_cast<uint32_t>(checkpoint[6]) | (static_cast<uint32_t>(checkpoint[7]) << 8U) | (static_cast<uint32_t>(checkpoint[8]) << 16U) | (static_cast<uint32_t>(checkpoint[9]) << 24U);
    std::vector<uint8_t> corruptLedger = checkpoint;
    corruptLedger[14U + worldLength] ^= 0xFFU;
    if (FarmAuthorityCheckpoint::Load(corruptLedger, restoredWorld, restoredService) || restoredWorld.DeterministicState() != stateBeforeFailedLoad || restoredService.Revision() != revisionBeforeFailedLoad) return 1;
    std::vector<uint8_t> truncated = checkpoint;
    truncated.pop_back();
    if (FarmAuthorityCheckpoint::Load(truncated, restoredWorld, restoredService) || restoredWorld.DeterministicState() != stateBeforeFailedLoad || restoredService.Revision() != revisionBeforeFailedLoad) return 1;
    std::printf("FARM_AUTHORITY_CHECKPOINT_SMOKE_OK revision=%llu bytes=%zu replay=1 rollback=1\n", static_cast<unsigned long long>(restoredService.Revision()), checkpoint.size());
    return 0;
}
