#pragma once
#include <cstdint>
#include <limits>

namespace NeoEngine::Networking {

struct InterestPoint {
    float x{};
    float y{};
    float z{};
};

struct InterestEntity {
    uint32_t networkId{};
    InterestPoint position{};
    uint32_t teamId{};
    bool alwaysRelevant{false};
};

class InterestRelevancy {
public:
    explicit InterestRelevancy(float radius = 100.0f) : radiusSquared_(radius * radius) {}

    bool relevant(const InterestPoint& observer, const InterestEntity& entity,
                  uint32_t observerTeam = std::numeric_limits<uint32_t>::max()) const {
        if (entity.alwaysRelevant) return true;
        if (entity.networkId == 0) return false;
        if (entity.teamId != 0 && observerTeam != std::numeric_limits<uint32_t>::max() &&
            entity.teamId != observerTeam) return false;
        const float dx = entity.position.x - observer.x;
        const float dy = entity.position.y - observer.y;
        const float dz = entity.position.z - observer.z;
        return dx * dx + dy * dy + dz * dz <= radiusSquared_;
    }

private:
    float radiusSquared_;
};

} // namespace NeoEngine::Networking
