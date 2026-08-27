#include "Runtime/NetworkSession.h"

#include <cassert>
#include <cmath>

using namespace NeoEngine;

int main() {
    NetworkSession server(NetworkRole::Server, 9000U);
    NetworkSession client(NetworkRole::Client, 42U);
    assert(server.initialize());
    assert(client.initialize());
    assert(server.registerPeer(42U));
    assert(server.assignOwnership(7U, 42U));

    NetworkInputCommand input{42U, 7U, 1U, 1U, 0.5F, 0.25F};
    assert(client.predict(input));

    NetworkTransformState authoritative{};
    assert(server.serverConsume(input, authoritative));
    assert(authoritative.networkId == 7U);
    assert(authoritative.ownerId == 42U);
    assert(authoritative.revision == 1U);
    assert(std::fabs(authoritative.x - 0.5F) < 0.0001F);
    assert(std::fabs(authoritative.z - 0.25F) < 0.0001F);

    NetworkReconciliationReceipt receipt{};
    assert(client.reconcile(authoritative, 1U, receipt));
    assert(receipt.authoritativeRevision == 1U);
    assert(receipt.acknowledgedInput == 1U);
    assert(!receipt.reconciled || receipt.replayedInputs == 0U);

    NetworkInputCommand duplicate = input;
    assert(!server.serverConsume(duplicate, authoritative));

    NetworkInputCommand forged{99U, 7U, 2U, 2U, 0.1F, 0.1F};
    assert(!server.serverConsume(forged, authoritative));

    NetworkInputCommand invalid{42U, 7U, 3U, 3U, 2.0F, 0.0F};
    assert(!server.serverConsume(invalid, authoritative));

    auto snapshot = server.snapshot(10U);
    assert(snapshot.sequence == 1U);
    assert(snapshot.states.size() == 1U);
    assert(client.acceptSnapshot(snapshot));
    assert(!client.acceptSnapshot(snapshot));

    return 0;
}
