#include "Runtime/InputState.h"
#include "Runtime/SdlInputBridge.h"

#include <SDL.h>

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;

    InputState input;
    SdlInputBridge bridge;
    if (!input.Bind("jump", MakeInputCode(InputDeviceType::Keyboard, SDL_SCANCODE_SPACE)) ||
        !input.Bind("touch", MakeInputCode(InputDeviceType::Touch, 1)) ||
        !input.Bind("gamepad.primary", MakeInputCode(InputDeviceType::Gamepad, SDL_CONTROLLER_BUTTON_A)) ||
        !bridge.InitializeHidden(64, 64) ||
        !bridge.PumpFrame(input)) {
        return 1;
    }

    SDL_Event touchDown{};
    touchDown.type = SDL_FINGERDOWN;
    touchDown.tfinger.fingerId = 7;
    touchDown.tfinger.x = 0.25F;
    touchDown.tfinger.y = 0.75F;
    if (SDL_PushEvent(&touchDown) < 0 || !bridge.PumpFrame(input)) return 1;
    const InputFrameMetadata touchMetadata = input.FrameMetadata();
    if (!touchMetadata.pointer.active || touchMetadata.pointer.pointerId != 7U ||
        std::fabs(touchMetadata.pointer.normalizedX - 0.25F) > 0.0001F ||
        std::fabs(touchMetadata.pointer.normalizedY - 0.75F) > 0.0001F ||
        !input.Query("touch").justPressed) {
        return 1;
    }

    SDL_Event touchMotion{};
    touchMotion.type = SDL_FINGERMOTION;
    touchMotion.tfinger.fingerId = 7;
    touchMotion.tfinger.x = 0.5F;
    touchMotion.tfinger.y = 0.125F;
    if (SDL_PushEvent(&touchMotion) < 0 || !bridge.PumpFrame(input)) return 1;
    const InputFrameMetadata motionMetadata = input.FrameMetadata();
    if (!motionMetadata.pointer.active || std::fabs(motionMetadata.pointer.normalizedX - 0.5F) > 0.0001F ||
        std::fabs(motionMetadata.pointer.normalizedY - 0.125F) > 0.0001F) {
        return 1;
    }

    SDL_Event axis{};
    axis.type = SDL_CONTROLLERAXISMOTION;
    axis.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;
    axis.caxis.value = 16384;
    if (SDL_PushEvent(&axis) < 0 || !bridge.PumpFrame(input)) return 1;
    const InputFrameMetadata axisMetadata = input.FrameMetadata();
    if (!axisMetadata.controller.connected || axisMetadata.controller.leftAxisX < 0.49F || axisMetadata.controller.leftAxisX > 0.51F) return 1;

    SDL_Event deviceRemoved{};
    deviceRemoved.type = SDL_CONTROLLERDEVICEREMOVED;
    if (SDL_PushEvent(&deviceRemoved) < 0 || !bridge.PumpFrame(input)) return 1;
    const InputFrameMetadata removedMetadata = input.FrameMetadata();
    if (removedMetadata.controller.connected || removedMetadata.controller.leftAxisX != 0.0F || removedMetadata.controller.leftAxisY != 0.0F) return 1;

    SDL_Event keyDown{};
    keyDown.type = SDL_KEYDOWN;
    keyDown.key.keysym.scancode = SDL_SCANCODE_SPACE;
    if (SDL_PushEvent(&keyDown) < 0 || !bridge.PumpFrame(input) || !input.Query("jump").pressed) return 1;

    SDL_Event focusLost{};
    focusLost.type = SDL_WINDOWEVENT;
    focusLost.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    if (SDL_PushEvent(&focusLost) < 0 || !bridge.PumpFrame(input)) return 1;
    const InputFrameMetadata focusMetadata = input.FrameMetadata();
    if (!focusMetadata.focusLost || input.Query("jump").pressed || !input.Query("jump").justReleased) return 1;

    const InputFrameMetadata beforeInvalid = input.FrameMetadata();
    if (input.SetTouchPointer(7U, std::nanf(""), 0.5F, true) || input.LastError() != InputError::InvalidMetadata) return 1;
    const InputFrameMetadata afterInvalid = input.FrameMetadata();
    if (afterInvalid.pointer.active != beforeInvalid.pointer.active || afterInvalid.pointer.pointerId != beforeInvalid.pointer.pointerId ||
        afterInvalid.pointer.normalizedX != beforeInvalid.pointer.normalizedX || afterInvalid.pointer.normalizedY != beforeInvalid.pointer.normalizedY) return 1;
    if (input.SetControllerAxis(2U, 0.0F) || input.LastError() != InputError::InvalidMetadata) return 1;

    SDL_Event quit{};
    quit.type = SDL_QUIT;
    if (SDL_PushEvent(&quit) < 0 || !bridge.PumpFrame(input) || !bridge.QuitRequested() || !input.FrameMetadata().quitRequested) return 1;

    bridge.Reset();
    input.ClearFrameMetadata();
    if (bridge.PumpFrame(input) || bridge.LastError() != SdlInputBridgeError::NotInitialized || bridge.IsReady()) return 1;

    std::printf("SDL_INPUT_METADATA_SMOKE_OK touch=1 motion=1 axis=1 device_reset=1 focus_release=1 invalid_atomic=1 quit=1 lifecycle=1\n");
    return 0;
}
