#include "FarmAuthoritativeService.h"

#include "FarmWorldTool.h"
#include "TrustSafetySystem.h"

namespace NeoEngine {

bool FarmAuthoritativeService::Initialize(FarmWorldTool& world, TrustSafetySystem& trustSafety, std::string playerId, std::string sessionId, const AuthorityConfig& config) {
    ready_ = false;
    world_ = nullptr;
    playerId_.clear();
    lastHarvestedUnits_ = 0;
    if (!world.IsReady() || !gate_.Initialize(trustSafety, config) || !gate_.BindSession(playerId, sessionId)) return false;
    world_ = &world;
    playerId_ = std::move(playerId);
    ready_ = true;
    return true;
}

bool FarmAuthoritativeService::BindSession(const std::string& playerId, const std::string& sessionId) {
    if (!ready_ || playerId != playerId_) return false;
    return gate_.BindSession(playerId, sessionId);
}

AuthorityDecision FarmAuthoritativeService::Submit(const AuthorityCommand& command, uint64_t serverTick) {
    if (!ready_ || world_ == nullptr || command.playerId != playerId_) return {AuthorityError::Unauthenticated, gate_.AuthoritativeRevision(), false};
    return gate_.Submit(command, serverTick, [this](const AuthorityCommand& value, uint64_t revision) { return Execute(value, revision); });
}

bool FarmAuthoritativeService::BuildSnapshot(FarmAuthoritySnapshot& snapshot) const {
    snapshot = {};
    if (!ready_ || world_ == nullptr || gate_.AuthoritativeRevision() == 0) return false;
    snapshot.revision = gate_.AuthoritativeRevision();
    snapshot.worldBytes = world_->Serialize();
    return !snapshot.worldBytes.empty();
}

bool FarmAuthoritativeService::Execute(const AuthorityCommand& command, uint64_t) {
    uint16_t x = 0;
    uint16_t z = 0;
    if (!ReadCoordinate(command, x, z)) return false;
    if (command.kind == "farm.till") return world_->PlayerTill(x, z);
    if (command.kind == "farm.plant.wheat") return world_->PlayerPlant(x, z, FarmCrop::Wheat);
    if (command.kind == "farm.plant.corn") return world_->PlayerPlant(x, z, FarmCrop::Corn);
    if (command.kind == "farm.plant.tomato") return world_->PlayerPlant(x, z, FarmCrop::Tomato);
    if (command.kind == "farm.water") return world_->PlayerWater(x, z);
    if (command.kind == "farm.harvest") return world_->PlayerHarvest(x, z, lastHarvestedUnits_);
    return false;
}

bool FarmAuthoritativeService::ReadCoordinate(const AuthorityCommand& command, uint16_t& x, uint16_t& z) {
    if (command.payload.size() != 4) return false;
    x = static_cast<uint16_t>(command.payload[0]) | (static_cast<uint16_t>(command.payload[1]) << 8U);
    z = static_cast<uint16_t>(command.payload[2]) | (static_cast<uint16_t>(command.payload[3]) << 8U);
    return true;
}

} // namespace NeoEngine
