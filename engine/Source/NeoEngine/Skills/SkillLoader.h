#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <json/json.h>
#include <android/log.h>

namespace NeoEngine {

class SkillLoader {
public:
    static std::vector<std::string> LoadSkillsFromDirectory(const std::string& path) {
        std::vector<std::string> loaded;
        // Scan directory for SKILL.md files
        // Example implementation
        loaded.push_back("android-builder");
        loaded.push_back("aries-game-dev");
        loaded.push_back("asset-forge");
        loaded.push_back("deep-research");
        loaded.push_back("security-audit-bits");
        __android_log_print(ANDROID_LOG_INFO, "SkillLoader", "Loaded %zu skills from %s", loaded.size(), path.c_str());
        return loaded;
    }

    static std::string ReadSkillDescription(const std::string& skillPath) {
        std::ifstream file(skillPath + "/SKILL.md");
        if (!file.is_open()) return "Skill description not available";
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content.substr(0, 200); // First 200 chars
    }
};

} // namespace NeoEngine
