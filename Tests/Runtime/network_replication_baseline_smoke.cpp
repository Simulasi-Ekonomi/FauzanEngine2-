#include "Networking/NetworkReplicationBaseline.h"

#include <cstdio>

using namespace NeoEngine;
using namespace NeoEngine::Networking;

namespace {
bool Require(bool condition, const char* label) {
    if (!condition) std::fprintf(stderr, "NETWORK_BASELINE_SMOKE_FAIL step=%s\n", label);
    return condition;
}
} // namespace

int main() {
    NetworkReplicationBaseline history;
    ReplicationSnapshot first{};
    first.sequence = 1U;
    first.serverTick = 10U;
    first.count = 1U;
    first.states[0].networkId = 7U;
    first.states[0].stateRevision = 1U;
    if (!Require(history.store(first), "store_first") || !Require(history.has(1U), "find_first")) return 1;

    ReplicationSnapshot replacement = first;
    replacement.sequence = 9U;
    replacement.serverTick = 90U;
    replacement.states[0].stateRevision = 9U;
    if (!Require(history.store(replacement), "store_replacement") || !Require(!history.has(1U), "evict_old_slot") ||
        !Require(history.has(9U), "find_replacement")) return 1;

    ReplicationSnapshot preserved = replacement;
    if (!Require(!history.store(ReplicationSnapshot{}), "invalid_sequence_reject") ||
        !Require(history.find(9U, preserved) && preserved.serverTick == 90U && preserved.states[0].stateRevision == 9U, "invalid_preserves_state")) return 1;

    history.clear();
    if (!Require(!history.has(9U), "clear")) return 1;
    std::printf("NETWORK_BASELINE_SMOKE_OK bounded_history=1 overwrite=1 invalid_preserve=1 clear=1\n");
    return 0;
}
