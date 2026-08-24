#include <iostream>
#include "Log.h"

extern "C" void NeoEngine_Boot() {
    // Memanggil sistem log yang dibuat di Tahap 9
    #if defined(__ANDROID__)
        Log::Write(LogLevel::Info, LogChannel::Core, "NeoEngine Core Booted via JNI/NativeAppGlue");
    #else
        std::cout << "NeoEngine Core: Systems Operational. Booting..." << std::endl;
    #endif
}
