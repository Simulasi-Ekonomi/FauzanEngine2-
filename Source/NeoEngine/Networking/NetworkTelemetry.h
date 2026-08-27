#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

struct NetworkTelemetrySnapshot {
    uint64_t packetsSent{};
    uint64_t packetsReceived{};
    uint64_t bytesSent{};
    uint64_t bytesReceived{};
    uint64_t retransmissions{};
    uint64_t duplicates{};
    uint64_t outOfOrder{};
    uint64_t replicationUpdates{};
    uint64_t rpcAccepted{};
    uint64_t rpcRejected{};
    uint64_t predictionCorrections{};
    uint32_t rttMs{};
    uint32_t jitterMs{};
    uint32_t packetLossPermille{};
};

class NetworkTelemetry {
public:
    void recordSent(uint64_t bytes) { ++snapshot_.packetsSent; snapshot_.bytesSent += bytes; }
    void recordReceived(uint64_t bytes) { ++snapshot_.packetsReceived; snapshot_.bytesReceived += bytes; }
    void recordRetransmission() { ++snapshot_.retransmissions; }
    void recordDuplicate() { ++snapshot_.duplicates; }
    void recordOutOfOrder() { ++snapshot_.outOfOrder; }
    void recordReplicationUpdate() { ++snapshot_.replicationUpdates; }
    void recordRpcAccepted() { ++snapshot_.rpcAccepted; }
    void recordRpcRejected() { ++snapshot_.rpcRejected; }
    void recordPredictionCorrection() { ++snapshot_.predictionCorrections; }
    void setRtt(uint32_t ms) { snapshot_.rttMs = ms; }
    void setJitter(uint32_t ms) { snapshot_.jitterMs = ms; }
    void setPacketLossPermille(uint32_t value) { snapshot_.packetLossPermille = value > 1000 ? 1000 : value; }
    const NetworkTelemetrySnapshot& snapshot() const { return snapshot_; }
    void reset() { snapshot_ = {}; }
private:
    NetworkTelemetrySnapshot snapshot_{};
};

} // namespace NeoEngine::Networking
