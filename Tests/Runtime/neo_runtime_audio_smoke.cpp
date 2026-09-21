#include "Runtime/NeoRuntime.h"
#include "Runtime/AudioComponent.h"
#include "Runtime/WavAudioParser.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    using namespace NeoEngine;

    RuntimeConfig config{};
    config.enableAudio = true;
    config.audioFramesPerCallback = 128;

    NeoRuntime runtime;
    assert(runtime.Initialize(config));
    assert(runtime.State() == RuntimeState::Initialized);
    assert(runtime.Audio() != nullptr);
    assert(runtime.Audio()->LastError() == AudioMixerError::None);
    assert(runtime.Audio()->IsReady());
    assert(runtime.Audio()->QueuedVoiceCount() == 0U);
    assert(runtime.Audio()->FramesMixed() == 0U);

    const auto wav = WavAudioParser::GenerateSyntheticWav(48000, 1, 440.0f, 0.05f);
    WavAudioData decoded;
    assert(WavAudioParser::Parse(wav, decoded));
    assert(decoded.sampleRate == 48000U);
    assert(decoded.channels == 1U);
    assert(!decoded.pcmSamples.empty());

    AudioComponent voice(101);
    assert(voice.SetSamples(decoded.pcmSamples));
    assert(voice.SetPitch(1.25f));
    voice.SetLooping(true);
    voice.SetPitch(1.25f);
    voice.SetGainQ8(256);
    voice.SetSpatialized(true);
    voice.SetPosition(2.0f, 0.0f, 0.0f);
    assert(runtime.PlayAudio(voice));
    assert(runtime.Audio()->QueuedVoiceCount() == 1U);
    assert(runtime.Audio()->QueuedVoiceCount() == 1U);

    const float movedPosition[3]{4.0f, 0.0f, 0.0f};
    assert(runtime.UpdateAudioPosition(101, movedPosition));
    assert(runtime.UpdateAudioPitch(101, 0.75f));
    assert(runtime.UpdateAudioGain(101, 192));
    assert(runtime.UpdateAudioPitch(101, 0.75f));
    assert(runtime.UpdateAudioGain(101, 192));

    AudioListener listener{};
    listener.position[0] = 1.0f;
    assert(runtime.SetAudioListener(listener));
    assert(std::isfinite(listener.position[0]));

    float invalidPosition[3]{0.0f, std::numeric_limits<float>::quiet_NaN(), 0.0f};
    assert(!runtime.UpdateAudioPosition(101, invalidPosition));
    assert(runtime.Audio()->QueuedVoiceCount() == 1U);

    SDL_Delay(40);
    assert(runtime.Audio()->FramesMixed() > 0U);
    assert(runtime.StopAudio(voice));
    assert(runtime.Audio()->QueuedVoiceCount() == 0U);
    assert(runtime.Audio()->QueuedVoiceCount() == 0U);
    assert(runtime.Shutdown());
    return 0;
}
