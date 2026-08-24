#pragma once
#include <string>

namespace NeoEngine {

enum class LogLevel {
    Info,
    Warning,
    Error,
    Fatal
};

enum class LogChannel {
    Core,
    Rendering,
    Physics,
    AI,
    Network,
    UI,
    Audio,
    Gameplay,
    Profiling,
    Memory
};

// Helper untuk konversi enum ke string
const char* LogLevelToString(LogLevel level);
const char* LogChannelToString(LogChannel channel);

} // namespace NeoEngine
