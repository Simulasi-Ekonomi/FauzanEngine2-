#include "Networking/NetworkRpcDispatcher.h"

#include <cstdio>

using namespace NeoEngine::Networking;

namespace {
bool Handler(const RpcEnvelope&) { return true; }

bool Require(bool condition, const char* label) {
    if (!condition) std::fprintf(stderr, "NETWORK_RPC_SMOKE_FAIL step=%s\n", label);
    return condition;
}
} // namespace

int main() {
    RpcDispatcher dispatcher;
    if (!Require(dispatcher.registerRpc(7U, RpcDirection::ClientToServer, &Handler), "register")) return 1;

    const RpcEnvelope peerOne{1U, 0U, 1U, 7U, RpcDirection::ClientToServer, 0U};
    const RpcEnvelope peerTwo{2U, 0U, 1U, 7U, RpcDirection::ClientToServer, 0U};
    if (!Require(dispatcher.dispatch(peerOne).accepted, "peer_one_first") ||
        !Require(dispatcher.dispatch(peerTwo).accepted, "peer_two_first") ||
        !Require(dispatcher.dispatch(peerOne).reason == RpcDispatchResult::Reason::DuplicateSequence, "peer_one_duplicate")) return 1;

    dispatcher.clearPeer(1U);
    if (!Require(dispatcher.dispatch(peerOne).accepted, "peer_one_cleared") ||
        !Require(dispatcher.dispatch(peerTwo).reason == RpcDispatchResult::Reason::DuplicateSequence, "peer_two_preserved")) return 1;

    std::printf("NETWORK_RPC_SMOKE_OK peer_clear_isolated=1 duplicate_protection=1\n");
    return 0;
}
