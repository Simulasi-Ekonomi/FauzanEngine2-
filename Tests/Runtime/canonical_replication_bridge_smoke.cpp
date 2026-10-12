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
    CanonicalEntity first{}, second{};
    if (!Require(world.CreateEntity(
                     {3.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F},
                     0U, CanonicalTransformAuthority::Scene, first),
                 "create_first_entity") ||
        !Require(world.CreateEntity(
                     {8.0F, 0.0F, 9.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F},
                     0U, CanonicalTransformAuthority::Scene, second),
                 "create_second_entity")) {
        return 1;
    }

    CanonicalReplicationBridge bridge(world, ReplicationRole::Server, 0U, false);
    if (!Require(bridge.Register(first, 1002U, 7U), "register_first") ||
        !Require(bridge.Register(second, 1003U, 8U), "register_second") ||
        !Require(!bridge.Register(first, 0U, 8U), "reject_zero_network_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidNetworkId,
                 "zero_network_id_error") ||
        !Require(!bridge.Register(first, std::numeric_limits<uint32_t>::max(), 8U),
                 "reject_max_network_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidNetworkId,
                 "max_network_id_error") ||
        !Require(!bridge.Register(first, 1004U, std::numeric_limits<uint32_t>::max()),
                 "reject_reserved_owner_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidOwnerId,
                 "owner_id_error") ||
        !Require(!bridge.Unregister(0U), "reject_zero_unregister_id") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::InvalidNetworkId,
                 "zero_unregister_id_error") ||
        !Require(bridge.Replication().RegisteredCount() == 2U,
                 "invalid_ids_do_not_mutate_registry")) {
        return 2;
    }

    ReplicationSnapshot snapshot{};
    if (!Require(bridge.BuildSnapshot(4U, snapshot), "build_snapshot") ||
        !Require(snapshot.count == 2U &&
                     snapshot.sequence != std::numeric_limits<uint64_t>::max() &&
                     snapshot.serverTick == 4U && snapshot.checksum != 0U,
                 "snapshot_content")) {
        return 3;
    }

    std::vector<uint8_t> bytes;
    if (!Require(bridge.EncodeSnapshot(snapshot, bytes), "encode_snapshot") ||
        !Require(!bytes.empty() && bytes.size() <= ReplicationSnapshotCodec::kMaxBytes,
                 "encoded_size")) {
        return 4;
    }

    ReplicationSnapshot decoded{};
    if (!Require(bridge.DecodeSnapshot(bytes, decoded), "decode_snapshot") ||
        !Require(decoded.count == snapshot.count && decoded.states.size() >= decoded.count &&
                     decoded.states[0].networkId == 1002U && decoded.states[0].ownerId == 7U &&
                     decoded.states[1].networkId == 1003U && decoded.states[1].ownerId == 8U &&
                     decoded.sequence == snapshot.sequence &&
                     decoded.checksum == snapshot.checksum &&
                     decoded.serverTick == snapshot.serverTick,
                 "round_trip")) {
        return 5;
    }

    bytes.back() ^= 0x01U;
    if (!Require(!bridge.DecodeSnapshot(bytes, decoded), "reject_corrupt_snapshot") ||
        !Require(bridge.LastError() == CanonicalReplicationBridgeError::DecodeFailed,
                 "corrupt_snapshot_error") ||
        !Require(decoded.count == 0U && decoded.states[0].networkId == 0U &&
                     decoded.states[0].ownerId == 0U,
                 "failed_decode_clears_output")) {
        return 6;
    }

    std::printf("CANONICAL_REPLICATION_BRIDGE_SMOKE_OK count=2 codec=1 corruption=1 reservedId=1\n");
    return 0;
}
