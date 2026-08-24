#pragma once

#include "AuthoritativeCommandGate.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace NeoEngine {

class FarmWorldTool;
class TrustSafetySystem;

struct FarmAuthoritySnapshot {
    uint64_t revision = 0;
    std::vector<uint8_t> worldBytes;
};

class FarmAuthoritativeService {
public:
    bool Initialize(FarmWorldTool& world, TrustSafetySystem& trustSafety, std::string playerId, std::string sessionId, const AuthorityConfig& config = {});
    bool BindSession(const std::string& playerId, const std::string& sessionId);
    AuthorityDecision Submit(const AuthorityCommand& command, uint64_t serverTick);
    bool BuildSnapshot(FarmAuthoritySnapshot& snapshot) const;
    [[nodiscard]] std::vector<uint8_t> SerializeAuthorityLedger() const;
    bool RestoreAuthorityLedger(std::span<const uint8_t> bytes);
    [[nodiscard]] uint64_t Revision() const { return gate_.AuthoritativeRevision(); }
    [[nodiscard]] uint32_t LastHarvestedUnits() const { return lastHarvestedUnits_; }
    [[nodiscard]] AuthorityError LastError() const { return gate_.LastError(); }
    [[nodiscard]] bool IsReady() const { return ready_; }

private:
    bool Execute(const AuthorityCommand& command, uint64_t revision);
    static bool ReadCoordinate(const AuthorityCommand& command, uint16_t& x, uint16_t& z);

    FarmWorldTool* world_ = nullptr;
    std::string playerId_;
    AuthoritativeCommandGate gate_;
    uint32_t lastHarvestedUnits_ = 0;
    bool ready_ = false;
};

} // namespace NeoEngine
