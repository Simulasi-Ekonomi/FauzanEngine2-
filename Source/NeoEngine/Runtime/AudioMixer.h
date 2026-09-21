#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class AudioAttenuationModel : uint8_t { Linear, InverseSquare, Logarithmic };
struct AudioAttenuation { AudioAttenuationModel model = AudioAttenuationModel::InverseSquare; float minDistance = 1.0f; float maxDistance = 100.0f; float minVolume = 0.0f; };
struct AudioListener { float position[3]{0,0,0}; float forward[3]{0,0,1}; float up[3]{0,1,0}; };
struct SpatialVoiceParams { uint32_t id=0; std::vector<int16_t> mono; bool spatialized=false; float position[3]{0,0,0}; AudioAttenuation attenuation{}; uint16_t gainQ8=256; bool looping=false; float pitch=1.0f; };
class AudioMixer {
public:
 static constexpr size_t kMaxVoices=32,kMaxSamplesPerVoice=480000,kMaxMixFrames=48000;
 // The optional looping/pitch parameters are additive: existing callers retain the original API
 // behavior while components can request looping without forcing spatialization.
 bool Play(uint32_t id,std::vector<int16_t> mono,uint16_t gainQ8=256,bool looping=false,float pitch=1.0f);
 bool PlaySpatial(const SpatialVoiceParams& params);
    bool UpdateVoicePosition(uint32_t id, const float position[3]);
    bool UpdateVoicePitch(uint32_t id, float pitch);
    bool UpdateVoiceGain(uint32_t id, uint16_t gainQ8);
 bool Stop(uint32_t id);
 void Clear();
 bool SetListener(const AudioListener& listener);
 void Mix(size_t frames,std::vector<int16_t>& stereo);
 size_t ActiveVoices()const{return m_Voices.size();}
private:
 struct Voice {
     uint32_t id=0;
     std::vector<int16_t> samples;
     size_t cursor=0;
     double cursorSubframe=0.0;
     uint16_t gain=256;
     float pan=0.0f;
     float pitch=1.0f;
     float position[3]{0.0f,0.0f,0.0f};
     AudioAttenuation attenuation{};
     bool looping=false;
     bool spatialized=false;
 };
 std::vector<Voice>m_Voices;
 AudioListener m_Listener{};
}; }
