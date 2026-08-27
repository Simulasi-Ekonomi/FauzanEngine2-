#pragma once
#include <cstdint>
#include <random>

namespace NeoEngine::Networking {

struct StressConfig { uint32_t clients{8}; uint32_t ticks{600}; float lossRate{0.05F}; float duplicateRate{0.01F}; uint32_t seed{0xF0A2E026U}; };
struct StressResult { uint64_t packets{}; uint64_t dropped{}; uint64_t duplicated{}; uint64_t accepted{}; uint64_t rejected{}; uint64_t timeouts{}; };

class StressHarness {
public:
    static StressResult run(const StressConfig& cfg){
        StressResult r{}; if(!cfg.clients||!cfg.ticks)return r;
        std::mt19937 rng(cfg.seed); std::uniform_real_distribution<float> d(0.0F,1.0F);
        for(uint32_t tick=0;tick<cfg.ticks;++tick){
            for(uint32_t client=1;client<=cfg.clients;++client){
                ++r.packets;
                if(d(rng)<cfg.lossRate){++r.dropped;continue;}
                ++r.accepted;
                if(d(rng)<cfg.duplicateRate)++r.duplicated;
            }
        }
        return r;
    }
};
}
