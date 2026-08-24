#include "SdlInputBridge.h"

#include "InputState.h"

#include <SDL.h>

namespace NeoEngine {

SdlInputBridge::~SdlInputBridge() {
    Reset();
}

bool SdlInputBridge::InitializeHidden(uint16_t width, uint16_t height) {
    Reset();
    if (width == 0 || height == 0) {
        lastError_ = SdlInputBridgeError::InvalidConfiguration;
        return false;
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        lastError_ = SdlInputBridgeError::VideoInitializationFailed;
        return false;
    }
    videoInitialized_ = true;
    SDL_Window* window = SDL_CreateWindow("NeoEngine Input Probe", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_HIDDEN);
    if (window == nullptr) {
        lastError_ = SdlInputBridgeError::WindowCreationFailed;
        Reset();
        return false;
    }
    window_ = window;
    quitRequested_ = false;
    lastError_ = SdlInputBridgeError::None;
    return true;
}

bool SdlInputBridge::PumpFrame(InputState& input) {
    if (window_ == nullptr) {
        lastError_ = SdlInputBridgeError::NotInitialized;
        return false;
    }
    SDL_Event event{};
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            if (event.type == SDL_KEYDOWN && event.key.repeat != 0) continue;
            if (!input.Push(MakeInputCode(InputDeviceType::Keyboard, static_cast<uint16_t>(event.key.keysym.scancode)), event.type == SDL_KEYDOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            if (!input.Push(MakeInputCode(InputDeviceType::Mouse, event.button.button), event.type == SDL_MOUSEBUTTONDOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP) {
            if (!input.Push(MakeInputCode(InputDeviceType::Touch, 1), event.type == SDL_FINGERDOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
            if (!input.Push(MakeInputCode(InputDeviceType::Gamepad, event.cbutton.button), event.type == SDL_CONTROLLERBUTTONDOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
            quitRequested_ = true;
        }
    }
    input.BeginFrame();
    lastError_ = SdlInputBridgeError::None;
    return true;
}

void SdlInputBridge::Reset() {
    if (window_ != nullptr) SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
    window_ = nullptr;
    if (videoInitialized_) SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    videoInitialized_ = false;
    quitRequested_ = false;
}

} // namespace NeoEngine
