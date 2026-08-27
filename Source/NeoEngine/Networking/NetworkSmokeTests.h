#pragma once
#include <cassert>
#include <cstdint>
#include "NetworkReliableWindow.h"
#include "NetworkHandshake.h"
#include "NetworkPeerLifecycle.h"
#include "NetworkEntityLifecycle.h"
#include "NetworkPacketFrame.h"
#include "NetworkConnectionState.h"
#include "NetworkInterpolation.h"
#include "NetworkExtrapolation.h"
#include "NetworkInputBuffer.h"
#include "NetworkReconciliation.h"
#include "NetworkInputAuthority.h"
#include "NetworkPredictionReplay.h"
#include "NetworkRpc.h"
#include "NetworkOwnership.h"
#include "NetworkReplicatedProperty.h"
#include "NetworkReplicationBudget.h"
#include "NetworkReplicationScheduler.h"
#include "NetworkLagCompensation.h"
#include "NetworkTimeSync.h"

namespace NeoEngine::Networking::SmokeTests {
inline void run(){
    ReliableWindow rw; assert(rw.queue(1,1,10)); assert(rw.acknowledge(1)); assert(rw.pending()==0);
    HandshakeMachine hs; assert(hs.begin(1,42)); HandshakeChallenge ch{1,42,7}; assert(hs.acceptChallenge(ch)); assert(hs.state()==HandshakeState::Established);
    PeerLifecycle pl; assert(pl.connect(1,1)); assert(pl.establish(1,1)); assert(pl.receive(1,2)); assert(pl.count()==1);
    NetworkEntityLifecycle el; assert(el.spawn(1,1,1)); assert(el.confirmSpawn(1)); assert(el.bumpRevision(1));
    PacketFrame pf; PacketHeader ph; ph.type=PacketType::Snapshot; ph.sequence=1; ph.payloadBytes=3; uint8_t in[3]={1,2,3}; assert(pf.encode(ph,in,3)); uint8_t out[3]{}; uint16_t n=0; PacketHeader decoded{}; assert(pf.decode(decoded,out,3,n)&&n==3&&out[1]==2);
    ConnectionStateMachine cs; assert(cs.transition(ConnectionEvent::Begin)); assert(cs.transition(ConnectionEvent::ChallengeAccepted)); assert(cs.state()==ConnectionState::Connected);
    SnapshotInterpolator si; assert(si.push({1,0,0,0})); assert(si.push({3,2,0,0})); InterpolatedTransform it{}; assert(si.sample(2,it)&&it.x==1.0F);
    auto ex=SnapshotExtrapolator::predict({1,0,0,0},{2,1,0,0},4); assert(ex.valid&&ex.x==3.0F);
    InputCommandBuffer ib; assert(ib.push({1,1,1,0,0,0})); InputCommand ic{};
 assert(ib.pop(ic)&&ic.sequence==1);
    ReconciliationBuffer rb; assert(rb.record({1,1,0,0})); PredictedState corrected{}; assert(rb.reconcile(1,{1,2,0,0},corrected)&&corrected.x==2);
    AuthoritativeInputValidator av; auto vr=av.validate({2,2,0.5F,0,0,0},3,1); assert(vr.accepted);
    InputCommandBuffer replayInputs; assert(replayInputs.push({1,1,1,0,0,0})); assert(replayInputs.push({2,2,1,0,0,0}));
 PredictedState ps{1,0,0,0}; auto sim=[](const InputCommand&c,PredictedState&s){s.x+=c.x;s.inputSequence=c.sequence;}; assert(PredictionReplay::replay(replayInputs,1,ps,sim)==1&&ps.x==1);
    RpcRegistry rr; auto handler=[](const RpcRequest&){return true;}; assert(rr.registerRpc(1,RpcDirection::ClientToServer,handler)); assert(rr.dispatch({1,1,1,1},RpcDirection::ClientToServer)); assert(!rr.dispatch({1,1,1,1},RpcDirection::ServerToClient));
    assert(OwnershipPolicy::canWrite(AuthorityRole::Server,99,1)); assert(OwnershipPolicy::canWrite(AuthorityRole::Owner,1,1)); assert(!OwnershipPolicy::canWrite(AuthorityRole::Owner,2,1));
    ReplicatedPropertySet props; assert(props.define(1,ReplicatedType::Float,1)); assert(props.setFloat(1,2.0F,1)); uint16_t ids[4]{}; assert(props.collectDirty(ids,4)==1&&ids[0]==1);
    ReplicationBudget budget(100); assert(budget.reserve(40)); assert(!budget.reserve(61));
    ReplicationCandidate candidates[2]={{1,20,10,0},{2,20,5,0}}; uint32_t selected[2]{}; ReplicationScheduler sched; assert(sched.schedule(candidates,2,budget,selected,2,10)==2);
    LagCompensationHistory lh; assert(lh.record(1,1,0,0,1)); assert(lh.record(2,2,0,0,2)); HistoricalTransform ht{}; assert(lh.sample(2,ht)&&ht.x==2);
    NetworkClock clock; clock.observe({10,11,0.033}); assert(clock.serverTickEstimate(10)>=10);
}
}
