#include "CanonicalReplicationBridge.h"
#include "CanonicalRuntimeWorld.h"

#include <cmath>
#include <limits>

namespace NeoEngine {

CanonicalReplicationBridge::CanonicalReplicationBridge(CanonicalRuntimeWorld& world, ReplicationRole role,
                                                       uint32_t localClientId, bool allowDynamicLifecycle)
    : world_(world), replication_(world.Scene(), role, localClientId, allowDynamicLifecycle) {}

bool CanonicalReplicationBridge::Register(const CanonicalEntity& entity, uint32_t networkId, uint32_t ownerId) {
    if (entity.scene.index == 0xFFFFU || world_.Scene().GetTransform(entity.scene) == nullptr ||
        networkId == std::numeric_limits<uint32_t>::max() || ownerId == std::numeric_limits<uint32_t>::max()) {
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
    if (networkId == 0U || networkId == std::numeric_limits<uint32_t>::max()) {
        lastError_ = CanonicalReplicationBridgeError::RegistrationFailed;
        return false;
    }
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
    if (snapshot.count > ReplicationWorld::kMaxEntities || snapshot.sequence == 0U || snapshot.sequence == std::numeric_limits<uint64_t>::max() ||
        snapshot.serverTick == std::numeric_limits<uint64_t>::max() || snapshot.count > snapshot.states.size()) {
        snapshot = {};
        lastError_ = CanonicalReplicationBridgeError::SnapshotFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::ApplySnapshot(const ReplicationSnapshot& snapshot, ReplicationApplyReceipt& receipt) {
    receipt = {};
    if (snapshot.count == 0U || snapshot.count > ReplicationWorld::kMaxEntities || snapshot.sequence == 0U || snapshot.sequence == std::numeric_limits<uint64_t>::max() ||
        snapshot.serverTick == std::numeric_limits<uint64_t>::max()) { lastError_ = CanonicalReplicationBridgeError::ApplyFailed; return false; }
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
    if (acknowledgement.sequence == 0U || acknowledgement.sequence == std::numeric_limits<uint64_t>::max() ||
        acknowledgement.serverTick == std::numeric_limits<uint64_t>::max()) {
        acknowledgement = {};
        lastError_ = CanonicalReplicationBridgeError::AcknowledgementFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::EncodeSnapshot(const ReplicationSnapshot& snapshot, std::vector<uint8_t>& bytes) {
    bytes.clear();
    if (snapshot.count == 0U || snapshot.count > ReplicationWorld::kMaxEntities || snapshot.sequence == 0U || snapshot.sequence == std::numeric_limits<uint64_t>::max() ||
        snapshot.serverTick == std::numeric_limits<uint64_t>::max()) { lastError_ = CanonicalReplicationBridgeError::EncodeFailed; return false; }
    if (bytes.capacity() > ReplicationSnapshotCodec::kMaxBytes || bytes.capacity() < bytes.size()) std::vector<uint8_t>().swap(bytes);
    ReplicationError error = ReplicationError::None;
    if (!ReplicationSnapshotCodec::Serialize(snapshot, bytes, error) || bytes.size() > ReplicationSnapshotCodec::kMaxBytes) {
        lastError_ = CanonicalReplicationBridgeError::EncodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::DecodeSnapshot(std::span<const uint8_t> bytes, ReplicationSnapshot& snapshot) {
    snapshot = {};
    if (bytes.empty() || bytes.size() > ReplicationSnapshotCodec::kMaxBytes) { lastError_ = CanonicalReplicationBridgeError::DecodeFailed; return false; }
    ReplicationError error = ReplicationError::None;
    if (!ReplicationSnapshotCodec::Deserialize(bytes, snapshot, error)) {
        lastError_ = CanonicalReplicationBridgeError::DecodeFailed;
        return false;
    }
    if (snapshot.count == 0U || snapshot.count > ReplicationWorld::kMaxEntities ||
        snapshot.sequence == 0U || snapshot.sequence == std::numeric_limits<uint64_t>::max() || snapshot.serverTick == std::numeric_limits<uint64_t>::max()) {
        snapshot = {};
        lastError_ = CanonicalReplicationBridgeError::DecodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::EncodeAcknowledgement(const ReplicationAcknowledgement& acknowledgement,
                                                       std::vector<uint8_t>& bytes) {
    bytes.clear();
    if (acknowledgement.sequence == std::numeric_limits<uint64_t>::max() || acknowledgement.serverTick == std::numeric_limits<uint64_t>::max()) { lastError_ = CanonicalReplicationBridgeError::EncodeFailed; return false; }
    if (bytes.capacity() > ReplicationAcknowledgementCodec::kMaxBytes || bytes.capacity() < bytes.size()) std::vector<uint8_t>().swap(bytes);
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
    acknowledgement = {};
    if (bytes.size() != 38U || bytes.size() > ReplicationAcknowledgementCodec::kMaxBytes) { lastError_ = CanonicalReplicationBridgeError::DecodeFailed; return false; }
    ReplicationError error = ReplicationError::None;
    if (!ReplicationAcknowledgementCodec::Deserialize(bytes, acknowledgement, error)) {
        lastError_ = CanonicalReplicationBridgeError::DecodeFailed;
        return false;
    }
    if (acknowledgement.sequence == std::numeric_limits<uint64_t>::max() ||
        acknowledgement.serverTick == std::numeric_limits<uint64_t>::max()) {
        acknowledgement = {};
        lastError_ = CanonicalReplicationBridgeError::DecodeFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

bool CanonicalReplicationBridge::ApplyAcknowledgement(const ReplicationAcknowledgement& acknowledgement) {
    if (acknowledgement.sequence == std::numeric_limits<uint64_t>::max() || acknowledgement.serverTick == std::numeric_limits<uint64_t>::max()) { lastError_ = CanonicalReplicationBridgeError::AcknowledgementFailed; return false; }
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
    if (networkId == std::numeric_limits<uint32_t>::max() || !std::isfinite(deltaX) || !std::isfinite(deltaZ) ||
        std::abs(deltaX) > ReplicationWorld::kMaxPredictionDelta || std::abs(deltaZ) > ReplicationWorld::kMaxPredictionDelta) {
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
    if (replication_.SnapshotSequence() == 0U || replication_.SnapshotSequence() == std::numeric_limits<uint64_t>::max()) { lastError_ = CanonicalReplicationBridgeError::ApplyFailed; return false; }
    if (receipt.accepted || receipt.sequence != 0U || receipt.appliedEntities != 0U) { lastError_ = CanonicalReplicationBridgeError::ApplyFailed; return false; }
    if (!replication_.ApplyInterpolation(receipt) || !receipt.accepted) {
        lastError_ = CanonicalReplicationBridgeError::ApplyFailed;
        return false;
    }
    lastError_ = CanonicalReplicationBridgeError::None;
    return true;
}

} // namespace NeoEngine
