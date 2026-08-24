#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <json/json.h>
#include <fstream>
#include <android/log.h>

namespace NeoEngine {

class SettingsManager {
private:
    std::unordered_map<std::string, std::string> m_Settings;
    std::string m_FilePath;

public:
    void SetFilePath(const std::string& path) { m_FilePath = path; }

    void SetString(const std::string& key, const std::string& value) { m_Settings[key] = value; }
    void SetInt(const std::string& key, int value) { m_Settings[key] = std::to_string(value); }
    void SetFloat(const std::string& key, float value) { m_Settings[key] = std::to_string(value); }
    void SetBool(const std::string& key, bool value) { m_Settings[key] = value ? "true" : "false"; }

    std::string GetString(const std::string& key, const std::string& def = "") const {
        auto it = m_Settings.find(key); return it != m_Settings.end() ? it->second : def;
    }
    int GetInt(const std::string& key, int def = 0) const {
        auto it = m_Settings.find(key); return it != m_Settings.end() ? std::stoi(it->second) : def;
    }
    float GetFloat(const std::string& key, float def = 0.0f) const {
        auto it = m_Settings.find(key); return it != m_Settings.end() ? std::stof(it->second) : def;
    }
    bool GetBool(const std::string& key, bool def = false) const {
        auto it = m_Settings.find(key); return it != m_Settings.end() ? it->second == "true" : def;
    }

    bool SaveToFile() {
        if (m_FilePath.empty()) return false;
        Json::Value root;
        for (auto& [k, v] : m_Settings) root[k] = v;
        Json::FastWriter writer;
        std::string json = writer.write(root);
        std::ofstream file(m_FilePath);
        if (!file.is_open()) return false;
        file << json;
        file.close();
        return true;
    }

    bool LoadFromFile() {
        if (m_FilePath.empty()) return false;
        std::ifstream file(m_FilePath);
        if (!file.is_open()) return false;
        std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(json, root)) {
            for (auto& key : root.getMemberNames()) m_Settings[key] = root[key].asString();
            return true;
        }
        return false;
    }
};

} // namespace NeoEngine
