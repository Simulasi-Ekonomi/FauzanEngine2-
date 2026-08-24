#include "AndroidPlatform.h"
#include <android/log.h>

namespace NeoEngine {

void AndroidPlatform::Log(const std::string& tag, const std::string& msg) {
    __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", msg.c_str());
}

void AndroidPlatform::ShowToast(const std::string& msg) {
    Log("Toast", msg);
}

float AndroidPlatform::GetBatteryLevel() const {
    return 100.0f; // Placeholder
}

bool AndroidPlatform::IsCharging() const {
    return true; // Placeholder
}

float AndroidPlatform::GetCPUTemperature() const {
    return 45.0f; // Placeholder
}

} // namespace NeoEngine
