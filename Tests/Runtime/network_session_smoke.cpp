#include "Runtime/NetworkSession.h"

#include <cmath>
#include <cstdio>

using namespace NeoEngine;

namespace {
bool Require(bool condition, const char* label) {
    if (!condition) std::fprintf(stderr, "NETWORK_SESSION_SMOKE_FAIL step=%s\n", label);
    return condition;
}
} // namespace

int main() {
    NetworkSession invalidPeer(NetworkRole::Client, 0U);
    if (!Require(!invalidPeer.initialize(), "invalid_peer_reject") || !Require(invalidPeer.lastError() == NetworkError::InvalidPeer, "invalid_peer_error")) return 1;

    NetworkSession server(NetworkRole::Server, 9000U);
    NetworkSession client(NetworkRole::Client, 42U);
    if (!Require(server.initialize(), "server_init") || !Require(server.lastError() == NetworkError::None, "server_init_error") ||
        !Require(client.initialize(), "client_init") || !Require(client.lastError() == NetworkError::None, "client_init_error") ||
        !Require(server.registerPeer(42U), "register_peer") || !Require(server.lastError() == NetworkError::None, "register_peer_error") ||
        !Require(server.assignOwnership(7U, 42U), "ownership") || !Require(server.lastError() == NetworkError::None, "ownership_error")) return 1;

    NetworkInputCommand input{42U, 7U, 1U, 1U, 0.5F, 0.25F};
    if (!Require(client.predict(input), "predict")) return 1;

    NetworkTransformState authoritative{};
    if (!Require(server.serverConsume(input, authoritative), "server_consume") || !Require(server.lastError() == NetworkError::None, "server_consume_error") ||
        !Require(authoritative.networkId == 7U && authoritative.ownerId == 42U && authoritative.revision == 1U, "authoritative_identity") ||
        !Require(std::fabs(authoritative.x - 0.5F) < 0.0001F && std::fabs(authoritative.z - 0.25F) < 0.0001F, "authoritative_transform")) return 1;

    NetworkReconciliationReceipt receipt{};
    if (!Require(client.reconcile(authoritative, 1U, receipt), "reconcile") || !Require(client.lastError() == NetworkError::None, "reconcile_error") ||
        !Require(receipt.authoritativeRevision == 1U && receipt.acknowledgedInput == 1U, "reconcile_receipt") ||
        !Require(!receipt.reconciled || receipt.replayedInputs == 0U, "reconcile_replay")) return 1;

    if (!Require(!server.serverConsume(input, authoritative), "duplicate_reject") || !Require(server.lastError() == NetworkError::Duplicate, "duplicate_error")) return 1;
    NetworkInputCommand forged{99U, 7U, 2U, 2U, 0.1F, 0.1F};
    if (!Require(!server.serverConsume(forged, authoritative), "ownership_reject") || !Require(server.lastError() == NetworkError::Ownership, "ownership_reject_error")) return 1;
    NetworkInputCommand invalid{42U, 7U, 3U, 3U, 2.0F, 0.0F};
    if (!Require(!server.serverConsume(invalid, authoritative), "magnitude_reject") || !Require(server.lastError() == NetworkError::InvalidInput, "magnitude_error")) return 1;

    const NetworkSnapshot snapshot = server.snapshot(10U);
    if (!Require(snapshot.sequence == 1U && snapshot.states.size() == 1U, "snapshot") ||
        !Require(client.acceptSnapshot(snapshot), "snapshot_accept") || !Require(client.lastError() == NetworkError::None, "snapshot_accept_error") ||
        !Require(!client.acceptSnapshot(snapshot), "snapshot_replay_reject") || !Require(client.lastError() == NetworkError::Duplicate, "snapshot_replay_error")) return 1;

    NetworkSession saturated(NetworkRole::Client, 42U);
    if (!Require(saturated.initialize(), "saturated_init")) return 1;
    NetworkInputCommand saturatedInput{42U, 7U, 0U, 0U, 1.0F, 0.0F};
    for (uint64_t sequence = 1U; sequence <= NetworkPredictionBuffer::MaxFrames; ++sequence) {
        saturatedInput.sequence = sequence;
        if (!Require(saturated.predict(saturatedInput), "prediction_capacity_fill")) return 1;
    }
    saturatedInput.sequence = NetworkPredictionBuffer::MaxFrames + 1U;
    if (!Require(!saturated.predict(saturatedInput), "prediction_capacity_reject") || !Require(saturated.lastError() == NetworkError::Capacity, "prediction_capacity_error")) return 1;
    NetworkReconciliationReceipt saturatedReceipt{};
    if (!Require(saturated.reconcile({7U, 42U, 0U, 0.0F, 0.0F, 0.0F}, 0U, saturatedReceipt), "prediction_capacity_reconcile") ||
        !Require(std::fabs(saturatedReceipt.correctionDistance - 256.0F * 256.0F) < 0.01F, "prediction_capacity_atomic")) return 1;

    std::printf("NETWORK_SESSION_SMOKE_OK prediction=1 authority=1 reconcile=1 duplicate_reject=1 snapshot=1 capacity_atomic=1\n");
    return 0;
}
