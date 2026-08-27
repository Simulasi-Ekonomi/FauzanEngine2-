#pragma once
#include "Core/ActorBase.h"
#include "NetworkEntityLifecycle.h"
#include "NetworkSnapshotDelta.h"

namespace NeoEngine::Networking {

class NetworkActorAdapter {
public:
    static bool capture(const NeoEngine::ActorBase& actor, uint32_t networkId, uint32_t ownerId, uint64_t revision, SnapshotState& out) {
        if (!networkId) return false;
        const auto p = actor.GetActorLocation();
        out = {networkId, ownerId, revision, p.x, p.y, p.z};
        return true;
    }

    static bool apply(const SnapshotState& state, NeoEngine::ActorBase& actor) {
        if (!state.networkId) return false;
        actor.SetActorLocation(NeoEngine::Vector3(state.x, state.y, state.z));
        return true;
    }
};

} // namespace NeoEngine::Networking
