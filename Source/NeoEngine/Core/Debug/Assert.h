#pragma once
#include "Log.h"
#include <signal.h>

namespace NeoEngine {

class Assert {
public:
    static void Fail(const char* expr, const char* file, int line, const char* msg) {
        Log::Write(LogLevel::Fatal, LogChannel::Core,
            std::string("ASSERTION FAILED: ") + expr + " at " + file + ":" + 
            std::to_string(line) + " - " + msg);
        __builtin_trap();
    }
    
    static void Check(bool condition, const char* expr, const char* file, int line, const char* msg) {
        if (!condition) Fail(expr, file, line, msg);
    }
};

} // namespace NeoEngine

#ifdef NEO_DEBUG
    #define NEO_ASSERT(expr, msg) \
        if (!(expr)) { NeoEngine::Assert::Fail(#expr, __FILE__, __LINE__, msg); }
    #define NEO_CHECK(expr, msg) \
        NeoEngine::Assert::Check(expr, #expr, __FILE__, __LINE__, msg)
#else
    #define NEO_ASSERT(expr, msg)
    #define NEO_CHECK(expr, msg)
#endif
