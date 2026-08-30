#include "SdlInputBridge.h"

#include "InputState.h"

#include <SDL3/SDL.h>

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
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        lastError_ = SdlInputBridgeError::VideoInitializationFailed;
        return false;
    }
    videoInitialized_ = true;
    SDL_Window* window = SDL_CreateWindow("NeoEngine Input Probe", static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_HIDDEN);
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
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat) continue;
            if (!input.Push(MakeInputCode(InputDeviceType::Keyboard, static_cast<uint16_t>(event.key.scancode)), event.type == SDL_EVENT_KEY_DOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            if (!input.Push(MakeInputCode(InputDeviceType::Mouse, event.button.button), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_MOTION) {
            const bool active = event.type != SDL_EVENT_FINGER_UP;
            if (!input.Push(MakeInputCode(InputDeviceType::Touch, 1), active) || !input.SetTouchPointer(static_cast<uint32_t>(event.tfinger.fingerID), event.tfinger.x, event.tfinger.y, active)) {
                lastError_ = input.LastError() == InputError::InvalidMetadata ? SdlInputBridgeError::MetadataRejected : SdlInputBridgeError::InputQueueRejected;
                return false;
            }
        } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
            if (!input.Push(MakeInputCode(InputDeviceType::Gamepad, event.gbutton.button), event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
                lastError_ = SdlInputBridgeError::InputQueueRejected;
                return false;
            }
            if (!input.SetControllerConnected(true)) {
                lastError_ = SdlInputBridgeError::MetadataRejected;
                return false;
            }
        } else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX || event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
                if (!input.SetControllerAxis(static_cast<uint8_t>(event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX ? 0U : 1U), NormalizeAxis(event.gaxis.value))) {
                    lastError_ = SdlInputBridgeError::MetadataRejected;
                    return false;
                }
            }
        } else if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
            if (!input.SetControllerConnected(true)) {
                lastError_ = SdlInputBridgeError::MetadataRejected;
                return false;
            }
        } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
            if (!input.SetControllerConnected(false)) {
                lastError_ = SdlInputBridgeError::MetadataRejected;
                return false;
            }
        } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            input.ReleaseAll();
            input.MarkFocusLost();
        } else if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
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
    if (videoInitialized_) SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    videoInitialized_ = false;
    quitRequested_ = false;
}

} // namespace NeoEngine
