#include "SdlInputBridge.h"

#include "InputState.h"

#include <SDL.h>

#include <algorithm>

namespace NeoEngine {

namespace {

float NormalizeAxis(Sint16 value) {
    const float normalized = static_cast<float>(value) / 32767.0F;
    return std::clamp(normalized, -1.0F, 1.0F);
}

} // namespace

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
    input.ClearFrameMetadata();
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
        } else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP || event.type == SDL_FINGERMOTION) {
            const bool active = event.type != SDL_FINGERUP;
            if (!input.Push(MakeInputCode(InputDeviceType::Touch, 1), active) || !input.SetTouchPointer(static_cast<uint32_t>(event.tfinger.fingerId), event.tfinger.x, event.tfinger.y, active)) {
                lastError_ = input.LastError() == InputError::InvalidMetadata ? SdlInputBridgeError::MetadataRejected : SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
            if (!input.Push(MakeInputCode(InputDeviceType::Gamepad, event.cbutton.button), event.type == SDL_CONTROLLERBUTTONDOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
            if (!input.SetControllerConnected(true)) {
                lastError_ = SdlInputBridgeError::MetadataRejected;
                return false;
            }
        } else if (event.type == SDL_CONTROLLERAXISMOTION) {
            if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                if (!input.SetControllerAxis(static_cast<uint8_t>(event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? 0U : 1U), NormalizeAxis(event.caxis.value))) {
                    lastError_ = SdlInputBridgeError::MetadataRejected;
                    return false;
                }
            }
        } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
            if (!input.SetControllerConnected(true)) {
                lastError_ = SdlInputBridgeError::MetadataRejected;
                return false;
            }
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (!input.SetControllerConnected(false)) {
                lastError_ = SdlInputBridgeError::MetadataRejected;
                return false;
            }
        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            input.ReleaseAll();
            input.MarkFocusLost();
        } else if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
            quitRequested_ = true;
            input.MarkQuitRequested();
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
