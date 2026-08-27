#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

enum class PeerState : uint8_t { Empty, Connecting, Established, Disconnecting, TimedOut };
struct PeerRecord { uint32_t peerId{}; PeerState state{PeerState::Empty}; uint64_t lastReceiveTick{}; uint64_t lastSendTick{}; uint32_t missedTicks{}; };

class PeerLifecycle {
public:
    static constexpr uint16_t Capacity=64;
    static constexpr uint32_t TimeoutTicks=180;
    bool connect(uint32_t id,uint64_t tick){auto*i=find(id);if(i){i->state=PeerState::Connecting;i->lastReceiveTick=tick;return true;}for(auto&p:peers_)if(p.state==PeerState::Empty){p={id,PeerState::Connecting,tick,tick,0};++count_;return true;}return false;}
    bool establish(uint32_t id,uint64_t tick){auto*p=find(id);if(!p||p->state!=PeerState::Connecting)return false;p->state=PeerState::Established;p->lastReceiveTick=tick;p->missedTicks=0;return true;}
    bool receive(uint32_t id,uint64_t tick){auto*p=find(id);if(!p||p->state!=PeerState::Established)return false;p->lastReceiveTick=tick;p->missedTicks=0;return true;}
    bool send(uint32_t id,uint64_t tick){auto*p=find(id);if(!p||p->state!=PeerState::Established)return false;p->lastSendTick=tick;return true;}
    uint16_t timeout(uint64_t tick){uint16_t n=0;for(auto&p:peers_)if(p.state==PeerState::Established&&tick>p.lastReceiveTick&&tick-p.lastReceiveTick>TimeoutTicks){p.state=PeerState::TimedOut;++n;}return n;}
    bool disconnect(uint32_t id){auto*p=find(id);if(!p)return false;p->state=PeerState::Disconnecting;return true;}
    bool remove(uint32_t id){auto*p=find(id);if(!p)return false;*p={};if(count_)--count_;return true;}
    const PeerRecord* get(uint32_t id)const{for(auto const&p:peers_)if(p.peerId==id)return &p;return nullptr;}
    uint16_t count()const{return count_;}
private: PeerRecord* find(uint32_t id){for(auto&p:peers_)if(p.peerId==id&&p.state!=PeerState::Empty)return &p;return nullptr;} const PeerRecord* find(uint32_t id)const{for(auto const&p:peers_)if(p.peerId==id&&p.state!=PeerState::Empty)return &p;return nullptr;} std::array<PeerRecord,Capacity> peers_{};uint16_t count_{};
};
}
