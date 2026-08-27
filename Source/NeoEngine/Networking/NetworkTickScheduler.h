#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

class NetworkTickScheduler {
public:
    explicit NetworkTickScheduler(uint32_t tickRate=60):tickRate_(tickRate?tickRate:60),interval_(1.0/double(tickRate_)){}
    uint32_t advance(double elapsed){
        if(elapsed<0.0)return 0; accumulator_+=elapsed; uint32_t ticks=0;
        while(accumulator_>=interval_ && ticks<MaxCatchUp){accumulator_-=interval_;++tick_;++ticks;}
        if(ticks==MaxCatchUp && accumulator_>=interval_) accumulator_=interval_;
        return ticks;
    }
    [[nodiscard]] uint64_t tick()const{return tick_;}
    [[nodiscard]] double alpha()const{return accumulator_/interval_;}
    [[nodiscard]] uint32_t rate()const{return tickRate_;}
private:
    static constexpr uint32_t MaxCatchUp=8;
    uint32_t tickRate_; double interval_; double accumulator_{}; uint64_t tick_{};
};

struct AckState { uint64_t latestInput{}; uint64_t latestSnapshot{}; uint64_t latestServerTick{}; };
}
