#include "Runtime/AudioComponent.h"
#include "Runtime/AudioMixer.h"
#include "Runtime/WavAudioParser.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;

    const auto wav = WavAudioParser::GenerateSyntheticWav(48000, 1, 256, 440.0f);
    WavAudioData decoded;
    assert(WavAudioParser::Parse(wav, decoded));
    assert(decoded.sampleRate == 48000 && decoded.channels == 1 && decoded.pcmSamples.size() == 256);

    auto truncated = wav;
    truncated.pop_back();
    assert(!WavAudioParser::Parse(truncated, decoded));

    AudioMixer mixer;
    AudioComponent component(1);
    assert(component.SetSamples(decoded.pcmSamples));
    component.SetGainQ8(256);
    assert(component.Play(mixer));
    std::vector<int16_t> output;
    mixer.Mix(64, output);
    assert(output.size() == 128);
    assert(mixer.ActiveVoices() == 0);

    AudioComponent spatial(2);
    assert(spatial.SetSamples(decoded.pcmSamples));
    spatial.SetSpatialized(true);
    spatial.SetPosition(2.0f, 0.0f, 0.0f);
    spatial.SetLooping(true);
    assert(spatial.Play(mixer));
    mixer.Mix(64, output);
    assert(output.size() == 128 && mixer.ActiveVoices() == 1);
    assert(spatial.Stop(mixer));

    return 0;
}
