#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace NeoEngine {

struct GameStateData {
    std::string currentLevel;
    float playTime = 0;
    std::string checkpoint;
    std::unordered_map<std::string, bool> flags;
};

class GameStateCore {
public:
    std::string GetLevelName() const { return m_Data.currentLevel; }
    float GetPlayTime() const { return m_Data.playTime; }

    void SetFlag(const std::string& key, bool value) { m_Data.flags[key] = value; }
    bool GetFlag(const std::string& key) const {
        auto it = m_Data.flags.find(key);
        return it != m_Data.flags.end() ? it->second : false;
    }

    void Reset() { m_Data = GameStateData{}; }

private:
    GameStateData m_Data;
};

} // namespace NeoEngine
