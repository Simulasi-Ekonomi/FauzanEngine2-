#include "Runtime/InputState.h"
#include "Runtime/SdlInputBridge.h"

#include <SDL.h>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    InputState input;
    SdlInputBridge bridge;
    if (!input.Bind("jump", MakeInputCode(InputDeviceType::Keyboard, SDL_SCANCODE_SPACE)) || !input.Bind("click", MakeInputCode(InputDeviceType::Mouse, SDL_BUTTON_LEFT)) || !input.Bind("touch", MakeInputCode(InputDeviceType::Touch, 1)) || !input.Bind("gamepad.primary", MakeInputCode(InputDeviceType::Gamepad, SDL_CONTROLLER_BUTTON_A)) || bridge.InitializeHidden(0, 64) || bridge.LastError() != SdlInputBridgeError::InvalidConfiguration || !bridge.InitializeHidden(64, 64)) return 1;
    SDL_Event down{};
    down.type = SDL_KEYDOWN;
    down.key.keysym.scancode = SDL_SCANCODE_SPACE;
    if (SDL_PushEvent(&down) < 0 || !bridge.PumpFrame(input) || !input.Query("jump").pressed || !input.Query("jump").justPressed) return 1;
    SDL_Event up{};
    up.type = SDL_KEYUP;
    up.key.keysym.scancode = SDL_SCANCODE_SPACE;
    if (SDL_PushEvent(&up) < 0 || !bridge.PumpFrame(input) || input.Query("jump").pressed || !input.Query("jump").justReleased) return 1;
    SDL_Event mouse{}; mouse.type = SDL_MOUSEBUTTONDOWN; mouse.button.button = SDL_BUTTON_LEFT;
    if (SDL_PushEvent(&mouse) < 0 || !bridge.PumpFrame(input) || !input.Query("click").justPressed) return 1;
    SDL_Event finger{}; finger.type = SDL_FINGERDOWN;
    if (SDL_PushEvent(&finger) < 0 || !bridge.PumpFrame(input) || !input.Query("touch").justPressed) return 1;
    SDL_Event controller{}; controller.type = SDL_CONTROLLERBUTTONDOWN; controller.cbutton.button = SDL_CONTROLLER_BUTTON_A;
    if (SDL_PushEvent(&controller) < 0 || !bridge.PumpFrame(input) || !input.Query("gamepad.primary").justPressed) return 1;
    SDL_Event quit{};
    quit.type = SDL_QUIT;
    if (SDL_PushEvent(&quit) < 0 || !bridge.PumpFrame(input) || !bridge.QuitRequested()) return 1;
    std::printf("SDL_INPUT_BRIDGE_SMOKE_OK keyboard=1 mouse=1 touch=1 gamepad=1 quit=1\n");
    return 0;
}
