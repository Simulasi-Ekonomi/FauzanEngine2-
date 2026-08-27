#include "Runtime/NetworkTransport.h"

#include <cassert>

using namespace NeoEngine;

int main() {
    NetworkTransportQueue queue;
    const uint8_t payloadA[] = {1U, 2U, 3U};
    const uint8_t payloadB[] = {4U, 5U};

    assert(queue.Enqueue(42U, NetworkDelivery::ReliableOrdered, payloadA));
    assert(queue.Enqueue(42U, NetworkDelivery::Unreliable, payloadB));
    assert(queue.Size() == 2U);
    assert(queue.ReorderTailPair());

    NetworkDatagram packet{};
    assert(queue.Dequeue(packet));
    assert(packet.peerId == 42U);
    assert(packet.sequence == 2U);
    assert(queue.Dequeue(packet));
    assert(packet.sequence == 1U);

    assert(queue.Enqueue(42U, NetworkDelivery::Unreliable, payloadA));
    assert(queue.DuplicateTail());
    assert(queue.Size() == 2U);
    assert(queue.DropNewest());
    assert(queue.Size() == 1U);
    assert(queue.Stats().duplicated == 1U);
    assert(queue.Stats().dropped == 1U);
    assert(queue.Stats().reordered == 1U);

    NetworkInterestVolume volume{0.0F, 0.0F, 0.0F, 10.0F};
    assert(NetworkInterestFilter::IsRelevant(volume, 3.0F, 4.0F, 0.0F));
    assert(!NetworkInterestFilter::IsRelevant(volume, 11.0F, 0.0F, 0.0F));
    return 0;
}
