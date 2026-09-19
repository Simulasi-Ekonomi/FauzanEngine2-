#include "Runtime/AudioComponent.h"
#include "Runtime/AudioMixer.h"
#include "Runtime/WavAudioParser.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;

    const auto wav = WavAudioParser::GenerateSyntheticWav(48000, 1, 440.0f, 0.01f);
    WavAudioData decoded;
    assert(WavAudioParser::Parse(wav, decoded));
    assert(decoded.sampleRate == 48000 && decoded.channels == 1 && decoded.pcmSamples.size() == 480);

    std::vector<uint8_t> wav8;
    wav8.insert(wav8.end(), {'R','I','F','F'});
    wav8.insert(wav8.end(), {40,0,0,0});
    wav8.insert(wav8.end(), {'W','A','V','E','f','m','t',' '});
    wav8.insert(wav8.end(), {16,0,0,0,1,0,1,0,0x40,0x1f,0,0,0x40,0x1f,0,0,1,0,8,0});
    wav8.insert(wav8.end(), {'d','a','t','a',4,0,0,0,0,64,128,255});
    WavAudioData decoded8;
    assert(WavAudioParser::Parse(wav8, decoded8));
    assert(decoded8.pcmSamples.size() == 4 && decoded8.pcmSamples.front() < 0 && decoded8.pcmSamples.back() > 0);
    const auto decodedSamples = decoded.pcmSamples;

    auto truncated = wav;
    truncated.pop_back();
    assert(!WavAudioParser::Parse(truncated, decoded));

    AudioMixer mixer;
    AudioComponent component(1);
    assert(component.SetSamples(decodedSamples));
    component.SetGainQ8(256);
    component.SetPitch(1.5f);
    assert(component.Play(mixer));
    std::vector<int16_t> output;
    mixer.Mix(64, output);
    assert(output.size() == 128);
    assert(mixer.ActiveVoices() == 1);
    mixer.Clear();

    AudioComponent spatial(2);
    assert(spatial.SetSamples(decodedSamples));
    spatial.SetSpatialized(true);
    spatial.SetPosition(2.0f, 0.0f, 0.0f);
    spatial.SetLooping(true);
    spatial.SetPitch(0.75f);
    assert(!([&] { SpatialVoiceParams invalid; invalid.id = 3; invalid.mono = decodedSamples; invalid.pitch = 0.0f; return mixer.PlaySpatial(invalid); })());
    assert(spatial.Play(mixer));
    mixer.Mix(64, output);
    assert(output.size() == 128 && mixer.ActiveVoices() == 1);
    const float moved[3]{8.0f, 0.0f, 0.0f};
    assert(mixer.UpdateVoicePosition(2, moved));
    assert(mixer.UpdateVoicePitch(2, 1.25f));
    assert(mixer.UpdateVoiceGain(2, 192));
    assert(spatial.Stop(mixer));

    return 0;
}
