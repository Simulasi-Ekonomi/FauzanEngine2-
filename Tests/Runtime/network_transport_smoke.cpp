#include "Runtime/NetworkTransport.h"

#include <array>
#include <cstdio>

using namespace NeoEngine;

namespace {
bool Require(bool condition, const char* label) {
    if (!condition) std::fprintf(stderr, "NETWORK_TRANSPORT_SMOKE_FAIL step=%s\n", label);
    return condition;
}
} // namespace

int main() {
    NetworkTransportQueue queue;
    const uint8_t payloadA[] = {1U, 2U, 3U};
    const uint8_t payloadB[] = {4U, 5U};
    std::array<uint8_t, NetworkTransportQueue::kMaxPayloadBytes + 1U> oversized{};

    if (!Require(!queue.Enqueue(0U, NetworkDelivery::Unreliable, payloadA), "invalid_peer_reject") ||
        !Require(!queue.Enqueue(42U, NetworkDelivery::Unreliable, oversized), "oversized_payload_reject") ||
        !Require(queue.Size() == 0U && queue.Stats().rejected == 2U, "rejection_atomic")) return 1;

    if (!Require(queue.Enqueue(42U, NetworkDelivery::ReliableOrdered, payloadA), "enqueue_a") ||
        !Require(queue.Enqueue(42U, NetworkDelivery::Unreliable, payloadB), "enqueue_b") ||
        !Require(queue.Size() == 2U, "size_two") || !Require(queue.ReorderTailPair(), "reorder")) return 1;

    NetworkDatagram packet{};
    if (!Require(queue.Dequeue(packet), "dequeue_first") || !Require(packet.peerId == 42U && packet.sequence == 2U, "dequeue_reordered") ||
        !Require(queue.Dequeue(packet), "dequeue_second") || !Require(packet.sequence == 1U, "dequeue_original")) return 1;

    if (!Require(queue.Enqueue(42U, NetworkDelivery::Unreliable, payloadA), "enqueue_duplicate_source") ||
        !Require(queue.DuplicateTail(), "duplicate") || !Require(queue.Size() == 2U, "duplicate_size") ||
        !Require(queue.DropNewest(), "drop_newest") || !Require(queue.Size() == 1U, "drop_size") ||
        !Require(queue.Stats().duplicated == 1U && queue.Stats().dropped == 1U && queue.Stats().reordered == 1U, "fault_stats")) return 1;

    const NetworkInterestVolume volume{0.0F, 0.0F, 0.0F, 10.0F};
    if (!Require(NetworkInterestFilter::IsRelevant(volume, 3.0F, 4.0F, 0.0F), "interest_inside") ||
        !Require(!NetworkInterestFilter::IsRelevant(volume, 11.0F, 0.0F, 0.0F), "interest_outside")) return 1;

    std::printf("NETWORK_TRANSPORT_SMOKE_OK queue=1 reorder=1 duplicate=1 drop=1 interest=1 bounded_payload=1 rejection_stats=1\n");
    return 0;
}
