#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

struct ReliablePacket {
    uint64_t sequence{};
    uint64_t payloadId{};
};

class ReliableChannel {
public:
    static constexpr uint32_t MaxInFlight = 256;

    bool enqueue(uint64_t payloadId) {
        if (!payloadId || inFlight_ >= MaxInFlight) return false;
        ++nextSequence_;
        lastQueuedPayload_ = payloadId;
        ++inFlight_;
        return true;
    }

    bool acknowledge(uint64_t sequence) {
        if (!sequence || sequence > nextSequence_ || sequence <= lastAcknowledged_) return false;
        lastAcknowledged_ = sequence;
        inFlight_ = static_cast<uint32_t>(nextSequence_ - lastAcknowledged_);
        return true;
    }

    bool shouldAccept(uint64_t sequence) const {
        return sequence > lastReceived_;
    }

    bool receive(uint64_t sequence) {
        if (!shouldAccept(sequence)) return false;
        lastReceived_ = sequence;
        return true;
    }

    uint64_t nextSequence() const { return nextSequence_; }
    uint64_t lastAcknowledged() const { return lastAcknowledged_; }
    uint64_t lastReceived() const { return lastReceived_; }
    uint32_t inFlight() const { return inFlight_; }
    uint64_t lastQueuedPayload() const { return lastQueuedPayload_; }

    void reset() {
        nextSequence_ = 0;
        lastAcknowledged_ = 0;
        lastReceived_ = 0;
        lastQueuedPayload_ = 0;
        inFlight_ = 0;
    }

private:
    uint64_t nextSequence_{};
    uint64_t lastAcknowledged_{};
    uint64_t lastReceived_{};
    uint64_t lastQueuedPayload_{};
    uint32_t inFlight_{};
};

} // namespace NeoEngine::Networking
