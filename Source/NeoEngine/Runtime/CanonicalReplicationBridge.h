#pragma once

#include "CanonicalRuntimeWorld.h"
#include "ReplicationWorld.h"

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class CanonicalReplicationBridgeError : uint8_t {
    None,
    InvalidEntity,
    RegistrationFailed,
    SnapshotFailed,
    ApplyFailed,
    AcknowledgementFailed,
    EncodeFailed,
    DecodeFailed
};

class CanonicalReplicationBridge {
public:
    CanonicalReplicationBridge(CanonicalRuntimeWorld& world, ReplicationRole role,
                                uint32_t localClientId = 0U, bool allowDynamicLifecycle = true);

    bool Register(const CanonicalEntity& entity, uint32_t networkId, uint32_t ownerId);
    bool Unregister(uint32_t networkId);
    bool BuildSnapshot(uint64_t serverTick, ReplicationSnapshot& snapshot);
    bool ApplySnapshot(const ReplicationSnapshot& snapshot, ReplicationApplyReceipt& receipt);
    bool BuildAcknowledgement(ReplicationAcknowledgement& acknowledgement) const;
    bool ApplyAcknowledgement(const ReplicationAcknowledgement& acknowledgement);
    bool EncodeSnapshot(const ReplicationSnapshot& snapshot, std::vector<uint8_t>& bytes);
    bool DecodeSnapshot(std::span<const uint8_t> bytes, ReplicationSnapshot& snapshot);
    bool EncodeAcknowledgement(const ReplicationAcknowledgement& acknowledgement, std::vector<uint8_t>& bytes);
    bool DecodeAcknowledgement(std::span<const uint8_t> bytes, ReplicationAcknowledgement& acknowledgement);
    bool Predict(uint32_t networkId, float deltaX, float deltaZ, ReplicationPredictionReceipt& receipt);
    bool Interpolate(ReplicationApplyReceipt& receipt);

    [[nodiscard]] const ReplicationWorld& Replication() const { return replication_; }
    [[nodiscard]] ReplicationWorld& Replication() { return replication_; }
    [[nodiscard]] CanonicalReplicationBridgeError LastError() const { return lastError_; }

private:
    CanonicalRuntimeWorld& world_;
    ReplicationWorld replication_;
    CanonicalReplicationBridgeError lastError_ = CanonicalReplicationBridgeError::None;
};

} // namespace NeoEngine
