#include "Runtime/CanonicalRuntimeWorld.h"
#include "Runtime/CanonicalReplicationBridge.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

namespace {
bool Require(bool condition, const char* stage) {
    if (condition) return true;
    std::fprintf(stderr, "CANONICAL_REPLICATION_BRIDGE_FAIL %s\n", stage);
    return false;
}
}

int main() {
    using namespace NeoEngine;

    CanonicalRuntimeWorld world;
    CanonicalEntity entity{};
    if (!Require(world.CreateEntity(
                     {3.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F},
                     0U, CanonicalTransformAuthority::Scene, entity),
                 "create_entity")) return 1;
    if (!Require(world.ConfigureReplication(ReplicationRole::Server, 0U, false),
                 "configure_world_replication")) return 2;
    if (!Require(world.RegisterReplicatedEntity(entity, 1001U, 7U),
                 "register_world_entity") ||
        !Require(world.Replication().ReplicatedCount() == 1U, "world_replication_count")) {
        return 3;
    }

    ReplicationSnapshot worldSnapshot{};
    if (!Require(world.BuildReplicationSnapshot(1U, worldSnapshot), "world_snapshot") ||
        !Require(worldSnapshot.count == 1U &&
                     worldSnapshot.states[0].networkId == 1001U &&
                     worldSnapshot.states[0].networkId != std::numeric_limits<uint32_t>::max(),
                 "world_snapshot_content")) {
        return 4;
    }

    CanonicalEntity secondEntity{};
    if (!Require(world.CreateEntity(
                     {8.0F, 0.0F, 9.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F},
                     0U, CanonicalTransformAuthority::Scene, secondEntity),
                 "create_second_entity")) return 5;

    CanonicalReplicationBridge bridge(world, ReplicationRole::Server, 0U, false);
    if (!Require(bridge.Register(entity, 1002U, 7U), "bridge_register_first") ||
        !Require(bridge.Register(secondEntity, 1003U, 8U), "bridge_register_second") ||
        !Require(!bridge.Register(entity, 0U, 8U), "reject_zero_network_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidNetworkId,
                 "zero_network_id_error") ||
        !Require(!bridge.Register(entity, std::numeric_limits<uint32_t>::max(), 8U),
                 "reject_max_network_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidNetworkId,
                 "max_network_id_error") ||
        !Require(!bridge.Register(entity, 1004U, std::numeric_limits<uint32_t>::max()),
                 "reject_reserved_owner_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidOwnerId,
                 "owner_id_error") ||
        !Require(!bridge.Unregister(0U), "reject_zero_unregister_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidNetworkId,
                 "zero_unregister_id_error") ||
        !Require(bridge.Replication().ReplicatedCount() == 2U,
                 "invalid_id_does_not_mutate_registry")) {
        return 6;
    }

    ReplicationSnapshot snapshot{};
    if (!Require(bridge.BuildSnapshot(4U, snapshot), "bridge_snapshot") ||
        !Require(snapshot.count == 2U && snapshot.sequence != std::numeric_limits<uint64_t>::max() &&
                     snapshot.serverTick == 4U && snapshot.checksum != 0U,
                 "bridge_snapshot_content")) {
        return 7;
    }

    std::vector<uint8_t> bytes;
    if (!Require(bridge.EncodeSnapshot(snapshot, bytes), "encode_snapshot") ||
        !Require(!bytes.empty() && bytes.size() <= ReplicationSnapshotCodec::kMaxBytes,
                 "encoded_size")) {
        return 7;
    }

    ReplicationSnapshot decoded{};
    if (!Require(bridge.DecodeSnapshot(bytes, decoded), "decode_snapshot") ||
        !Require(decoded.count == snapshot.count && decoded.states.size() >= decoded.count &&
                     decoded.sequence == snapshot.sequence &&
                     decoded.checksum == snapshot.checksum &&
                     decoded.serverTick == snapshot.serverTick,
                 "round_trip")) {
        return 8;
    }

    bytes.back() ^= 0x01U;
    if (!Require(!bridge.DecodeSnapshot(bytes, decoded), "reject_corrupt_snapshot") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::DecodeFailed,
                 "corrupt_snapshot_error") ||
        !Require(decoded.count == 0U && decoded.states.empty(),
                 "failed_decode_clears_output")) {
        return 9;
    }

    std::printf("CANONICAL_REPLICATION_BRIDGE_SMOKE_OK count=2 codec=1 corruption=1 reservedId=1\n");
    return 0;
}
