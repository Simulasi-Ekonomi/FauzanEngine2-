#pragma once
#include <string>

struct EngineConfig {
    std::string EngineName = "NeoEngine Sovereign";
    int VersionMajor = 1;
    int VersionMinor = 0;
    bool bEnableValidationLayers = true;
};
