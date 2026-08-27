#pragma once
#include <cstddef>
#include <cstdint>

namespace NeoEngine::Networking {

struct ReplicationDeltaHeader {
    uint64_t actorId{};
    uint64_t baselineRevision{};
    uint64_t targetRevision{};
    uint32_t payloadSize{};
};

class NetworkReplicationDelta {
public:
    static bool validHeader(const ReplicationDeltaHeader& header, std::size_t availableBytes) {
        if (!header.actorId || !header.baselineRevision || header.targetRevision <= header.baselineRevision) return false;
        if (header.payloadSize > availableBytes) return false;
        return true;
    }

    static bool applicable(uint64_t currentRevision, const ReplicationDeltaHeader& header) {
        return header.baselineRevision == currentRevision &&
               header.targetRevision > header.baselineRevision;
    }
};

} // namespace NeoEngine::Networking
