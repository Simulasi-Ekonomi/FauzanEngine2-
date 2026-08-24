#pragma once
#include <string>

namespace NeoEngine {

struct EngineConfig {
    std::string appName = "FauzanEngine Game";
    std::string version = "2.1.0";
    std::string buildType = "Development";
    int targetFPS = 60;
    bool vsync = true;
    bool enableValidationLayers = true;
    bool enableDebugLog = true;
    int maxEntities = 100000;
    int maxThreads = 4;
    float fixedDeltaTime = 0.016667f;
    std::string defaultMap = "MainMenu";
    std::string saveDirectory = "/sdcard/FauzanEngine/Saves/";
    std::string logDirectory = "/sdcard/FauzanEngine/Logs/";
};

class EngineConfiguration {
public:
    static EngineConfiguration& Get();
    
    void LoadFromFile(const std::string& path);
    void SaveToFile(const std::string& path);
    void ApplyDefaults();
    
    EngineConfig& GetConfig() { return config_; }
    const EngineConfig& GetConfig() const { return config_; }
    
    void SetAppName(const std::string& name) { config_.appName = name; }
    void SetTargetFPS(int fps) { config_.targetFPS = fps; }
    void SetVSync(bool v) { config_.vsync = v; }
    
private:
    EngineConfig config_;
};

} // namespace NeoEngine
