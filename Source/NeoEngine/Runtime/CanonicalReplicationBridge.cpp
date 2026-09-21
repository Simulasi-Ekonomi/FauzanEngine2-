#include "CanonicalReplicationBridge.h"
#include "CanonicalRuntimeWorld.h"

#include <cmath>
#include <limits>

namespace NeoEngine {

CanonicalReplicationBridge::CanonicalReplicationBridge(CanonicalRuntimeWorld& world, ReplicationRole role,
                                                       uint32_t localClientId, bool allowDynamicLifecycle)
    : world_(world), replication_(world.Scene(), role, localClientId, allowDynamicLifecycle) {}

bool CanonicalReplicationBridge::Register(const CanonicalEntity& entity, uint32_t networkId, uint32_t ownerId) {
    if (entity.scene.index == 0xFFFFU || world_.Scene().GetTransform(entity.scene) == nullptr) {
        lastError_ = CanonicalReplicationBridgeError::InvalidEntity;
        return false;
    }
    if (!replication_.RegisterEntity(entity.scene, networkId, ownerId)) {
        lastError_ = CanonicalReplicationBridgeError::RegistrationFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::Unregister(uint32_t networkId) {
    if (!replication_.UnregisterEntity(networkId)) {
        lastError_ = CanonicalReplicationBridgeError::RegistrationFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::BuildSnapshot(uint64_t serverTick, ReplicationSnapshot& snapshot) {
    if (serverTick == std::numeric_limits<uint64_t>::max()) { lastError_ = ReplicationError::InvalidSnapshot; return false; }
    snapshot = {};
    if (!replication_.BuildServerSnapshot(serverTick, snapshot)) {
        lastError_ = CanonicalReplicationBridgeError::SnapshotFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::ApplySnapshot(const ReplicationSnapshot& snapshot, ReplicationApplyReceipt& receipt) {
    receipt = {};
    if (!replication_.ApplyServerSnapshot(snapshot, receipt)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::BuildAcknowledgement(ReplicationAcknowledgement& acknowledgement) const {
    acknowledgement = {};
    if (!replication_.BuildClientAcknowledgement(acknowledgement)) {
        lastError_ = CanonicalReplicationBridgeError::AcknowledgementFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::EncodeSnapshot(const ReplicationSnapshot& snapshot, std::vector<uint8_t>& bytes) {
    bytes.clear();
    if (bytes.capacity() > ReplicationSnapshotCodec::kMaxBytes) std::vector<uint8_t>().swap(bytes);
    ReplicationError error = ReplicationError::None;
    if (!ReplicationSnapshotCodec::Serialize(snapshot, bytes, error) || bytes.size() > ReplicationSnapshotCodec::kMaxBytes) {
        lastError_ = CanonicalReplicationBridgeError::EncodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::DecodeSnapshot(std::span<const uint8_t> bytes, ReplicationSnapshot& snapshot) {
    if (bytes.size() > ReplicationSnapshotCodec::kMaxBytes) { lastError_ = CanonicalReplicationBridgeError::DecodeFailed; return false; }
    ReplicationError error = ReplicationError::None;
    if (!ReplicationSnapshotCodec::Deserialize(bytes, snapshot, error)) {
        lastError_ = CanonicalReplicationBridgeError::DecodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::EncodeAcknowledgement(const ReplicationAcknowledgement& acknowledgement,
                                                       std::vector<uint8_t>& bytes) {
    bytes.clear();
    if (bytes.capacity() > ReplicationAcknowledgementCodec::kMaxBytes) std::vector<uint8_t>().swap(bytes);
    ReplicationError error = ReplicationError::None;
    if (!ReplicationAcknowledgementCodec::Serialize(acknowledgement, bytes, error) || bytes.size() > ReplicationAcknowledgementCodec::kMaxBytes) {
        lastError_ = CanonicalReplicationBridgeError::EncodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::DecodeAcknowledgement(std::span<const uint8_t> bytes,
                                                        ReplicationAcknowledgement& acknowledgement) {
    if (bytes.size() > ReplicationAcknowledgementCodec::kMaxBytes) { lastError_ = CanonicalReplicationBridgeError::DecodeFailed; return false; }
    ReplicationError error = ReplicationError::None;
    if (!ReplicationAcknowledgementCodec::Deserialize(bytes, acknowledgement, error)) {
        lastError_ = CanonicalReplicationBridgeError::DecodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::ApplyAcknowledgement(const ReplicationAcknowledgement& acknowledgement) {
    if (!replication_.ApplyClientAcknowledgement(acknowledgement)) {
        lastError_ = CanonicalReplicationBridgeError::AcknowledgementFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::Predict(uint32_t networkId, float deltaX, float deltaZ,
                                         ReplicationPredictionReceipt& receipt) {
    receipt = {};
    if (!std::isfinite(deltaX) || !std::isfinite(deltaZ)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    if (!replication_.PredictLocalInput(networkId, deltaX, deltaZ, receipt)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::Interpolate(ReplicationApplyReceipt& receipt) {
    receipt = {};
    if (!replication_.ApplyInterpolation(receipt)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

} // namespace NeoEngine
