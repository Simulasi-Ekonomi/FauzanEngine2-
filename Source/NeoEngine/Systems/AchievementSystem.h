#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct Achievement { std::string id, title, desc; bool unlocked=false; int progress=0, target=1; std::string rewardId; int rewardCount=0; };

class AchievementSystem {
private:
    std::unordered_map<std::string, Achievement> m_Achievements;
    std::function<void(const Achievement&)> m_OnUnlock;
    
public:
    void AddAchievement(const std::string& id, const std::string& title, const std::string& desc, int target, const std::string& reward="", int rewardCount=0){
        m_Achievements[id] = {id, title, desc, false, 0, target, reward, rewardCount};
    }
    void UpdateProgress(const std::string& id, int amount=1){
        auto it = m_Achievements.find(id);
        if(it == m_Achievements.end() || it->second.unlocked) return;
        it->second.progress += amount;
        if(it->second.progress >= it->second.target){
            it->second.progress = it->second.target;
            it->second.unlocked = true;
            if(m_OnUnlock) m_OnUnlock(it->second);
        }
    }
    const auto& GetAchievements() const { return m_Achievements; }
    void SetOnUnlock(std::function<void(const Achievement&)> cb){ m_OnUnlock = cb; }
};

} // namespace NeoEngine
