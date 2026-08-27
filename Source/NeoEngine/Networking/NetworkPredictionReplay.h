#pragma once
#include <cstdint>
#include "NetworkInputBuffer.h"
#include "NetworkReconciliation.h"

namespace NeoEngine::Networking {

class PredictionReplay {
public:
    template<class SimulateFn>
    static uint32_t replay(InputCommandBuffer& inputs, uint64_t acknowledged, PredictedState& state, SimulateFn&& simulate){
        InputCommand command{}; uint32_t replayed=0;
        while(inputs.pop(command)){
            if(command.sequence<=acknowledged)continue;
            simulate(command,state);++replayed;
        }
        return replayed;
    }
};
}
