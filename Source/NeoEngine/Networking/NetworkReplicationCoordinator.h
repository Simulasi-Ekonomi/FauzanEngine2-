#pragma once
#include <array>
#include <cstdint>
#include "NetworkPeerLifecycle.h"
#include "NetworkEntityLifecycle.h"
#include "NetworkReplicationPolicy.h"
#include "NetworkSnapshotDelta.h"

namespace NeoEngine::Networking {

struct ReplicationClientView { uint32_t peerId{}; float x{},y{},z{}; float interestRadius{100.0F}; uint64_t lastSnapshot{}; };
struct ReplicationPlan { uint32_t peerId{}; uint16_t entityCount{}; std::array<uint32_t,ReplicationPolicy::MaxCandidates> entities{}; };

class ReplicationCoordinator {
public:
    static constexpr uint16_t MaxClients=64;
    static constexpr uint16_t MaxEntities=1024;

    bool addClient(uint32_t peerId,float x,float y,float z,float radius,uint64_t tick){if(!peers_.connect(peerId,tick)||!peers_.establish(peerId,tick))return false;for(auto&v:views_)if(!v.peerId){v={peerId,x,y,z,radius,0};++clients_;return true;}return false;}
    bool removeClient(uint32_t peerId){for(auto&v:views_)if(v.peerId==peerId){v={};if(clients_)--clients_;peers_.remove(peerId);return true;}return false;}
    bool updateView(uint32_t peerId,float x,float y,float z,float radius,uint64_t tick){for(auto&v:views_)if(v.peerId==peerId){v.x=x;v.y=y;v.z=z;if(radius>0)v.interestRadius=radius;v.lastSnapshot=tick;return peers_.receive(peerId,tick);}return false;}
    uint16_t buildPlan(uint32_t peerId,const ReplicationEntity* entities,uint16_t count,ReplicationPlan& out){for(auto const&v:views_)if(v.peerId==peerId){ReplicationEntity viewer{0,v.x,v.y,v.z,0,peerId,0};out.peerId=peerId;out.entityCount=0;uint32_t ids[ReplicationPolicy::MaxCandidates]{};if(!ReplicationPolicy::buildInterest(viewer,entities,count,v.interestRadius,ids,out.entityCount))return 0;for(uint16_t i=0;i<out.entityCount;++i)out.entities[i]=ids[i];return out.entityCount;}return 0;}
    uint16_t timeout(uint64_t tick){return peers_.timeout(tick);}
    uint16_t clients()const{return clients_;}
    const PeerLifecycle& peerLifecycle()const{return peers_;}
private:
    PeerLifecycle peers_{};std::array<ReplicationClientView,MaxClients> views_{};uint16_t clients_{};
};
}
