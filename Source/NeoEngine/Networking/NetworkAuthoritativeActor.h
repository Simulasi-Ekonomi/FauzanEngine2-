#pragma once
#include <cstdint>
#include "NetworkActorAdapter.h"
#include "NetworkEntityLifecycle.h"
#include "NetworkLagCompensation.h"

namespace NeoEngine::Networking {

struct AuthorityContext { uint32_t serverPeerId{}; uint32_t ownerPeerId{}; uint64_t serverTick{}; };

class AuthoritativeActorBridge {
public:
    static bool captureServerState(const NeoEngine::ActorBase& actor,uint32_t networkId,const AuthorityContext& authority,SnapshotState& out){
        if(!authority.serverPeerId||!networkId)return false;
        return NetworkActorAdapter::capture(actor,networkId,authority.ownerPeerId,revision_,out);
    }
    static bool applyClientState(const SnapshotState& state,NeoEngine::ActorBase& actor,const AuthorityContext& authority){
        if(!authority.serverPeerId||state.ownerId!=authority.ownerPeerId)return false;
        if(state.revision<revision_)return false;
        revision_=state.revision;
        return NetworkActorAdapter::apply(state,actor);
    }
    static void advanceRevision(){++revision_;}
    static uint64_t revision(){return revision_;}
private: inline static uint64_t revision_{};
};
}
