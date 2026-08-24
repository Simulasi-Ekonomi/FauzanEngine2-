#pragma once
#include "Log.h"

#if defined(NEO_DEBUG)
    #include <signal.h>
    #define NEO_ASSERT(expr, msg) \
    if (!(expr)) { \
        Log::Write(LogLevel::Fatal, LogChannel::Core, std::string("ASSERTION FAILED: ") + msg); \
        __builtin_trap(); \
    }
#else
    #define NEO_ASSERT(expr, msg) ((void)0)
#endif
