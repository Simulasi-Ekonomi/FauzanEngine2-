#include "Networking/NetworkInterestSet.h"
#include "Networking/NetworkReplicationPolicy.h"

#include <cstdio>

using namespace NeoEngine::Networking;

namespace {
bool Require(bool condition, const char* label) {
    if (!condition) std::fprintf(stderr, "NETWORK_POLICY_SMOKE_FAIL step=%s\n", label);
    return condition;
}
} // namespace

int main() {
    const ReplicationEntity viewer{1U, 0.0F, 0.0F, 0.0F, 1.0F, 42U, 1U};
    const ReplicationEntity entities[] = {
        {1U, 0.0F, 0.0F, 0.0F, 1.0F, 42U, 1U},
        {2U, 3.0F, 4.0F, 0.0F, 4.0F, 42U, 2U},
        {3U, 20.0F, 0.0F, 0.0F, 9.0F, 99U, 3U},
    };

    uint32_t interest[ReplicationPolicy::MaxCandidates]{};
    uint16_t interestCount = 0U;
    if (!Require(ReplicationPolicy::buildInterest(viewer, entities, 3U, 10.0F, interest, interestCount), "interest_build") ||
        !Require(interestCount == 2U && interest[0] == 1U && interest[1] == 2U, "interest_members")) return 1;

    ReplicationCandidate prioritized[3U]{};
    const uint16_t prioritizedCount = ReplicationPolicy::prioritize(viewer, entities, 3U, prioritized, 3U);
    if (!Require(prioritizedCount == 3U, "priority_count") || !Require(prioritized[0].networkId == 1U && prioritized[1].networkId == 2U && prioritized[2].networkId == 3U, "priority_order") ||
        !Require(ReplicationPolicy::prioritize(viewer, nullptr, 3U, prioritized, 3U) == 0U, "null_reject")) return 1;

    const ReplicationInterest candidates[] = {
        {11U, 0U, 4U, 10U, ReplicationPriority::High, false},
        {12U, 0U, 9U, 10U, ReplicationPriority::Normal, false},
    };
    NetworkInterestSet interestSet(42U);
    if (!Require(interestSet.rebuild(candidates, 2U), "interest_set_build") ||
        !Require(interestSet.size() == 2U && interestSet[0].actorId == 11U && interestSet[1].actorId == 12U, "interest_set_order") ||
        !Require(!interestSet.rebuild(nullptr, 1U), "interest_set_invalid_reject") ||
        !Require(interestSet.size() == 2U && interestSet[0].actorId == 11U && interestSet[1].actorId == 12U, "interest_set_atomic")) return 1;

    std::printf("NETWORK_POLICY_SMOKE_OK interest=2 prioritized=3 null_reject=1 interest_set_atomic=1\n");
    return 0;
}
