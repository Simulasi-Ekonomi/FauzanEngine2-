#pragma once
#include <cstdint>
#include <vector>
namespace NeoEngine { class AudioMixer{public:static constexpr size_t kMaxVoices=32,kMaxSamplesPerVoice=480000;bool Play(uint32_t id,std::vector<int16_t>mono,uint16_t gainQ8=256);bool Stop(uint32_t id);void Mix(size_t frames,std::vector<int16_t>&stereo);size_t ActiveVoices()const{return m_Voices.size();}private:struct Voice{uint32_t id;std::vector<int16_t>samples;size_t cursor=0;uint16_t gain=256;};std::vector<Voice>m_Voices;};}
