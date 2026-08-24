#pragma once
#include "../Platform.h"
#include <functional>
#include <string>

namespace NeoEngine {

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
    float GetBatteryLevel() const;
    bool IsCharging() const;
    float GetCPUTemperature() const;

private:
    AndroidPlatform() = default;
    std::function<void(float)> m_MainLoop;
};

} // namespace NeoEngine
