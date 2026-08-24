#include "AndroidPlatform.h"
#include <android/log.h>
#include <time.h>

namespace NeoEngine {

AndroidPlatform& AndroidPlatform::Get() {
    static AndroidPlatform instance;
    return instance;
}

void AndroidPlatform::Init() {
    Log("AndroidPlatform", "Initialized");
}

void AndroidPlatform::Shutdown() {
    Log("AndroidPlatform", "Shutdown");
}

void AndroidPlatform::PumpEvents() {
    // Placeholder: baca input dari ALooper/AInputEvent
}

uint64_t AndroidPlatform::GetTimeNano() const {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000L + now.tv_nsec;
}

void AndroidPlatform::SetMainLoopCallback(std::function<void(float)> cb) {
    m_MainLoop = cb;
}

void AndroidPlatform::Log(const std::string& tag, const std::string& msg) {
    __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", msg.c_str());
}

void AndroidPlatform::ShowToast(const std::string& msg) {
    Log("Toast", msg);
}

float AndroidPlatform::GetBatteryLevel() const {
    return 100.0f;
}

bool AndroidPlatform::IsCharging() const {
    return true;
}

float AndroidPlatform::GetCPUTemperature() const {
    return 45.0f;
}

} // namespace NeoEngine
