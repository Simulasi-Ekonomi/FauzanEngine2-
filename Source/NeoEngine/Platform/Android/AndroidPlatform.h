#pragma once
#include "../Platform.h"
#include <functional>
#include <string>
#include <queue>
#include <mutex>
#include <cstdint>

struct ANativeWindow;

namespace NeoEngine {

struct AndroidInputMotionEvent {
    int32_t action;
    float x;
    float y;
    int32_t pointerId;
    uint64_t timestamp;
};

struct AndroidInputKeyEvent {
    int32_t action;
    int32_t keyCode;
    uint64_t timestamp;
};

class AndroidPlatform : public Platform {
public:
    static AndroidPlatform& Get();

    void Init() override;
    void Shutdown() override;
    void PumpEvents() override;
    std::string GetPlatformName() const override { return "Android"; }
    uint64_t GetTimeNano() const override;
    void SetMainLoopCallback(std::function<void(float)> cb) override;

    void Log(const std::string& tag, const std::string& msg);
    void ShowToast(const std::string& msg);

    void UpdateHardwareState(float batteryLevel, bool isCharging, float cpuTemp);
    float GetBatteryLevel() const;
    bool IsCharging() const;
    float GetCPUTemperature() const;

    void SetNativeWindow(ANativeWindow* window);
    ANativeWindow* GetNativeWindow() const;
    void SetDisplayMetrics(int32_t width, int32_t height, float dpi);
    int32_t GetScreenWidth() const { return m_ScreenWidth; }
    int32_t GetScreenHeight() const { return m_ScreenHeight; }
    float GetScreenDPI() const { return m_ScreenDPI; }

    void InjectMotionEvent(const AndroidInputMotionEvent& event);
    void InjectKeyEvent(const AndroidInputKeyEvent& event);
    bool PollMotionEvent(AndroidInputMotionEvent& outEvent);
    bool PollKeyEvent(AndroidInputKeyEvent& outEvent);

private:
    AndroidPlatform() = default;
    std::function<void(float)> m_MainLoop;

    float m_BatteryLevel = 100.0f;
    bool m_IsCharging = true;
    float m_CPUTemperature = 35.0f;

    ANativeWindow* m_NativeWindow = nullptr;
    int32_t m_ScreenWidth = 1920;
    int32_t m_ScreenHeight = 1080;
    float m_ScreenDPI = 320.0f;

    std::queue<AndroidInputMotionEvent> m_MotionEvents;
    std::queue<AndroidInputKeyEvent> m_KeyEvents;
    mutable std::mutex m_EventMutex;
};

} // namespace NeoEngine
