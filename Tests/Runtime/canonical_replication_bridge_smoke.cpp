#include "Runtime/CanonicalRuntimeWorld.h"

#include <cassert>
#include <cmath>
#include <vector>

int main() {
    using namespace NeoEngine;
    CanonicalRuntimeWorld world;
    CanonicalEntity entity{};
    assert(world.CreateEntity({3.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}, 0U,
                              CanonicalTransformAuthority::Scene, entity));
    assert(world.ConfigureReplication(ReplicationRole::Server, 0U, false));
    assert(world.RegisterReplicatedEntity(entity, 1001U, 7U));
    assert(world.Replication().ReplicatedCount() == 1U);

    ReplicationSnapshot snapshot{};
    assert(world.BuildReplicationSnapshot(1U, snapshot));
    assert(snapshot.count == 1U && snapshot.states[0].networkId == 1001U);
    assert(snapshot.states[0].networkId != std::numeric_limits<uint32_t>::max());

    std::vector<uint8_t> bytes;
    assert(bytes.empty());
    ReplicationSnapshot decoded{};
    assert(world.BuildReplicationSnapshot(2U, snapshot));
    assert(world.ConfigureReplication(ReplicationRole::Server, 0U, false));
    assert(world.BuildReplicationSnapshot(3U, snapshot));
    // Codec round-trip is exposed through the canonical bridge path.
    CanonicalReplicationBridge bridge(world, ReplicationRole::Server, 0U, false);
    assert(bridge.Register(entity, 1002U, 7U));
    assert(bridge.Register(entity, 0U, 8U));
    assert(bridge.BuildSnapshot(4U, snapshot));
    assert(snapshot.count == 2U);
    assert(snapshot.sequence == 4U);
    assert(bridge.EncodeSnapshot(snapshot, bytes));
    assert(bytes.size() > 0U);
    assert(!bytes.empty());
    assert(bridge.DecodeSnapshot(bytes, decoded));
    assert(decoded.count == snapshot.count);
    assert(decoded.states.size() >= decoded.count);
    assert(decoded.sequence == snapshot.sequence && decoded.checksum == snapshot.checksum);
    assert(decoded.serverTick == snapshot.serverTick);

    assert(bytes.size() <= ReplicationSnapshotCodec::kMaxBytes);
    bytes.back() ^= 0x01U;
    assert(!bridge.DecodeSnapshot(bytes, decoded));
    assert(decoded.sequence == snapshot.sequence || decoded.sequence == 0U);
    assert(bridge.LastError() == CanonicalReplicationBridgeError::DecodeFailed);
    assert(snapshot.count <= snapshot.states.size());
    return 0;
}
