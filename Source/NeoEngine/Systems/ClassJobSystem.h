#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {
struct ClassInfo { std::string name; int hpBonus=0, mpBonus=0, atkBonus=0, defBonus=0, spdBonus=0; std::vector<std::string> skills; };
struct JobInfo { std::string name; std::string parentClass; int level=1; float exp=0; std::vector<std::string> abilities; };
class ClassJobSystem {
private:
    std::unordered_map<std::string, ClassInfo> m_Classes;
    std::unordered_map<std::string, JobInfo> m_PlayerJobs;
    std::function<void(const std::string&, const JobInfo&)> m_OnLevelUp;
public:
    ClassJobSystem() {
        m_Classes = {
            {"Warrior", {"Warrior", 30, 0, 15, 10, 5, {"Slash","Bash","War Cry"}}},
            {"Mage", {"Mage", 10, 40, 20, 5, 0, {"Fireball","Ice Storm","Teleport"}}},
            {"Archer", {"Archer", 15, 10, 12, 8, 10, {"Arrow Rain","Snipe","Escape"}}},
            {"Cleric", {"Cleric", 20, 30, 5, 12, 3, {"Heal","Shield","Revive"}}},
            {"Assassin", {"Assassin", 10, 5, 25, 3, 15, {"Backstab","ShadowStep","Poison"}}},
        };
    }
    JobInfo* AssignClass(const std::string& playerId, const std::string& className) {
        auto it = m_Classes.find(className);
        if (it == m_Classes.end()) return nullptr;
        auto& job = m_PlayerJobs[playerId];
        job.name = className; job.parentClass = className; job.level = 1; job.exp = 0;
        job.abilities = it->second.skills;
        return &job;
    }
    bool GainExp(const std::string& playerId, float exp) {
        auto it = m_PlayerJobs.find(playerId);
        if (it == m_PlayerJobs.end()) return false;
        it->second.exp += exp;
        float needed = it->second.level * 100.0f;
        if (it->second.exp >= needed) {
            it->second.exp -= needed;
            it->second.level++;
            if (m_OnLevelUp) m_OnLevelUp(playerId, it->second);
            return true;
        }
        return false;
    }
    const ClassInfo* GetClass(const std::string& name) const { auto it = m_Classes.find(name); return it != m_Classes.end() ? &it->second : nullptr; }
    const JobInfo* GetJob(const std::string& playerId) const { auto it = m_PlayerJobs.find(playerId); return it != m_PlayerJobs.end() ? &it->second : nullptr; }
    void SetOnLevelUp(std::function<void(const std::string&, const JobInfo&)> cb) { m_OnLevelUp = cb; }
};
}
