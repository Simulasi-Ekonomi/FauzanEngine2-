#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct PredictedState { uint64_t inputSequence{}; float x{},y{},z{}; };

class ReconciliationBuffer {
public:
    static constexpr uint16_t Capacity=256;
    bool record(const PredictedState& state){if(!state.inputSequence)return false;states_[(head_+count_)%Capacity]=state;if(count_<Capacity)++count_;else head_=(head_+1)%Capacity;return true;}
    bool find(uint64_t sequence,PredictedState& out)const{for(uint16_t i=0;i<count_;++i){auto const&s=states_[(head_+i)%Capacity];if(s.inputSequence==sequence){out=s;return true;}}return false;}
    bool reconcile(uint64_t acknowledgedSequence,const PredictedState& authoritative,PredictedState& corrected){if(!acknowledgedSequence||authoritative.inputSequence!=acknowledgedSequence)return false;corrected=authoritative;return true;}
    uint16_t discardThrough(uint64_t sequence){uint16_t n=0;while(count_&&states_[head_].inputSequence<=sequence){head_=(head_+1)%Capacity;--count_;++n;}return n;}
    [[nodiscard]] uint16_t pending()const{return count_;}
private: std::array<PredictedState,Capacity> states_{};uint16_t head_{},count_{};
};
}
