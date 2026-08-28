#include <cmath>
#include "Networking/NetworkActorAdapter.h"
#include "Networking/NetworkAuthoritativeActor.h"
#include "Networking/NetworkAuthorityGate.h"
#include "Networking/NetworkCommandGuard.h"
#include "Networking/NetworkInterestRelevancy.h"
#include "Networking/NetworkReconnectPolicy.h"
#include "Networking/NetworkReliableChannel.h"
#include "Networking/NetworkReplicationCoordinator.h"
#include "Networking/NetworkRpcDispatcher.h"
#include "Networking/NetworkSessionReconnection.h"
#include "Networking/NetworkSessionResume.h"
#include "Networking/NetworkSnapshotDelta.h"
#include "Networking/NetworkStateSmoothing.h"
#include "Networking/NetworkStressHarness.h"
#include "Networking/NetworkTickScheduler.h"
#include <cstdio>
#include <vector>

using namespace NeoEngine;
using namespace NeoEngine::Networking;

namespace {
bool Require(bool value, const char* label) {
    if (!value) std::fprintf(stderr, "NETWORK_EXTENDED_SMOKE_FAIL step=%s\n", label);
    return value;
}
bool RpcHandler(const RpcEnvelope&) { return true; }
}

int main() {
    AuthorityGate gate;
    gate.reset(7, 11);
    AuthorityCommand command{7, 11, 1, 2, 32};
    if (!Require(gate.validate(command).accepted, "authority_accept") ||
        !Require(gate.validate(command).reason == AuthorityReject::InvalidSequence, "authority_duplicate") ||
        !Require(gate.validate({8, 11, 2, 2, 32}).reason == AuthorityReject::InvalidPeer, "authority_peer_reject") ||
        !Require(gate.validate({7, 12, 2, 2, 32}).reason == AuthorityReject::NotOwner, "authority_owner_reject") ||
        !Require(gate.validate({7, 11, 2, 2, AuthorityGate::MaxPayloadBytes + 1}).reason == AuthorityReject::PayloadTooLarge, "authority_payload_reject")) return 1;

    NetworkCommandGuard guard;
    NetworkCommandKey key{7, 1, 9};
    if (!Require(guard.accept(key), "guard_accept") || !Require(!guard.accept(key), "guard_duplicate") ||
        !Require(guard.contains(key), "guard_contains") || !Require(guard.discardThrough(7, 1) == 1, "guard_discard")) return 2;

    ActorBase serverActor;
    serverActor.SetActorLocation(Vector3(3, 4, 5));
    SnapshotState captured{};
    AuthorityContext authority{100, 7, 42};
    if (!Require(NetworkActorAdapter::capture(serverActor, 50, 7, 3, captured), "actor_capture") ||
        !Require(captured.x == 3 && captured.y == 4 && captured.z == 5, "actor_capture_position") ||
        !Require(!NetworkActorAdapter::capture(serverActor, 0, 7, 3, captured), "actor_invalid_id")) return 3;
    ActorBase clientActor;
    if (!Require(AuthoritativeActorBridge::captureServerState(serverActor, 50, authority, captured), "bridge_capture") ||
        !Require(AuthoritativeActorBridge::applyClientState(captured, clientActor, authority), "bridge_apply") ||
        !Require(clientActor.GetActorLocation().x == 3, "bridge_position") ||
        !Require(!AuthoritativeActorBridge::applyClientState(captured, clientActor, AuthorityContext{100, 8, 42}), "bridge_owner_reject")) return 4;
    AuthoritativeActorBridge::advanceRevision();
    if (!Require(!AuthoritativeActorBridge::applyClientState({50, 7, 0, 1, 1, 1}, clientActor, authority), "bridge_stale_reject")) return 5;

    InterestRelevancy relevancy(5.0F);
    if (!Require(relevancy.relevant({0, 0, 0}, {1, {3, 4, 0}, 2, false}, 2), "relevancy_inside") ||
        !Require(!relevancy.relevant({0, 0, 0}, {2, {3, 4, 0}, 3, false}, 2), "relevancy_team") ||
        !Require(relevancy.relevant({0, 0, 0}, {3, {100, 0, 0}, 3, true}, 2), "relevancy_always") ||
        !Require(!relevancy.relevant({0, 0, 0}, {0, {0, 0, 0}, 0, false}), "relevancy_invalid")) return 6;

    ReplicationCoordinator coordinator;
    ReplicationEntity entities[2]{{1, 0, 0, 0, 2, 7, 1}, {2, 20, 0, 0, 2, 8, 1}};
    ReplicationPlan plan{};
    if (!Require(coordinator.addClient(7, 0, 0, 0, 5, 1), "coordinator_add") ||
        !Require(coordinator.buildPlan(7, entities, 2, plan) == 1 && plan.entities[0] == 1, "coordinator_plan") ||
        !Require(coordinator.updateView(7, 20, 0, 0, 5, 2), "coordinator_update") ||
        !Require(coordinator.buildPlan(7, entities, 2, plan) == 1 && plan.entities[0] == 2, "coordinator_replan") ||
        !Require(coordinator.removeClient(7) && coordinator.clients() == 0, "coordinator_remove")) return 7;

    RpcDispatcher dispatcher;
    if (!Require(dispatcher.registerRpc(10, RpcDirection::ClientToServer, &RpcHandler), "rpc_register") ||
        !Require(dispatcher.dispatch({7, 11, 1, 10, RpcDirection::ClientToServer, 4}).accepted, "rpc_dispatch") ||
        !Require(dispatcher.dispatch({7, 11, 1, 10, RpcDirection::ClientToServer, 4}).reason == RpcDispatchResult::Reason::DuplicateSequence, "rpc_duplicate") ||
        !Require(dispatcher.dispatch({7, 11, 2, 10, RpcDirection::ServerToClient, 4}).reason == RpcDispatchResult::Reason::DirectionDenied, "rpc_direction")) return 8;

    SessionReconnection reconnect(2);
    if (!Require(reconnect.transition(ReconnectionEvent::Begin), "reconnect_begin") ||
        !Require(reconnect.transition(ReconnectionEvent::Connected), "reconnect_connected") ||
        !Require(reconnect.transition(ReconnectionEvent::ConnectionLost), "reconnect_lost") ||
        !Require(reconnect.transition(ReconnectionEvent::RetryTimer), "reconnect_retry") ||
        !Require(reconnect.transition(ReconnectionEvent::RetryFailed), "reconnect_failed") ||
        !Require(reconnect.state() == ReconnectionState::Backoff, "reconnect_backoff")) return 9;

    SessionResumeState resume;
    if (!Require(resume.establish(99, 7), "resume_establish") || !Require(resume.acknowledge(5), "resume_ack") ||
        !Require(resume.canResume({99, 7, 4}), "resume_valid") || !Require(!resume.canResume({99, 8, 4}), "resume_peer_reject") ||
        !Require(!resume.acknowledge(3), "resume_ack_regress")) return 10;

    SnapshotState before{1, 7, 1, 1, 2, 3};
    SnapshotState after{1, 8, 2, 4, 2, 6};
    const SnapshotDelta delta = SnapshotDeltaCodec::makeDelta(before, after);
    SnapshotState applied = before;
    if (!Require(delta.mask == (Position | Owner | Revision), "delta_mask") ||
        !Require(SnapshotDeltaCodec::apply(delta, applied) && applied.ownerId == 8 && applied.x == 4 && applied.revision == 2, "delta_apply") ||
        !Require(SnapshotDeltaCodec::build(&before, &after, 1).size() == 1, "delta_build")) return 11;

    const auto interpolated = NetworkStateSmoother::interpolate({1, 1.0, 0, 0, 0}, {2, 3.0, 4, 0, 0}, 2.0);
    const auto extrapolated = NetworkStateSmoother::extrapolate({1, 1.0, 0, 0, 0}, {2, 2.0, 2, 0, 0}, 2.25);
    if (!Require(std::fabs(interpolated.x - 2.0F) < 0.001F && std::fabs(interpolated.y) < 0.001F, "smoothing_interpolate") ||
        !Require(std::fabs(extrapolated.x - 2.5F) < 0.001F && std::fabs(extrapolated.y) < 0.001F, "smoothing_extrapolate")) return 12;

    NetworkTickScheduler scheduler(60);
    if (!Require(scheduler.advance(1.0 / 30.0) == 2 && scheduler.tick() == 2, "tick_advance") ||
        !Require(scheduler.advance(-1.0) == 0, "tick_negative") || !Require(scheduler.rate() == 60, "tick_rate")) return 13;

    const auto stress = StressHarness::run({4, 20, 0.0F, 0.0F, 1234U});
    if (!Require(stress.packets == 80 && stress.accepted == 80 && stress.dropped == 0 && stress.duplicated == 0, "stress_deterministic") ||
        !Require(StressHarness::run({0, 1, 0, 0, 1}).packets == 0, "stress_invalid")) return 14;

    ReliableChannel reliable;
    if (!Require(reliable.enqueue(10), "reliable_enqueue") || !Require(reliable.inFlight() == 1, "reliable_inflight") ||
        !Require(reliable.shouldAccept(1), "reliable_accept") || !Require(reliable.receive(1), "reliable_receive") ||
        !Require(reliable.acknowledge(1), "reliable_ack") || !Require(reliable.inFlight() == 0, "reliable_complete")) return 15;

    std::puts("NETWORK_EXTENDED_SMOKE_OK authority=1 actor=1 relevancy=1 coordinator=1 rpc=1 reconnect=1 resume=1 delta=1 smoothing=1 tick=1 stress=1 reliable=1");
    return 0;
}
