#pragma once
#include "Logger.h"
#include "LogMacros.h"
#include "LogCategory.h"

namespace NeoEngine {
    
    // Mempertahankan enum lama untuk menjaga kompatibilitas Core.cpp
    enum class LogChannel {
        Core,
        Memory,
        Platform,
        ECS,
        Rendering
    };

    class Log {
    public:
        static void Write(LogLevel level, LogChannel channel, const std::string& message) {
            // Melakukan instruksi casting (konversi) ke LogCategory milik sistem AAA
            Logger::Log(level, static_cast<LogCategory>(channel), message);
        }
    };
}
