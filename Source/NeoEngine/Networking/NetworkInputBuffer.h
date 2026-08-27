#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct InputCommand { uint64_t sequence{}; uint64_t clientTick{}; float x{},y{},z{}; uint32_t buttons{}; };

class InputCommandBuffer {
public:
    static constexpr uint16_t Capacity=256;
    bool push(const InputCommand& command){
        if(!command.sequence||command.sequence<=lastSequence_||count_>=Capacity)return false;
        commands_[(head_+count_)%Capacity]=command;++count_;lastSequence_=command.sequence;return true;
    }
    bool pop(InputCommand& command){if(!count_)return false;command=commands_[head_];head_=(head_+1)%Capacity;--count_;return true;}
    bool acknowledge(uint64_t sequence){if(sequence>lastSequence_)return false;lastAck_=sequence;while(count_&&commands_[head_].sequence<=sequence){head_=(head_+1)%Capacity;--count_;}return true;}
    [[nodiscard]] uint16_t pending()const{return count_;}
    [[nodiscard]] uint64_t lastSequence()const{return lastSequence_;}
    [[nodiscard]] uint64_t lastAck()const{return lastAck_;}
private: std::array<InputCommand,Capacity> commands_{};uint16_t head_{},count_{};uint64_t lastSequence_{},lastAck_{};
};
}
