#include "Runtime/AudioMixer.h"
#include <cstdio>
using namespace NeoEngine;int main(){AudioMixer m;std::vector<int16_t>out;bool ok=m.Play(1,{1000,1000})&&m.Play(2,{2000,2000})&&!m.Play(1,{1})&&m.ActiveVoices()==2;m.Mix(2,out);ok=ok&&out.size()==4&&out[0]==3000&&out[1]==3000&&m.ActiveVoices()==0&&!m.Stop(1);if(!ok)return 1;std::printf("AUDIO_MIXER_SMOKE_OK frames=%zu\n",out.size()/2);}
