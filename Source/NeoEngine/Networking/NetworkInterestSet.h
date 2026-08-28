#pragma once
#include "NetworkReplicationPriority.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace NeoEngine::Networking {

class NetworkInterestSet {
public:
    static constexpr std::size_t kMaxActors = 1024;

    explicit NetworkInterestSet(uint64_t observerId = 0) : observerId_(observerId) {}

    void setObserver(uint64_t observerId) { observerId_ = observerId; clear(); }

    bool rebuild(const ReplicationInterest* candidates, std::size_t count) {
        if (!observerId_ || (!candidates && count != 0) || count > kMaxActors) return false;
        std::size_t candidateSize = 0;
        for (std::size_t i = 0; i < count; ++i) {
            ReplicationInterest interest = candidates[i];
            interest.observerId = observerId_;
            if (!NetworkReplicationPriorityPolicy::relevant(interest)) continue;
            bool duplicate = false;
            for (std::size_t j = 0; j < candidateSize; ++j) if (actors_[j].actorId == interest.actorId) { duplicate = true; break; }
            if (!duplicate) actors_[candidateSize++] = interest;
        }
        std::sort(actors_.begin(), actors_.begin() + static_cast<std::ptrdiff_t>(candidateSize),
                  [](const ReplicationInterest& a, const ReplicationInterest& b) {
                      return a.actorId < b.actorId;
                  });
        size_ = candidateSize;
        return true;
    }

    std::size_t size() const { return size_; }
    const ReplicationInterest* data() const { return actors_.data(); }
    const ReplicationInterest& operator[](std::size_t index) const { return actors_[index]; }
    uint64_t observerId() const { return observerId_; }

private:
    bool contains(uint64_t actorId) const {
        for (std::size_t i = 0; i < size_; ++i) if (actors_[i].actorId == actorId) return true;
        return false;
    }
    void clear() { size_ = 0; }

    uint64_t observerId_{};
    std::array<ReplicationInterest, kMaxActors> actors_{};
    std::size_t size_{};
};

} // namespace NeoEngine::Networking
