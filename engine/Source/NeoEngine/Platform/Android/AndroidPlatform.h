#pragma once
#include <string>

namespace NeoEngine {

class AndroidPlatform {
public:
    static AndroidPlatform& Get() {
        static AndroidPlatform instance;
        return instance;
    }

    void Init(const std::string& internalPath, const std::string& externalPath) {
        m_InternalPath = internalPath;
        m_ExternalPath = externalPath;
    }

    std::string GetInternalPath() const { return m_InternalPath; }
    std::string GetExternalPath() const { return m_ExternalPath; }

    bool HasExternalStorage() const { return !m_ExternalPath.empty(); }

    void Log(const std::string& tag, const std::string& msg);
    void ShowToast(const std::string& msg);

    float GetBatteryLevel() const;
    bool IsCharging() const;
    float GetCPUTemperature() const;

private:
    AndroidPlatform() = default;
    std::string m_InternalPath;
    std::string m_ExternalPath;
};

} // namespace NeoEngine
