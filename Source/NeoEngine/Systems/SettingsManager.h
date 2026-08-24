#pragma once
#include <string>
#include <unordered_map>
#include <fstream>

namespace NeoEngine {

class SettingsManager {
private:
    std::unordered_map<std::string, std::string> m_Settings;
    std::string m_FilePath;

public:
    void SetFilePath(const std::string& path) { m_FilePath = path; }
    void SetString(const std::string& key, const std::string& value) { m_Settings[key] = value; }
    std::string GetString(const std::string& key) const {
        auto it = m_Settings.find(key); return it != m_Settings.end() ? it->second : "";
    }
    bool SaveToFile() {
        std::ofstream file(m_FilePath);
        if (!file.is_open()) return false;
        for (auto& [k, v] : m_Settings) file << k << "=" << v << "\n";
        return true;
    }
};

} // namespace NeoEngine
