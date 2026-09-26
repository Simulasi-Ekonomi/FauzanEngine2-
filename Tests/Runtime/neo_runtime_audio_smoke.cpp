#include "Runtime/SdlAudioBridge.h"
#include "Runtime/AudioComponent.h"
#include "Runtime/WavAudioParser.h"

#include <SDL3/SDL.h>
#include <cassert>
#include <limits>

int main() {
    using namespace NeoEngine;

    SdlAudioBridge audio;
    assert(audio.Initialize(128));
    assert(audio.IsReady());
    assert(audio.LastError() == SdlAudioBridgeError::None);
    assert(audio.QueuedVoiceCount() == 0U);
    assert(audio.FramesMixed() == 0U);

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
    voice.SetGainQ8(256);
    voice.SetSpatialized(true);
    voice.SetPosition(2.0f, 0.0f, 0.0f);

    assert(audio.Play(voice.Id(), voice.Samples(), voice.GainQ8(), voice.IsLooping(), voice.Pitch()));
    assert(audio.QueuedVoiceCount() == 1U);

    const float movedPosition[3]{4.0f, 0.0f, 0.0f};
    assert(audio.UpdateVoicePosition(101, movedPosition));
    assert(audio.UpdateVoicePitch(101, 0.75f));
    assert(audio.UpdateVoiceGain(101, 192));

    AudioListener listener{};
    listener.position[0] = 1.0f;
    assert(audio.SetListener(listener));

    const float invalidPosition[3]{0.0f, std::numeric_limits<float>::quiet_NaN(), 0.0f};
    assert(!audio.UpdateVoicePosition(101, invalidPosition));
    assert(audio.QueuedVoiceCount() == 1U);

    SDL_Delay(40);
    assert(audio.FramesMixed() > 0U);
    assert(audio.Stop(101));
    assert(audio.QueuedVoiceCount() == 0U);

    audio.Reset();
    assert(!audio.IsReady());
    return 0;
}
