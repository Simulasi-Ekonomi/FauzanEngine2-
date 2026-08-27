#pragma once
#include "../Runtime/ReplicationWorld.h"

#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

class NetworkReplicationBaseline {
public:
    static constexpr uint8_t kHistory = 8U;

    bool store(const ReplicationSnapshot& snapshot) {
        if (snapshot.sequence == 0U || snapshot.count > ReplicationSnapshot::kMaxEntities) return false;
        const uint8_t slot = static_cast<uint8_t>(snapshot.sequence % kHistory);
        history_[slot] = snapshot;
        valid_[slot] = true;
        return true;
    }

    bool find(uint64_t sequence, ReplicationSnapshot& snapshot) const {
        if (sequence == 0U) return false;
        const uint8_t slot = static_cast<uint8_t>(sequence % kHistory);
        if (!valid_[slot] || history_[slot].sequence != sequence) return false;
        snapshot = history_[slot];
        return true;
    }

    bool has(uint64_t sequence) const {
        ReplicationSnapshot snapshot{};
        return find(sequence, snapshot);
    }

    void clear() {
        valid_.fill(false);
        for (auto& snapshot : history_) snapshot = {};
    }

private:
    std::array<ReplicationSnapshot, kHistory> history_{};
    std::array<bool, kHistory> valid_{};
};

} // namespace NeoEngine::Networking
