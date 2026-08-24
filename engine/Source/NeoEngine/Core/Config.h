#pragma once
#include <string>

namespace NeoEngine {

struct EngineConfig {
    std::string appName = "FauzanEngine Game";
    std::string version = "2.0.0";
    int targetFPS = 60;
    bool vsync = true;
    bool enableDebugLog = false;
    bool enableAI = true;
    std::string modelPath = "/sdcard/Gemma4/";
    std::string savePath = "/sdcard/FauzanEngine/Saves/";
    int maxActors = 100000;
    float worldSizeKm = 100.0f;
    bool enableMonetization = false;
    std::string iapKey = "";
};

} // namespace NeoEngine
