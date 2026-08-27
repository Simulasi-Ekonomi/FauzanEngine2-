#pragma once
#include <cstdint>
#include "NetworkTelemetry.h"

namespace NeoEngine::Networking {

enum class NetworkTelemetryEvent : uint8_t {
    PacketSent,
    PacketReceived,
    Retransmission,
    Duplicate,
    OutOfOrder,
    ReplicationUpdate,
    RpcAccepted,
    RpcRejected,
    PredictionCorrection
};

struct NetworkTelemetryEventData {
    NetworkTelemetryEvent type{};
    uint64_t value{};
};

class NetworkTelemetrySink {
public:
    explicit NetworkTelemetrySink(NetworkTelemetry& telemetry) : telemetry_(telemetry) {}

    void emit(const NetworkTelemetryEventData& event) {
        switch (event.type) {
        case NetworkTelemetryEvent::PacketSent: telemetry_.recordSent(event.value); break;
        case NetworkTelemetryEvent::PacketReceived: telemetry_.recordReceived(event.value); break;
        case NetworkTelemetryEvent::Retransmission: telemetry_.recordRetransmission(); break;
        case NetworkTelemetryEvent::Duplicate: telemetry_.recordDuplicate(); break;
        case NetworkTelemetryEvent::OutOfOrder: telemetry_.recordOutOfOrder(); break;
        case NetworkTelemetryEvent::ReplicationUpdate: telemetry_.recordReplicationUpdate(); break;
        case NetworkTelemetryEvent::RpcAccepted: telemetry_.recordRpcAccepted(); break;
        case NetworkTelemetryEvent::RpcRejected: telemetry_.recordRpcRejected(); break;
        case NetworkTelemetryEvent::PredictionCorrection: telemetry_.recordPredictionCorrection(); break;
        }
    }

private:
    NetworkTelemetry& telemetry_;
};

} // namespace NeoEngine::Networking
