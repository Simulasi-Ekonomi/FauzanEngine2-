#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class ReplicationPriority : uint8_t { Low = 0, Normal = 1, High = 2, Critical = 3 };

struct ReplicationInterest {
    uint64_t actorId{};
    uint64_t observerId{};
    uint32_t distanceSquared{};
    uint16_t relevanceRadius{};
    ReplicationPriority priority{ReplicationPriority::Normal};
    bool alwaysRelevant{};
};

class NetworkReplicationPriorityPolicy {
public:
    static bool relevant(const ReplicationInterest& interest) {
        if (!interest.actorId || !interest.observerId) return false;
        if (interest.alwaysRelevant) return true;
        const uint32_t radius = interest.relevanceRadius;
        return interest.distanceSquared <= radius * radius;
    }

    static uint8_t score(const ReplicationInterest& interest) {
        if (!relevant(interest)) return 0;
        return static_cast<uint8_t>(interest.priority) + 1u;
    }
};

} // namespace NeoEngine::Networking
