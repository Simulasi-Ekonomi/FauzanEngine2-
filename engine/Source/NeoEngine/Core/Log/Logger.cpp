#include "Logger.h"
#include <android/log.h>

void Logger::Info(const std::string& msg) {
    __android_log_print(ANDROID_LOG_INFO, "NeoEngine", "[INFO]: %s", msg.c_str());
}

void Logger::Warning(const std::string& msg) {
    __android_log_print(ANDROID_LOG_WARN, "NeoEngine", "[WARN]: %s", msg.c_str());
}

void Logger::Error(const std::string& msg) {
    __android_log_print(ANDROID_LOG_ERROR, "NeoEngine", "[ERROR]: %s", msg.c_str());
}
