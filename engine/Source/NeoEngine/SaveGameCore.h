#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <json/json.h>
#include <android/log.h>

#define LOG_TAG "SaveGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

struct SaveData {
    std::string playerName;
    float posX, posY, posZ;
    int score, level, lives;
    float playTime;
    std::string sceneName;
    std::string timestamp;
};

class SaveGameCore {
public:
    SaveGameCore(const std::string& path = "/sdcard/FauzanEngine/Saves/")
        : m_SavePath(path) {}
    
    bool SaveToSlot(int slot, const SaveData& data) {
        std::string filename = m_SavePath + "save_" + std::to_string(slot) + ".json";
        Json::Value root;
        root["playerName"] = data.playerName;
        root["posX"] = data.posX;
        root["posY"] = data.posY;
        root["posZ"] = data.posZ;
        root["score"] = data.score;
        root["level"] = data.level;
        root["lives"] = data.lives;
        root["playTime"] = data.playTime;
        root["sceneName"] = data.sceneName;
        root["timestamp"] = data.timestamp;
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOGI("Failed to save game to %s", filename.c_str());
            return false;
        }
        Json::StyledWriter writer;
        file << writer.write(root);
        file.close();
        LOGI("Game saved to slot %d", slot);
        return true;
    }
    
    SaveData LoadFromSlot(int slot) {
        SaveData data{};
        std::string filename = m_SavePath + "save_" + std::to_string(slot) + ".json";
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOGI("No save file in slot %d", slot);
            return data;
        }
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root)) return data;
        data.playerName = root.get("playerName", "").asString();
        data.posX = root.get("posX", 0).asFloat();
        data.posY = root.get("posY", 0).asFloat();
        data.posZ = root.get("posZ", 0).asFloat();
        data.score = root.get("score", 0).asInt();
        data.level = root.get("level", 1).asInt();
        data.lives = root.get("lives", 3).asInt();
        data.playTime = root.get("playTime", 0).asFloat();
        data.sceneName = root.get("sceneName", "").asString();
        data.timestamp = root.get("timestamp", "").asString();
        LOGI("Game loaded from slot %d", slot);
        return data;
    }
    
    bool DeleteSlot(int slot) {
        std::string filename = m_SavePath + "save_" + std::to_string(slot) + ".json";
        return remove(filename.c_str()) == 0;
    }
    
    std::vector<std::string> GetSaveSlots() {
        std::vector<std::string> slots;
        for (int i = 1; i <= 10; i++) {
            SaveData d = LoadFromSlot(i);
            if (!d.playerName.empty()) {
                slots.push_back("Slot " + std::to_string(i) + ": " + d.playerName + " (Score: " + std::to_string(d.score) + ")");
            }
        }
        return slots;
    }

private:
    std::string m_SavePath;
};

} // namespace NeoEngine
