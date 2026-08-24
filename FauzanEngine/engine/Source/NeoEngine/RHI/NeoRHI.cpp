#include <android/log.h>

void InitializeRHI() {
    __android_log_print(ANDROID_LOG_INFO, "NeoRHI", "RHI Layer: Vulkan Interface Active.");
}
