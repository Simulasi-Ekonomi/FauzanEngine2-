#pragma once
#include <string>
#include <android/log.h>
namespace NeoEngine {
enum class LogLevel{Trace,Debug,Info,Warning,Error,Fatal};
class Logger {
public:
    static void Log(LogLevel level,const char* tag,const char* fmt,...);
    static void SetMinLevel(LogLevel l){s_MinLevel=l;}
private:
    static LogLevel s_MinLevel;
};
#define NEO_LOG(tag,fmt,...) NeoEngine::Logger::Log(NeoEngine::LogLevel::Info,tag,fmt,##__VA_ARGS__)
}
