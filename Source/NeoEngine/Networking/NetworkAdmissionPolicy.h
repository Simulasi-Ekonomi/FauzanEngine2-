#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class AdmissionResult : uint8_t { Accepted, InvalidPeer, InvalidProtocol, Capacity, DuplicateSession };

struct AdmissionRequest {
    uint32_t peerId{};
    uint16_t protocolVersion{};
    uint64_t sessionId{};
};

class NetworkAdmissionPolicy {
public:
    static constexpr uint16_t ProtocolVersion = 1;
    explicit NetworkAdmissionPolicy(uint32_t maxPeers = 64) : maxPeers_(maxPeers) {}

    AdmissionResult evaluate(const AdmissionRequest& request) const {
        if (!request.peerId) return AdmissionResult::InvalidPeer;
        if (request.protocolVersion != ProtocolVersion) return AdmissionResult::InvalidProtocol;
        if (request.sessionId && request.sessionId == activeSessionId_) return AdmissionResult::DuplicateSession;
        if (activePeers_ >= maxPeers_) return AdmissionResult::Capacity;
        return AdmissionResult::Accepted;
    }

    void admit(uint32_t peerId, uint64_t sessionId) {
        if (peerId && sessionId) { ++activePeers_; activeSessionId_ = sessionId; }
    }

    void release(uint64_t sessionId) {
        if (sessionId == activeSessionId_ && activePeers_) { --activePeers_; if (!activePeers_) activeSessionId_ = 0; }
    }

    uint32_t activePeers() const { return activePeers_; }
    uint32_t maxPeers() const { return maxPeers_; }

private:
    uint32_t maxPeers_{};
    uint32_t activePeers_{};
    uint64_t activeSessionId_{};
};

} // namespace NeoEngine::Networking
