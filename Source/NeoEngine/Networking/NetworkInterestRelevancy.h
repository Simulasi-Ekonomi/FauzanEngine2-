#pragma once
#include <cmath>
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
    explicit InterestRelevancy(float radius = 100.0f) : radiusSquared_((std::isfinite(radius) && radius > 0.0F) ? radius * radius : 0.0F) {}

    bool relevant(const InterestPoint& observer, const InterestEntity& entity,
                  uint32_t observerTeam = std::numeric_limits<uint32_t>::max()) const {
        if (entity.networkId == 0 || radiusSquared_ <= 0.0F) return false;
        if (!std::isfinite(observer.x) || !std::isfinite(observer.y) || !std::isfinite(observer.z) ||
            !std::isfinite(entity.position.x) || !std::isfinite(entity.position.y) || !std::isfinite(entity.position.z)) return false;
        if (entity.alwaysRelevant) return true;
        if (entity.teamId != 0 && observerTeam != std::numeric_limits<uint32_t>::max() &&
            entity.teamId != observerTeam) return false;
        const float dx = entity.position.x - observer.x;
        const float dy = entity.position.y - observer.y;
        const float dz = entity.position.z - observer.z;
        const float distanceSquared = dx * dx + dy * dy + dz * dz;
        return std::isfinite(distanceSquared) && distanceSquared <= radiusSquared_;
    }

private:
    float radiusSquared_;
};

} // namespace NeoEngine::Networking
