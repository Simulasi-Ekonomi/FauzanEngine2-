#pragma once

#include "FarmSystem.h"
#include "FarmWorldTool.h"

#include <cstdint>
#include <string>

namespace NeoEngine {

struct FarmTelemetryIdentity {
    std::string sourceRef;
    std::string gameKey;
    std::string engineVersion;
    std::string playerId;
};

struct FarmTelemetryPolicy {
    bool includePlayerId = false;
    bool includeEconomicValues = false;
    bool includeEventValues = false;
    uint16_t maxEvents = 32U;
};

class FarmTelemetryAdapter {
public:
    explicit FarmTelemetryAdapter(FarmTelemetryIdentity identity, FarmTelemetryPolicy policy = {})
        : m_Identity(std::move(identity)), m_Policy(policy) {}
    bool BuildEnvelope(const FarmSystem& farm, uint64_t occurredAtMs, std::string& json) const;
    bool BuildWorldEnvelope(const FarmSystem& farm, const FarmWorldTool& world, uint64_t occurredAtMs, std::string& json) const;

private:
    static bool IsIdentityValid(const FarmTelemetryIdentity& identity);
    FarmTelemetryIdentity m_Identity;
    FarmTelemetryPolicy m_Policy;
};

} // namespace NeoEngine
