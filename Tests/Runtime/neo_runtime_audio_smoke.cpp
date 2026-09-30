#include "Runtime/SdlAudioBridge.h"
#include "Runtime/AudioComponent.h"
#include "Runtime/WavAudioParser.h"

#include <SDL3/SDL.h>
#include <limits>


#define REQUIRE(...) do { if (!(__VA_ARGS__)) return 1; } while (false)

int main() {
    using namespace NeoEngine;

    SdlAudioBridge audio;
    REQUIRE(audio.Initialize(128));
    REQUIRE(audio.IsReady());
    REQUIRE(audio.LastError() == SdlAudioBridgeError::None);
    REQUIRE(audio.QueuedVoiceCount() == 0U);
    REQUIRE(audio.FramesMixed() == 0U);

    const auto wav = WavAudioParser::GenerateSyntheticWav(48000, 1, 440.0f, 0.05f);
    WavAudioData decoded;
    REQUIRE(WavAudioParser::Parse(wav, decoded));
    REQUIRE(decoded.sampleRate == 48000U);
    REQUIRE(decoded.channels == 1U);
    REQUIRE(!decoded.pcmSamples.empty());

    AudioComponent voice(101);
    REQUIRE(voice.SetSamples(decoded.pcmSamples));
    voice.SetPitch(1.25f);
    voice.SetLooping(true);
    voice.SetGainQ8(256);
    voice.SetSpatialized(true);
    voice.SetPosition(2.0f, 0.0f, 0.0f);

    REQUIRE(voice.Play(audio));
    REQUIRE(audio.QueuedVoiceCount() == 1U);

    const float movedPosition[3]{4.0f, 0.0f, 0.0f};
    REQUIRE(audio.UpdateVoicePosition(101, movedPosition));
    REQUIRE(audio.UpdateVoicePitch(101, 0.75f));
    REQUIRE(audio.UpdateVoiceGain(101, 192));

    AudioListener listener{};
    listener.position[0] = 1.0f;
    REQUIRE(audio.SetListener(listener));

    const float invalidPosition[3]{0.0f, std::numeric_limits<float>::quiet_NaN(), 0.0f};
    REQUIRE(!audio.UpdateVoicePosition(101, invalidPosition));
    REQUIRE(audio.QueuedVoiceCount() == 1U);

    SDL_Delay(40);
    REQUIRE(audio.MixFrames(1024));
    REQUIRE(audio.FramesMixed() >= 1024U);
    REQUIRE(audio.Stop(101));
    REQUIRE(audio.QueuedVoiceCount() == 0U);

    audio.Reset();
    REQUIRE(!audio.IsReady());
    return 0;
}
