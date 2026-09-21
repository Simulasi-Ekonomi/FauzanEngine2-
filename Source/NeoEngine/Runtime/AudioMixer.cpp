#include "AudioMixer.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <limits>

namespace NeoEngine {
namespace {
float Dot3(const float a[3], const float b[3]) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
float Length3(const float v[3]) { return std::sqrt(Dot3(v,v)); }
bool Normalize3(float v[3]) { const float length=Length3(v); if(!std::isfinite(length)||length<=1.0e-5f)return false; v[0]/=length;v[1]/=length;v[2]/=length;return true; }
}

bool AudioMixer::Play(uint32_t id,std::vector<int16_t> samples,uint16_t gainQ8,bool looping,float pitch) {
    if (m_Voices.size()>kMaxVoices) { m_Voices.clear(); return false; }
    if(id==0||samples.empty()||samples.size()>kMaxSamplesPerVoice||gainQ8==0||!std::isfinite(pitch)||pitch<=0.001f||pitch>8.0f)return false;
    for(const auto& voice:m_Voices)if(voice.id==id)return false;
    if(m_Voices.size()>=kMaxVoices||m_Voices.size()==std::numeric_limits<size_t>::max())return false;
    Voice voice; voice.id=id;voice.samples=std::move(samples);voice.gain=gainQ8;voice.looping=looping;voice.pitch=pitch;voice.spatialized=false;
    try { m_Voices.push_back(std::move(voice)); } catch (...) { return false; }
    return m_Voices.size()<=kMaxVoices;
}

bool AudioMixer::PlaySpatial(const SpatialVoiceParams& params) {
    if (m_Voices.size()>kMaxVoices) { m_Voices.clear(); return false; }
    if(params.id==0||params.mono.empty()||params.mono.capacity()<params.mono.size()||params.mono.size()>kMaxSamplesPerVoice||params.gainQ8==0||!std::isfinite(params.pitch)||params.pitch<=0.001f||params.pitch>8.0f)return false;
    for(const auto& voice:m_Voices)if(voice.id==params.id)return false;
    if(m_Voices.size()>=kMaxVoices)return false;
    for(float x:params.position)if(!std::isfinite(x))return false;
    const float dx=params.position[0]-m_Listener.position[0],dy=params.position[1]-m_Listener.position[1],dz=params.position[2]-m_Listener.position[2];
    const float distance=std::sqrt(dx*dx+dy*dy+dz*dz); if(!std::isfinite(distance))return false;
    if(!std::isfinite(params.attenuation.minDistance)||!std::isfinite(params.attenuation.maxDistance)||!std::isfinite(params.attenuation.minVolume))return false;
    if(params.attenuation.minDistance<0.0f||params.attenuation.maxDistance<=0.0f||params.attenuation.maxDistance<params.attenuation.minDistance||params.attenuation.minVolume<0.0f||params.attenuation.minVolume>1.0f)return false;
    const float minD=std::max(0.001f,params.attenuation.minDistance),maxD=std::max(minD+0.001f,params.attenuation.maxDistance);
    const float minVolume=std::clamp(params.attenuation.minVolume,0.0f,1.0f);
    float attenuation=1.0f;
    if(params.spatialized){if(distance>=maxD)attenuation=minVolume;else if(distance>minD){const float t=(distance-minD)/(maxD-minD);if(!std::isfinite(t))return false;attenuation=params.attenuation.model==AudioAttenuationModel::Linear?1.0f-t:1.0f/(1.0f+t*t*(maxD/minD));attenuation=std::max(minVolume,attenuation);}}
    float pan=0.0f;
    if(params.spatialized&&distance>0.001f){float forward[3]{m_Listener.forward[0],m_Listener.forward[1],m_Listener.forward[2]},up[3]{m_Listener.up[0],m_Listener.up[1],m_Listener.up[2]};if(Normalize3(forward)&&Normalize3(up)){const float fu=Dot3(forward,up);up[0]-=forward[0]*fu;up[1]-=forward[1]*fu;up[2]-=forward[2]*fu;if(Normalize3(up)){const float right[3]{up[1]*forward[2]-up[2]*forward[1],up[2]*forward[0]-up[0]*forward[2],up[0]*forward[1]-up[1]*forward[0]};pan=std::clamp((dx*right[0]+dy*right[1]+dz*right[2])/distance,-1.0f,1.0f);}}}
    if(!std::isfinite(pan)||!std::isfinite(attenuation)||attenuation<0.0f||attenuation>1.0f)return false;
    if(params.gainQ8==0)return false;
    Voice voice;voice.id=params.id;voice.samples=params.mono;voice.gain=params.gainQ8;voice.pan=pan;voice.pitch=params.pitch;voice.position[0]=params.position[0];voice.position[1]=params.position[1];voice.position[2]=params.position[2];voice.attenuation=params.attenuation;voice.looping=params.looping;voice.spatialized=params.spatialized;
    try { m_Voices.push_back(std::move(voice)); } catch (...) { return false; }
    return m_Voices.size()<=kMaxVoices;
}

bool AudioMixer::UpdateVoicePosition(uint32_t id,const float position[3]) { if(id==0||position==nullptr||m_Voices.size()>kMaxVoices)return false;for(float value:{position[0],position[1],position[2]})if(!std::isfinite(value))return false;for(auto& voice:m_Voices)if(voice.id==id){voice.position[0]=position[0];voice.position[1]=position[1];voice.position[2]=position[2];return true;}return false; }
bool AudioMixer::UpdateVoicePitch(uint32_t id,float pitch) { if(id==0||m_Voices.size()>kMaxVoices||!std::isfinite(pitch)||pitch<=0.001f||pitch>8.0f)return false;for(auto& voice:m_Voices)if(voice.id==id){voice.pitch=pitch;return true;}return false; }
bool AudioMixer::UpdateVoiceGain(uint32_t id,uint16_t gainQ8) { if(id==0||m_Voices.size()>kMaxVoices||gainQ8==0)return false;for(auto& voice:m_Voices)if(voice.id==id){voice.gain=gainQ8;return true;}return false; }
bool AudioMixer::Stop(uint32_t id) { if(id==0||m_Voices.size()>kMaxVoices)return false;auto it=std::find_if(m_Voices.begin(),m_Voices.end(),[&](const auto& voice){return voice.id==id;});if(it==m_Voices.end())return false;m_Voices.erase(it);return true; }
void AudioMixer::Clear(){m_Voices.clear();}

bool AudioMixer::SetListener(const AudioListener& listener) {
    for(float value:listener.position)if(!std::isfinite(value))return false;
    for(float value:listener.forward)if(!std::isfinite(value))return false;
    for(float value:listener.up)if(!std::isfinite(value))return false;
    float forward[3]{listener.forward[0],listener.forward[1],listener.forward[2]},up[3]{listener.up[0],listener.up[1],listener.up[2]};
    if(!Normalize3(forward)||!Normalize3(up))return false;
    const float orthogonality=std::fabs(Dot3(forward,up));if(!std::isfinite(orthogonality)||orthogonality>0.999f)return false;
    AudioListener canonical = listener;
    canonical.forward[0]=forward[0]; canonical.forward[1]=forward[1]; canonical.forward[2]=forward[2];
    canonical.up[0]=up[0]; canonical.up[1]=up[1]; canonical.up[2]=up[2];
    m_Listener=canonical;return true;
}

void AudioMixer::Mix(size_t frames,std::vector<int16_t>& out) {
    if(frames>kMaxMixFrames||frames>std::numeric_limits<size_t>::max()/2U||m_Voices.size()>kMaxVoices){out.clear();return;}
    try{out.assign(frames*2U,0);}catch(...){out.clear();return;}
    if (out.capacity() < out.size() || out.size() > kMaxMixFrames * 2U) { out.clear(); return; }
    if(out.size()!=frames*2U){out.clear();return;}
    m_Voices.erase(std::remove_if(m_Voices.begin(),m_Voices.end(),[](const auto& voice){
        return voice.id==0U || voice.samples.empty() || voice.samples.size()>kMaxSamplesPerVoice || !std::isfinite(voice.cursorSubframe) ||
               !std::isfinite(voice.pitch) || voice.pitch<=0.001F || voice.pitch>8.0F || !std::isfinite(voice.pan) || voice.pan<-1.0F || voice.pan>1.0F;
    }),m_Voices.end());
    if (m_Voices.size()>kMaxVoices) { m_Voices.clear(); out.clear(); return; }
    for(size_t f=0;f<frames;++f){
        if (m_Voices.size()>kMaxVoices) { out.clear(); return; }
        int64_t left=0,right=0;
        for(auto& voice:m_Voices){
            if(voice.samples.empty()||voice.samples.size()>kMaxSamplesPerVoice||!std::isfinite(voice.cursorSubframe)||!std::isfinite(voice.pitch)||voice.pitch<=0.001f||voice.pitch>8.0f)continue;
            if(voice.cursorSubframe>=static_cast<double>(voice.samples.size())){if(voice.looping)voice.cursorSubframe=std::fmod(voice.cursorSubframe,static_cast<double>(voice.samples.size()));else continue;}
            if(!std::isfinite(voice.cursorSubframe)||voice.cursorSubframe<0.0)continue;
            const double position=voice.cursorSubframe;const size_t idx0=static_cast<size_t>(position);if(idx0>=voice.samples.size())continue;const size_t idx1=idx0+1U<voice.samples.size()?idx0+1U:(voice.looping?0U:idx0);const double frac=position-static_cast<double>(idx0);
            const int32_t s0=voice.samples[idx0],s1=voice.samples[idx1];const double interpolatedValue=static_cast<double>(s0)+frac*static_cast<double>(s1-s0);if(!std::isfinite(interpolatedValue))continue;const int32_t interpolated=static_cast<int32_t>(std::llround(interpolatedValue));
            float dynamicGain=static_cast<float>(voice.gain)/256.0f,dynamicPan=voice.pan;
            if(voice.spatialized){
                if(!std::isfinite(voice.attenuation.minDistance)||!std::isfinite(voice.attenuation.maxDistance)||!std::isfinite(voice.attenuation.minVolume)||voice.attenuation.minDistance<0.0f||voice.attenuation.maxDistance<=0.0f||voice.attenuation.maxDistance<voice.attenuation.minDistance||voice.attenuation.minVolume<0.0f||voice.attenuation.minVolume>1.0f)continue;
                const float dx=voice.position[0]-m_Listener.position[0],dy=voice.position[1]-m_Listener.position[1],dz=voice.position[2]-m_Listener.position[2];const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);if(!std::isfinite(distance))continue;
                const float minD=std::max(0.001f,voice.attenuation.minDistance),maxD=std::max(minD+0.001f,voice.attenuation.maxDistance),minVolume=std::clamp(voice.attenuation.minVolume,0.0f,1.0f);float attenuation=1.0f;
                if(distance>=maxD)attenuation=minVolume;else if(distance>minD){const float t=(distance-minD)/(maxD-minD);if(!std::isfinite(t))continue;if(voice.attenuation.model==AudioAttenuationModel::Linear)attenuation=1.0f-t;else if(voice.attenuation.model==AudioAttenuationModel::Logarithmic)attenuation=1.0f-std::log10(1.0f+9.0f*t);else attenuation=1.0f/(1.0f+t*t*(maxD/minD));attenuation=std::max(minVolume,attenuation);}dynamicGain*=attenuation;
                if(distance>0.001f){float forward[3]{m_Listener.forward[0],m_Listener.forward[1],m_Listener.forward[2]},up[3]{m_Listener.up[0],m_Listener.up[1],m_Listener.up[2]};if(Normalize3(forward)&&Normalize3(up)){const float fu=Dot3(forward,up);up[0]-=forward[0]*fu;up[1]-=forward[1]*fu;up[2]-=forward[2]*fu;if(Normalize3(up)){const float rightAxis[3]{up[1]*forward[2]-up[2]*forward[1],up[2]*forward[0]-up[0]*forward[2],up[0]*forward[1]-up[1]*forward[0]};dynamicPan=std::clamp((dx*rightAxis[0]+dy*rightAxis[1]+dz*rightAxis[2])/distance,-1.0f,1.0f);}}}
            }
            if(!std::isfinite(dynamicGain)||!std::isfinite(dynamicPan)||dynamicGain<0.0f||dynamicGain>256.0f||dynamicPan<-1.0f||dynamicPan>1.0f)continue;
            const double scaledSample=static_cast<double>(interpolated)*static_cast<double>(dynamicGain);if(!std::isfinite(scaledSample)||scaledSample>static_cast<double>(std::numeric_limits<int64_t>::max())||scaledSample<static_cast<double>(std::numeric_limits<int64_t>::min()))continue;
            const double roundedSample=std::llround(scaledSample); if(!std::isfinite(roundedSample)) continue; const int64_t sample=static_cast<int64_t>(roundedSample);const double nextCursor=voice.cursorSubframe+voice.pitch;
            if (nextCursor > static_cast<double>(std::numeric_limits<size_t>::max())) continue;if(!std::isfinite(nextCursor)||nextCursor<voice.cursorSubframe)continue;voice.cursorSubframe=nextCursor;
            if(voice.cursorSubframe>static_cast<double>(std::numeric_limits<size_t>::max()))continue;
            if (voice.cursorSubframe < 0.0) continue;voice.cursor=static_cast<size_t>(voice.cursorSubframe);
            if(!voice.spatialized){if ((sample>0 && left>std::numeric_limits<int64_t>::max()-sample) || (sample<0 && left<std::numeric_limits<int64_t>::min()-sample)) continue; if ((sample>0 && right>std::numeric_limits<int64_t>::max()-sample) || (sample<0 && right<std::numeric_limits<int64_t>::min()-sample)) continue; left+=sample;right+=sample;}else{const float pan=std::clamp(dynamicPan,-1.0f,1.0f),angle=(pan+1.0f)*0.5f*1.57079632679489661923f;if(!std::isfinite(angle))continue;const float lg=std::cos(angle),rg=std::sin(angle);if(!std::isfinite(lg)||!std::isfinite(rg))continue;const double leftValue=static_cast<double>(sample)*lg,rightValue=static_cast<double>(sample)*rg;if(!std::isfinite(leftValue)||!std::isfinite(rightValue))continue;if(leftValue>static_cast<double>(std::numeric_limits<int64_t>::max())||leftValue<static_cast<double>(std::numeric_limits<int64_t>::min())||rightValue>static_cast<double>(std::numeric_limits<int64_t>::max())||rightValue<static_cast<double>(std::numeric_limits<int64_t>::min()))continue;left+=static_cast<int64_t>(std::llround(leftValue));right+=static_cast<int64_t>(std::llround(rightValue));}
        }
        out[f*2U]=static_cast<int16_t>(std::clamp<int64_t>(left,-32768,32767));out[f*2U+1U]=static_cast<int16_t>(std::clamp<int64_t>(right,-32768,32767));
    }
    if (m_Voices.size()>kMaxVoices) { m_Voices.clear(); out.clear(); return; }
    m_Voices.erase(std::remove_if(m_Voices.begin(),m_Voices.end(),[](const auto& voice){return !voice.looping&&!voice.samples.empty()&&voice.cursorSubframe>=static_cast<double>(voice.samples.size());}),m_Voices.end());
}
} // namespace NeoEngine
