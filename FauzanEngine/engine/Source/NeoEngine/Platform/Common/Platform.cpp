#include "Platform.h"
#if defined(__ANDROID__)
    #include "../Android/AndroidPlatform.h"
    static AndroidPlatform GPlatform;
#else
    #error Platform not supported
#endif
Platform& Platform::Get() { return GPlatform; }
