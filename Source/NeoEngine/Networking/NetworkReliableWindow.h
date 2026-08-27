#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct ReliablePacket { uint64_t sequence{}; uint32_t peerId{}; uint16_t bytes{}; uint8_t attempts{}; bool acknowledged{}; };
struct ReliableStats { uint64_t queued{}; uint64_t acknowledged{}; uint64_t retransmitted{}; uint64_t expired{}; uint64_t rejected{}; };

class ReliableWindow {
public:
    static constexpr uint16_t Capacity=256;
    static constexpr uint8_t MaxAttempts=8;

    bool queue(uint32_t peerId,uint64_t sequence,uint16_t bytes){
        if(!peerId||!sequence||!bytes||count_>=Capacity){++stats_.rejected;return false;}
        slots_[(head_+count_)%Capacity]={sequence,peerId,bytes,1,false};++count_;++stats_.queued;return true;
    }
    bool acknowledge(uint64_t sequence){
        for(uint16_t i=0;i<count_;++i){auto& p=slots_[(head_+i)%Capacity];if(p.sequence==sequence&&!p.acknowledged){p.acknowledged=true;++stats_.acknowledged;compact();return true;}}
        return false;
    }
    template<class Fn> uint32_t collectRetransmit(Fn&& send){
        uint32_t n=0;
        for(uint16_t i=0;i<count_;++i){auto& p=slots_[(head_+i)%Capacity];if(p.acknowledged)continue;if(p.attempts>=MaxAttempts){++stats_.expired;continue;}send(p);++p.attempts;++stats_.retransmitted;++n;}
        compact(); return n;
    }
    [[nodiscard]] uint16_t pending()const{return count_;}
    [[nodiscard]] const ReliableStats& stats()const{return stats_;}
private:
    void compact(){while(count_&&slots_[head_].acknowledged){head_=(head_+1)%Capacity;--count_;}}
    std::array<ReliablePacket,Capacity> slots_{};uint16_t head_{},count_{};ReliableStats stats_{};
};
}
