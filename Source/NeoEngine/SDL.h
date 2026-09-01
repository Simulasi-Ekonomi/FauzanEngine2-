#ifndef NEOENGINE_SDL3_LEGACY_COMPAT_H
#define NEOENGINE_SDL3_LEGACY_COMPAT_H

#include <SDL3/SDL.h>

// VulkanTexturedPresent.cpp still uses the legacy SDL2 call signatures.
// Keep its behavior intact while routing those calls to SDL3.
inline int NeoEngine_SDL3Compat_Init(SDL_InitFlags flags) {
    return SDL_Init(flags) ? 0 : -1;
}

inline SDL_Window* NeoEngine_SDL3Compat_CreateWindow(const char* title,
                                                       int /*x*/, int /*y*/,
                                                       int width, int height,
                                                       SDL_WindowFlags flags) {
    return SDL_CreateWindow(title, width, height, flags);
}

#ifndef SDL_WINDOWPOS_UNDEFINED
#define SDL_WINDOWPOS_UNDEFINED 0
#endif

#define SDL_Init(flags) NeoEngine_SDL3Compat_Init(flags)
#define SDL_CreateWindow(title, x, y, width, height, flags) \
    NeoEngine_SDL3Compat_CreateWindow(title, x, y, width, height, flags)

#endif
