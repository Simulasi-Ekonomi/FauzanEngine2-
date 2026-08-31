#include "Runtime/InputState.h"
#include "Runtime/SdlInputBridge.h"

#include <SDL3/SDL.h>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    InputState input;
    SdlInputBridge bridge;
    if (!input.Bind("jump", MakeInputCode(InputDeviceType::Keyboard, SDL_SCANCODE_SPACE)) || !input.Bind("click", MakeInputCode(InputDeviceType::Mouse, SDL_BUTTON_LEFT)) || !input.Bind("touch", MakeInputCode(InputDeviceType::Touch, 1)) || !input.Bind("gamepad.primary", MakeInputCode(InputDeviceType::Gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) || bridge.InitializeHidden(0, 64) || bridge.LastError() != SdlInputBridgeError::InvalidConfiguration || !bridge.InitializeHidden(64, 64)) return 1;
    SDL_Event down{};
    down.type = SDL_EVENT_KEY_DOWN;
    down.key.scancode = SDL_SCANCODE_SPACE;
    if (!SDL_PushEvent(&down) || !bridge.PumpFrame(input) || !input.Query("jump").pressed || !input.Query("jump").justPressed) return 1;
    SDL_Event up{};
    up.type = SDL_EVENT_KEY_UP;
    up.key.scancode = SDL_SCANCODE_SPACE;
    if (!SDL_PushEvent(&up) || !bridge.PumpFrame(input) || input.Query("jump").pressed || !input.Query("jump").justReleased) return 1;
    SDL_Event mouse{}; mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN; mouse.button.button = SDL_BUTTON_LEFT;
    if (!SDL_PushEvent(&mouse) || !bridge.PumpFrame(input) || !input.Query("click").justPressed) return 1;
    SDL_Event finger{}; finger.type = SDL_EVENT_FINGER_DOWN;
    if (!SDL_PushEvent(&finger) || !bridge.PumpFrame(input) || !input.Query("touch").justPressed) return 1;
    SDL_Event controller{}; controller.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN; controller.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
    if (!SDL_PushEvent(&controller) || !bridge.PumpFrame(input) || !input.Query("gamepad.primary").justPressed) return 1;
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    if (!SDL_PushEvent(&quit) || !bridge.PumpFrame(input) || !bridge.QuitRequested()) return 1;
    std::printf("SDL_INPUT_BRIDGE_SMOKE_OK keyboard=1 mouse=1 touch=1 gamepad=1 quit=1\n");
    return 0;
}
