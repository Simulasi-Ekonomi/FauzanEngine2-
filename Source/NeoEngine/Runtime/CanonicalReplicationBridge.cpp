#include "CanonicalReplicationBridge.h"

namespace NeoEngine {

CanonicalReplicationBridge::CanonicalReplicationBridge(CanonicalRuntimeWorld& world, ReplicationRole role,
                                                       uint32_t localClientId, bool allowDynamicLifecycle)
    : world_(world), replication_(world.Scene(), role, localClientId, allowDynamicLifecycle) {}

bool CanonicalReplicationBridge::Register(const CanonicalEntity& entity, uint32_t networkId, uint32_t ownerId) {
    CanonicalEntity canonical{};
    if (!world_.GetEntity(entity.scene, canonical) || !canonical.active || canonical.scene != entity.scene) {
        lastError_ = CanonicalReplicationBridgeError::InvalidEntity;
        return false;
    }
    if (!replication_.RegisterEntity(canonical.scene, networkId, ownerId)) {
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
    if (!replication_.BuildServerSnapshot(serverTick, snapshot)) {
        lastError_ = CanonicalReplicationBridgeError::SnapshotFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::ApplySnapshot(const ReplicationSnapshot& snapshot, ReplicationApplyReceipt& receipt) {
    if (!replication_.ApplyServerSnapshot(snapshot, receipt)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::BuildAcknowledgement(ReplicationAcknowledgement& acknowledgement) const {
    if (!replication_.BuildClientAcknowledgement(acknowledgement)) {
        lastError_ = CanonicalReplicationBridgeError::AcknowledgementFailed;
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
    if (!replication_.PredictLocalInput(networkId, deltaX, deltaZ, receipt)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::Interpolate(ReplicationApplyReceipt& receipt) {
    if (!replication_.ApplyInterpolation(receipt)) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

} // namespace NeoEngine
