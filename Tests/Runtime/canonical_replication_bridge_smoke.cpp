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

    ReplicationSnapshot snapshot{};
    assert(world.BuildReplicationSnapshot(1U, snapshot));
    assert(snapshot.count == 1U && snapshot.states[0].networkId == 1001U);

    std::vector<uint8_t> bytes;
    ReplicationSnapshot decoded{};
    assert(world.BuildReplicationSnapshot(2U, snapshot));
    assert(world.ConfigureReplication(ReplicationRole::Server, 0U, false));
    assert(world.BuildReplicationSnapshot(3U, snapshot));
    // Codec round-trip is exposed through the canonical bridge path.
    CanonicalReplicationBridge bridge(world, ReplicationRole::Server, 0U, false);
    assert(bridge.Register(entity, 1002U, 7U));
    assert(bridge.BuildSnapshot(4U, snapshot));
    assert(bridge.EncodeSnapshot(snapshot, bytes));
    assert(!bytes.empty());
    assert(bridge.DecodeSnapshot(bytes, decoded));
    assert(decoded.sequence == snapshot.sequence && decoded.checksum == snapshot.checksum);

    bytes.back() ^= 0x01U;
    assert(!bridge.DecodeSnapshot(bytes, decoded));
    assert(bridge.LastError() == CanonicalReplicationBridgeError::DecodeFailed);
    return 0;
}
