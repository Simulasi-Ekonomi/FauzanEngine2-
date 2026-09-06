#include "AndroidPlatform.h"
#include <time.h>
#include <iostream>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace NeoEngine {

AndroidPlatform& AndroidPlatform::Get() {
    static AndroidPlatform instance;
    return instance;
}

void AndroidPlatform::Init() {
    Log("AndroidPlatform", "AndroidPlatform System Initialized");
}

void AndroidPlatform::Shutdown() {
    std::lock_guard<std::mutex> lock(m_EventMutex);
    while (!m_MotionEvents.empty()) m_MotionEvents.pop();
    while (!m_KeyEvents.empty()) m_KeyEvents.pop();
    m_NativeWindow = nullptr;
    Log("AndroidPlatform", "AndroidPlatform System Shutdown");
}

void AndroidPlatform::PumpEvents() {
    AndroidInputMotionEvent motionEv;
    while (PollMotionEvent(motionEv)) {
    }

    AndroidInputKeyEvent keyEv;
    while (PollKeyEvent(keyEv)) {
    }
}

uint64_t AndroidPlatform::GetTimeNano() const {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ULL + static_cast<uint64_t>(now.tv_nsec);
}

void AndroidPlatform::SetMainLoopCallback(std::function<void(float)> cb) {
    m_MainLoop = cb;
}

void AndroidPlatform::Log(const std::string& tag, const std::string& msg) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", msg.c_str());
#else
    std::cout << "[" << tag << "] " << msg << std::endl;
#endif
}

void AndroidPlatform::ShowToast(const std::string& msg) {
    Log("Toast", msg);
}

void AndroidPlatform::UpdateHardwareState(float batteryLevel, bool isCharging, float cpuTemp) {
    m_BatteryLevel = batteryLevel;
    m_IsCharging = isCharging;
    m_CPUTemperature = cpuTemp;
}

float AndroidPlatform::GetBatteryLevel() const {
    return m_BatteryLevel;
}

bool AndroidPlatform::IsCharging() const {
    return m_IsCharging;
}

float AndroidPlatform::GetCPUTemperature() const {
    return m_CPUTemperature;
}

void AndroidPlatform::SetNativeWindow(ANativeWindow* window) {
    m_NativeWindow = window;
}

ANativeWindow* AndroidPlatform::GetNativeWindow() const {
    return m_NativeWindow;
}

void AndroidPlatform::SetDisplayMetrics(int32_t width, int32_t height, float dpi) {
    m_ScreenWidth = width;
    m_ScreenHeight = height;
    m_ScreenDPI = dpi;
}

void AndroidPlatform::InjectMotionEvent(const AndroidInputMotionEvent& event) {
    std::lock_guard<std::mutex> lock(m_EventMutex);
    m_MotionEvents.push(event);
}

void AndroidPlatform::InjectKeyEvent(const AndroidInputKeyEvent& event) {
    std::lock_guard<std::mutex> lock(m_EventMutex);
    m_KeyEvents.push(event);
}

bool AndroidPlatform::PollMotionEvent(AndroidInputMotionEvent& outEvent) {
    std::lock_guard<std::mutex> lock(m_EventMutex);
    if (m_MotionEvents.empty()) return false;
    outEvent = m_MotionEvents.front();
    m_MotionEvents.pop();
    return true;
}

bool AndroidPlatform::PollKeyEvent(AndroidInputKeyEvent& outEvent) {
    std::lock_guard<std::mutex> lock(m_EventMutex);
    if (m_KeyEvents.empty()) return false;
    outEvent = m_KeyEvents.front();
    m_KeyEvents.pop();
    return true;
}

} // namespace NeoEngine
