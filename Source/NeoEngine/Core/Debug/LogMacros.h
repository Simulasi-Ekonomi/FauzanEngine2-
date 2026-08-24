#pragma once
#include "Logger.h"

namespace NeoEngine {

// Makro universal untuk logging dengan 2 argumen (kategori + pesan)
#define LOG_INFO_2(cat, msg)  NeoEngine::Logger::Log(NeoEngine::LogLevel::Info, cat, msg)
#define LOG_WARN_2(cat, msg)  NeoEngine::Logger::Log(NeoEngine::LogLevel::Warning, cat, msg)
#define LOG_ERROR_2(cat, msg) NeoEngine::Logger::Log(NeoEngine::LogLevel::Error, cat, msg)
#define LOG_FATAL_2(cat, msg) NeoEngine::Logger::Log(NeoEngine::LogLevel::Fatal, cat, msg)

// Makro dengan 1 argumen (pesan saja, kategori default Core)
#define LOG_INFO_1(msg)       NeoEngine::Logger::Log(NeoEngine::LogLevel::Info, NeoEngine::LogCategory::Core, msg)
#define LOG_WARN_1(msg)       NeoEngine::Logger::Log(NeoEngine::LogLevel::Warning, NeoEngine::LogCategory::Core, msg)
#define LOG_ERROR_1(msg)      NeoEngine::Logger::Log(NeoEngine::LogLevel::Error, NeoEngine::LogCategory::Core, msg)
#define LOG_FATAL_1(msg)      NeoEngine::Logger::Log(NeoEngine::LogLevel::Fatal, NeoEngine::LogCategory::Core, msg)

// Makro overload (1 atau 2 argumen)
#define GET_MACRO(_1,_2,NAME,...) NAME
#define LOG_INFO(...)  GET_MACRO(__VA_ARGS__, LOG_INFO_2, LOG_INFO_1)(__VA_ARGS__)
#define LOG_WARN(...)  GET_MACRO(__VA_ARGS__, LOG_WARN_2, LOG_WARN_1)(__VA_ARGS__)
#define LOG_ERROR(...) GET_MACRO(__VA_ARGS__, LOG_ERROR_2, LOG_ERROR_1)(__VA_ARGS__)
#define LOG_FATAL(...) GET_MACRO(__VA_ARGS__, LOG_FATAL_2, LOG_FATAL_1)(__VA_ARGS__)

} // namespace NeoEngine
