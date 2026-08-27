#pragma once
#include <cstdint>
#include "NetworkInputBuffer.h"

namespace NeoEngine::Networking {

struct InputValidationConfig { float maxAxisMagnitude{1.0F}; uint32_t allowedButtons{0xFFFFFFFFu}; };
struct ValidatedInput { InputCommand command{}; bool accepted{}; };

class AuthoritativeInputValidator {
public:
    explicit AuthoritativeInputValidator(InputValidationConfig config={}):config_(config){}
    ValidatedInput validate(const InputCommand& input,uint64_t serverTick,uint64_t lastAccepted) const {
        if(!input.sequence||input.sequence<=lastAccepted||input.clientTick>serverTick+MaxFutureTicks)return {};
        if(input.buttons&~config_.allowedButtons)return {};
        const float magnitude2=input.x*input.x+input.y*input.y+input.z*input.z;
        if(!(magnitude2>=0.0F)&&magnitude2==magnitude2)return {};
        if(magnitude2>config_.maxAxisMagnitude*config_.maxAxisMagnitude)return {};
        return {input,true};
    }
private:
    static constexpr uint64_t MaxFutureTicks=2;
    InputValidationConfig config_;
};
}
