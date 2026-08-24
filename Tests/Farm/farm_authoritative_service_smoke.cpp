#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

namespace {

NeoEngine::AuthorityCommand Command(const char* id, const char* kind, uint64_t sequence, uint16_t x, uint16_t z) {
    return {"farm-authority-player", "farm-authority-session", id, kind, sequence, 0, {static_cast<uint8_t>(x & 0xFFU), static_cast<uint8_t>(x >> 8U), static_cast<uint8_t>(z & 0xFFU), static_cast<uint8_t>(z >> 8U)}};
}

} // namespace

int main() {
    using namespace NeoEngine;
    FarmSystem farm(20, 20, 100);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.worldWidth = 20;
    config.worldHeight = 20;
    if (!world.Initialize(farm, trust, "farm-authority-player", config)) return 1;
    FarmAuthoritativeService service;
    if (!service.Initialize(world, trust, "farm-authority-player", "farm-authority-session")) return 1;

    const AuthorityDecision till = service.Submit(Command("farm-cmd-001", "farm.till", 1, 2, 2), 10);
    const AuthorityDecision tillReplay = service.Submit(Command("farm-cmd-001", "farm.till", 1, 2, 2), 10);
    const AuthorityDecision plant = service.Submit(Command("farm-cmd-002", "farm.plant.wheat", 2, 2, 2), 11);
    const AuthorityDecision water = service.Submit(Command("farm-cmd-003", "farm.water", 3, 2, 2), 12);
    if (!world.Tick(12)) return 1;
    const AuthorityDecision harvest = service.Submit(Command("farm-cmd-004", "farm.harvest", 4, 2, 2), 25);
    FarmAuthoritySnapshot snapshot;
    const uint64_t serverWorldState = world.DeterministicState();
    if (!service.BuildSnapshot(snapshot) || snapshot.revision != 4 || snapshot.worldBytes.empty()) return 1;
    FarmSystem reconciledFarm(20, 20, 0);
    TrustSafetySystem reconciledTrust;
    FarmWorldTool reconciledWorld;
    if (!reconciledWorld.Initialize(reconciledFarm, reconciledTrust, "farm-authority-player", config) || !reconciledWorld.Deserialize(snapshot.worldBytes) || reconciledWorld.DeterministicState() != serverWorldState) return 1;
    const AuthorityDecision invalidKind = service.Submit(Command("farm-cmd-005", "farm.topup", 5, 2, 2), 26);
    if (!trust.Report("farm-authority-player", "farm-authority-fraud-a", FraudSignal::LedgerMismatch) || !trust.Report("farm-authority-player", "farm-authority-fraud-b", FraudSignal::LedgerMismatch)) return 1;
    const AuthorityDecision banned = service.Submit(Command("farm-cmd-006", "farm.till", 5, 3, 3), 27);
    if (!till.Accepted() || till.authoritativeRevision != 1 || !tillReplay.Accepted() || !tillReplay.replayed || tillReplay.authoritativeRevision != 1 ||
        !plant.Accepted() || !water.Accepted() || !harvest.Accepted() || service.LastHarvestedUnits() != 2 || invalidKind.error != AuthorityError::HandlerRejected ||
        banned.error != AuthorityError::Banned || service.Revision() != 4 || farm.TileStateAt(2, 2) != FarmTileState::Empty) {
        return 1;
    }
    std::printf("FARM_AUTHORITATIVE_SERVICE_SMOKE_OK revision=%llu harvested=%u replay=1 banned=1\n", static_cast<unsigned long long>(service.Revision()), service.LastHarvestedUnits());
    return 0;
}
