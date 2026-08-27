#pragma once
#include <algorithm>
#include <cstdint>

namespace NeoEngine::Networking {

struct NetworkTransformSample {
    uint64_t sequence{0};
    double timestamp{0.0};
    float x{0.0f}, y{0.0f}, z{0.0f};
};

struct NetworkTransformState {
    float x{0.0f}, y{0.0f}, z{0.0f};
};

class NetworkStateSmoother {
public:
    static NetworkTransformState interpolate(const NetworkTransformSample& a,
                                             const NetworkTransformSample& b,
                                             double timestamp) {
        if (b.timestamp <= a.timestamp) return {b.x, b.y, b.z};
        const double t = std::clamp((timestamp - a.timestamp) / (b.timestamp - a.timestamp), 0.0, 1.0);
        return {
            static_cast<float>(a.x + (b.x - a.x) * t),
            static_cast<float>(a.y + (b.y - a.y) * t),
            static_cast<float>(a.z + (b.z - a.z) * t)
        };
    }

    static NetworkTransformState extrapolate(const NetworkTransformSample& previous,
                                             const NetworkTransformSample& latest,
                                             double timestamp,
                                             double maxExtrapolationSeconds = 0.25) {
        if (latest.timestamp <= previous.timestamp || timestamp <= latest.timestamp)
            return {latest.x, latest.y, latest.z};
        const double dt = std::min(timestamp - latest.timestamp, std::max(0.0, maxExtrapolationSeconds));
        const double interval = latest.timestamp - previous.timestamp;
        const double vx = (latest.x - previous.x) / interval;
        const double vy = (latest.y - previous.y) / interval;
        const double vz = (latest.z - previous.z) / interval;
        return {
            static_cast<float>(latest.x + vx * dt),
            static_cast<float>(latest.y + vy * dt),
            static_cast<float>(latest.z + vz * dt)
        };
    }
};

} // namespace NeoEngine::Networking
