#include "Systems/FarmAuthoritativeSessionHost.h"
#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

using namespace NeoEngine;

namespace {
FarmSessionCommand Till(const char* id, uint64_t sequence, uint64_t tick, uint16_t x, uint16_t z, const char* claimedPlayer = "player-a") {
    return {claimedPlayer, id, "farm.till", sequence, tick,
            {static_cast<uint8_t>(x & 0xFFU), static_cast<uint8_t>(x >> 8U), static_cast<uint8_t>(z & 0xFFU), static_cast<uint8_t>(z >> 8U)}};
}
} // namespace

int main() {
    TrustSafetySystem trust;
    FarmSystem farm(4U, 4U, 100);
    farm.SetTrustSafety(&trust, "player-a");
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.worldWidth = 4U;
    config.worldHeight = 4U;
    config.npcCount = 2U;
    FarmAuthoritativeService authority;
    FarmAuthoritativeSessionHost host;
    uint64_t sessionA = 0U;
    uint64_t sessionB = 0U;
    FarmAuthoritativeCommandReceipt receipt{};
    bool ok = host.Initialize(authority) == false && host.LastError() == FarmAuthoritativeSessionError::InvalidConfiguration;
    ok = ok && world.Initialize(farm, trust, "player-a", config) && authority.Initialize(world, trust, "player-a", "bootstrap-session") && host.Initialize(authority);
    ok = ok && !host.Authenticate({"player-a", "short"}, sessionA) && host.LastError() == FarmAuthoritativeSessionError::InvalidPrincipal && sessionA == 0U;
    ok = ok && host.Authenticate({"player-a", "session-a-0001"}, sessionA) && sessionA != 0U && host.SessionCount() == 1U;
    const std::vector<uint8_t> pristineWorld = world.Serialize();
    ok = ok && !host.Submit(999U, Till("host-cmd-001", 1U, 1U, 0U, 0U), 1U, receipt) && host.LastError() == FarmAuthoritativeSessionError::UnknownSession && world.Serialize() == pristineWorld;
    ok = ok && !host.Submit(sessionA, Till("host-cmd-001", 1U, 1U, 0U, 0U, "player-b"), 1U, receipt) && host.LastError() == FarmAuthoritativeSessionError::SubjectSpoofed && world.Serialize() == pristineWorld;
    ok = ok && host.Submit(sessionA, Till("host-cmd-001", 1U, 1U, 0U, 0U), 1U, receipt) && receipt.version == FarmAuthoritativeCommandReceipt::kVersion && receipt.decision.Accepted() && !receipt.decision.replayed && receipt.snapshot.version == FarmAuthoritativeSnapshotReceipt::kVersion && receipt.snapshot.authoritativeRevision == 1U && receipt.snapshot.stateHash != 0U && !receipt.snapshot.worldBytes.empty() && receipt.delta.baseRevision == 0U && receipt.delta.authoritativeRevision == 1U && receipt.delta.stateChanged && farm.TileStateAt(0U, 0U) == FarmTileState::Tilled;
    const FarmAuthoritativeCommandReceipt acceptedReceipt = receipt;
    receipt.snapshot.worldBytes[0] ^= 0x01U;
    ok = ok && host.LastReceipt() && host.LastReceipt()->snapshot.worldBytes == acceptedReceipt.snapshot.worldBytes;
    ok = ok && host.Submit(sessionA, Till("host-cmd-001", 1U, 1U, 0U, 0U), 1U, receipt) && receipt.decision.replayed && receipt.decision.authoritativeRevision == acceptedReceipt.decision.authoritativeRevision && !receipt.delta.stateChanged && receipt.snapshot.worldBytes == acceptedReceipt.snapshot.worldBytes;
    const std::vector<uint8_t> beforeRejectedWorld = world.Serialize();
    ok = ok && !host.Submit(sessionA, {"player-a", "host-cmd-002", "farm.sell", 2U, 2U, {}}, 2U, receipt) && host.LastError() == FarmAuthoritativeSessionError::CommandRejected && world.Serialize() == beforeRejectedWorld && host.LastReceipt() && host.LastReceipt()->decision.replayed;
    ok = ok && host.Authenticate({"player-a", "session-a-0002"}, sessionB) && sessionB != sessionA && host.SessionCount() == 1U;
    ok = ok && !host.Submit(sessionA, Till("host-cmd-002", 2U, 2U, 1U, 0U), 2U, receipt) && host.LastError() == FarmAuthoritativeSessionError::UnknownSession;
    ok = ok && host.Submit(sessionB, Till("host-cmd-002", 2U, 2U, 1U, 0U), 2U, receipt) && !receipt.decision.replayed && receipt.decision.authoritativeRevision == 2U && receipt.delta.baseRevision == 1U && receipt.delta.authoritativeRevision == 2U && receipt.delta.stateChanged && farm.TileStateAt(1U, 0U) == FarmTileState::Tilled;
    if (!ok) {
        std::fprintf(stderr, "FARM_AUTHORITATIVE_SESSION_HOST_SMOKE_FAIL\n");
        return 1;
    }
    std::printf("FARM_AUTHORITATIVE_SESSION_HOST_SMOKE_OK auth=1 replay=1 spoof=1 stale=1 snapshot=1\n");
    return 0;
}
