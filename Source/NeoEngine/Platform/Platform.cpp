#include "Platform.h"
#include "Android/AndroidPlatform.h"

NeoEngine::Platform& NeoEngine::Platform::Get() {
    return NeoEngine::AndroidPlatform::Get();
}