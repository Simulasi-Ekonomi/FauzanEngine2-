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

class FarmTelemetryAdapter {
public:
    explicit FarmTelemetryAdapter(FarmTelemetryIdentity identity) : m_Identity(std::move(identity)) {}
    bool BuildEnvelope(const FarmSystem& farm, uint64_t occurredAtMs, std::string& json) const;
    bool BuildWorldEnvelope(const FarmSystem& farm, const FarmWorldTool& world, uint64_t occurredAtMs, std::string& json) const;

private:
    static bool IsIdentityValid(const FarmTelemetryIdentity& identity);
    FarmTelemetryIdentity m_Identity;
};

} // namespace NeoEngine
