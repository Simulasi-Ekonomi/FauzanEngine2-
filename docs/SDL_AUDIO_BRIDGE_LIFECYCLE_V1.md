# SDL Audio Bridge Lifecycle V1

## Scope

`SdlAudioBridge::Reset` now pauses and locks the active SDL audio device, clears the bounded `AudioMixer` voice queue while the callback cannot enter it, unlocks/closes the device, then releases the SDL audio subsystem. A reset with no active device clears the same queue under the bridge mutex. Reset intentionally preserves the most recent explicit error; a successful `Initialize` replaces it with `None`.

This makes reset/reinitialize a defined ownership boundary: queued pre-reset voices cannot play on a subsequently opened device, frame accounting restarts at zero, and a new voice ID can be accepted after recovery. `Play` remains fail-closed when no device is ready.

## Evidence and boundary

`sdl_audio_bridge_smoke` passes in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1` under SDL’s dummy audio driver. It proves invalid-init error preservation, long-lived queued voices before reset, callback-safe clear, post-reset `NotInitialized` rejection, error preservation across a second reset, reinitialize at zero frames/no voices, and acceptance of an ID previously queued before reset.

The dummy driver only proves local callback/device lifecycle. It does not prove physical audio output, device-loss notifications, hot-plug, Android/iOS audio focus, codecs, spatial audio, volume routing, latency targets, accessibility, or production platform readiness.
