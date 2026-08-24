#pragma once
#include "Logger.h"

// Makro untuk mendukung gaya lama (1 argumen) dan gaya baru (2 argumen)
#define LOG_INFO_2(cat, msg) NeoEngine::Logger::Log(NeoEngine::LogLevel::Info, cat, msg)
#define LOG_INFO_1(msg)      NeoEngine::Logger::Log(NeoEngine::LogLevel::Info, NeoEngine::LogCategory::Core, msg)

#define GET_MACRO(_1,_2,NAME,...) NAME
#define LOG_INFO(...) GET_MACRO(__VA_ARGS__, LOG_INFO_2, LOG_INFO_1)(__VA_ARGS__)

#define LOG_WARNING(cat, msg) NeoEngine::Logger::Log(NeoEngine::LogLevel::Warning, cat, msg)
#define LOG_ERROR(cat, msg)   NeoEngine::Logger::Log(NeoEngine::LogLevel::Error, cat, msg)
#define LOG_FATAL(cat, msg)   NeoEngine::Logger::Log(NeoEngine::LogLevel::Fatal, cat, msg)
