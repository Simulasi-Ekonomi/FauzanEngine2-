#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace NeoEngine {

struct PlayerProfile {
    std::string playerId, playerName;
    int level = 1;
    int exp = 0;
    std::string title;
    std::string guild;
    std::string avatar;
    std::string bio;
    int totalPlayTime = 0;
    int friends = 0;
    std::vector<std::string> achievements;
    bool isOnline = false;
    std::chrono::system_clock::time_point lastSeen;
    std::unordered_map<std::string, std::string> stats;
    bool profilePublic = true;
};

class PlayerProfileSystem {
private:
    std::unordered_map<std::string, PlayerProfile> m_Profiles;
    std::function<void(const PlayerProfile&)> m_OnProfileUpdated;

public:
    PlayerProfile* CreateProfile(const std::string& id, const std::string& name) {
        m_Profiles[id] = {id, name}; return &m_Profiles[id];
    }
    PlayerProfile* GetProfile(const std::string& id) {
        auto it = m_Profiles.find(id); return it != m_Profiles.end() ? &it->second : nullptr;
    }
    bool UpdateProfile(const std::string& id, const std::string& bio, const std::string& avatar) {
        auto* p = GetProfile(id); if (!p) return false;
        p->bio = bio; p->avatar = avatar;
        if (m_OnProfileUpdated) m_OnProfileUpdated(*p);
        return true;
    }
    void SetOnProfileUpdated(std::function<void(const PlayerProfile&)> cb) { m_OnProfileUpdated = cb; }
};

} // namespace NeoEngine
