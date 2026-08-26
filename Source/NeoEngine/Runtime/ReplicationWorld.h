#pragma once

#include "SceneWorld.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class ReplicationRole : uint8_t { Server, Client };
enum class ReplicationError : uint8_t {
    None,
    NotInitialized,
    InvalidEntity,
    InvalidNetworkId,
    DuplicateNetworkId,
    Capacity,
    NotServer,
    NotClient,
    OwnershipRejected,
    InvalidSnapshot,
    StaleSnapshot,
    UnknownEntity,
    InvalidInput,
    SceneApplyRejected,
    CorruptSnapshot,
    SpawnRejected,
    DespawnRejected,
};

struct ReplicatedEntityState {
    uint32_t networkId = 0U;
    uint32_t ownerId = 0U;
    uint64_t stateRevision = 0U;
    Transform3 transform{};
};

struct ReplicationSnapshot {
    static constexpr uint16_t kMaxEntities = 1024U;
    uint64_t sequence = 0U;
    uint64_t serverTick = 0U;
    uint16_t count = 0U;
    std::array<ReplicatedEntityState, kMaxEntities> states{};
    uint64_t checksum = 0U;
};

struct ReplicationApplyReceipt {
    uint64_t sequence = 0U;
    uint64_t serverTick = 0U;
    uint16_t appliedEntities = 0U;
    uint16_t spawnedEntities = 0U;
    uint16_t despawnedEntities = 0U;
    uint16_t interpolatedEntities = 0U;
    uint32_t reconciledPredictions = 0U;
    bool accepted = false;
};

struct ReplicationPredictionReceipt {
    uint32_t networkId = 0U;
    uint64_t predictionSequence = 0U;
    Transform3 predictedTransform{};
};

class ReplicationSnapshotCodec {
public:
    static constexpr size_t kMaxBytes = 1024U * 1024U;
    static bool Serialize(const ReplicationSnapshot& snapshot, std::vector<uint8_t>& bytes, ReplicationError& error);
    static bool Deserialize(std::span<const uint8_t> bytes, ReplicationSnapshot& snapshot, ReplicationError& error);
};

class ReplicationWorld {
public:
    static constexpr uint16_t kMaxEntities = ReplicationSnapshot::kMaxEntities;
    static constexpr uint16_t kMaxInterpolationPermille = 1000U;
    static constexpr float kMaxPredictionDelta = 10.0F;

    ReplicationWorld(SceneWorld& sceneWorld, ReplicationRole role, uint32_t localClientId = 0U, bool allowDynamicLifecycle = false);
    ReplicationWorld(const ReplicationWorld&) = delete;
    ReplicationWorld& operator=(const ReplicationWorld&) = delete;

    bool RegisterEntity(SceneEntity entity, uint32_t networkId, uint32_t ownerId);
    bool UnregisterEntity(uint32_t networkId);
    bool BuildServerSnapshot(uint64_t serverTick, ReplicationSnapshot& snapshot);
    bool ApplyServerSnapshot(const ReplicationSnapshot& snapshot, ReplicationApplyReceipt& receipt);
    bool SetInterpolationAlphaPermille(uint16_t alphaPermille);
    bool ApplyInterpolation(ReplicationApplyReceipt& receipt);
    bool PredictLocalInput(uint32_t networkId, float deltaX, float deltaZ, ReplicationPredictionReceipt& receipt);
    bool SetDynamicLifecycleEnabled(bool enabled);

    [[nodiscard]] bool IsRegistered(uint32_t networkId) const;
    [[nodiscard]] bool AuthoritativeState(uint32_t networkId, ReplicatedEntityState& state) const;
    [[nodiscard]] uint16_t RegisteredCount() const { return registeredCount_; }
    [[nodiscard]] uint64_t SnapshotSequence() const { return snapshotSequence_; }
    [[nodiscard]] uint64_t PredictionSequence() const { return predictionSequence_; }
    [[nodiscard]] ReplicationRole Role() const { return role_; }
    [[nodiscard]] ReplicationError LastError() const { return lastError_; }

private:
    struct Slot {
        bool registered = false;
        SceneEntity entity{};
        uint32_t networkId = 0U;
        uint32_t ownerId = 0U;
        uint64_t stateRevision = 0U;
        Transform3 previousAuthoritative{};
        Transform3 authoritative{};
        Transform3 predictedTransform{};
        bool hasAuthoritative = false;
        bool hasPrediction = false;
    };

    bool Fail(ReplicationError error);
    bool ValidTransform(const Transform3& transform) const;
    Slot* FindSlot(uint32_t networkId);
    const Slot* FindSlot(uint32_t networkId) const;
    Slot* FindSlot(SceneEntity entity);
    bool ValidateSnapshot(const ReplicationSnapshot& snapshot) const;
    bool ApplyTransform(const Slot& slot, const Transform3& transform);

    SceneWorld& sceneWorld_;
    ReplicationRole role_ = ReplicationRole::Server;
    uint32_t localClientId_ = 0U;
    bool allowDynamicLifecycle_ = false;
    std::array<Slot, kMaxEntities> slots_{};
    uint16_t registeredCount_ = 0U;
    uint64_t snapshotSequence_ = 0U;
    uint64_t predictionSequence_ = 0U;
    uint16_t interpolationAlphaPermille_ = 1000U;
    ReplicationError lastError_ = ReplicationError::None;
};

} // namespace NeoEngine
