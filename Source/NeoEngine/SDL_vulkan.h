#ifndef NEOENGINE_SDL3_VULKAN_LEGACY_COMPAT_H
#define NEOENGINE_SDL3_VULKAN_LEGACY_COMPAT_H

#include <SDL3/SDL_vulkan.h>

#include <vector>

inline bool NeoEngine_SDL3Compat_VulkanGetInstanceExtensions(SDL_Window* /*window*/,
                                                               unsigned* count,
                                                               const char** names) {
    if (count == nullptr) return false;
    Uint32 sdlCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&sdlCount);
    if (extensions == nullptr) return false;
    if (names == nullptr) {
        *count = static_cast<unsigned>(sdlCount);
        return true;
    }
    if (*count < static_cast<unsigned>(sdlCount)) return false;
    for (Uint32 i = 0; i < sdlCount; ++i) names[i] = extensions[i];
    *count = static_cast<unsigned>(sdlCount);
    return true;
}

inline bool NeoEngine_SDL3Compat_VulkanCreateSurface(SDL_Window* window,
                                                       VkInstance instance,
                                                       VkSurfaceKHR* surface) {
    return SDL_Vulkan_CreateSurface(window, instance, nullptr, surface);
}

#define SDL_Vulkan_GetInstanceExtensions(window, count, names) \
    NeoEngine_SDL3Compat_VulkanGetInstanceExtensions(window, count, names)
#define SDL_Vulkan_CreateSurface(window, instance, surface) \
    NeoEngine_SDL3Compat_VulkanCreateSurface(window, instance, surface)

// SDL3's compatibility header may define SDL_TRUE as an intentionally
// undefined legacy token. The local Vulkan compatibility layer returns bool,
// so provide the legacy spelling expected by VulkanTexturedPresent.cpp.
#ifdef SDL_TRUE
#undef SDL_TRUE
#endif
#define SDL_TRUE true

#endif
