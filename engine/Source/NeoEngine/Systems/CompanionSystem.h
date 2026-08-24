#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstdlib>

namespace NeoEngine {

enum class CompanionRole { Tank, Healer, DPS, Support, Scout, Gatherer };

struct Companion {
    std::string id, name;
    CompanionRole role;
    int level = 1;
    int exp = 0;
    float hp = 100, maxHp = 100;
    float attack = 10, defense = 5;
    float loyalty = 50;
    int skillSlot1 = -1, skillSlot2 = -1;
    bool active = false;
    std::string personality;
};

struct CompanionSkill {
    int id;
    std::string name;
    CompanionRole role;
    float cooldown = 5.0f;
    float power = 1.0f;
};

class CompanionSystem {
private:
    std::vector<Companion> m_Companions;
    std::vector<CompanionSkill> m_Skills;
    int m_ActiveCompanion = -1;
    std::function<void(Companion&)> m_OnLevelUp;
    std::function<void(const Companion&)> m_OnEvolve;
    
public:
    CompanionSystem() {
        m_Skills = {
            {0, "Heal", CompanionRole::Healer, 8.0f, 20.0f},
            {1, "Taunt", CompanionRole::Tank, 12.0f, 0.0f},
            {2, "Berserk", CompanionRole::DPS, 15.0f, 2.5f},
            {3, "Shield", CompanionRole::Support, 20.0f, 0.0f},
            {4, "Gather", CompanionRole::Gatherer, 30.0f, 0.0f},
            {5, "Scout", CompanionRole::Scout, 10.0f, 0.0f},
            {6, "Loyalty", CompanionRole::DPS, 5.0f, 1.5f},
        };
    }
    
    Companion* Recruit(const std::string& name, CompanionRole role) {
        m_Companions.push_back({"c_"+std::to_string(m_Companions.size()), name, role, 1, 0, 
                               100.0f, 100.0f, 10.0f, 5.0f, 50.0f});
        return &m_Companions.back();
    }
    
    bool SetActive(int index) {
        if (index < 0 || index >= m_Companions.size()) return false;
        if (m_ActiveCompanion >= 0) m_Companions[m_ActiveCompanion].active = false;
        m_ActiveCompanion = index;
        m_Companions[index].active = true;
        return true;
    }
    
    Companion* GetActive() {
        if (m_ActiveCompanion < 0 || m_ActiveCompanion >= m_Companions.size()) return nullptr;
        return &m_Companions[m_ActiveCompanion];
    }
    
    void Feed(Companion& c, int food) {
        c.exp += food * 10;
        c.loyalty += 1;
        if (c.loyalty > 100) c.loyalty = 100;
        CheckLevelUp(c);
    }
    
    void CheckLevelUp(Companion& c) {
        int needed = c.level * 50;
        if (c.exp >= needed) {
            c.exp -= needed;
            c.level++;
            c.maxHp += 20;
            c.hp = c.maxHp;
            c.attack += 3;
            c.defense += 2;
            if (m_OnLevelUp) m_OnLevelUp(c);
        }
    }
    
    const std::vector<Companion>& GetAll() const { return m_Companions; }
    void SetOnLevelUp(std::function<void(Companion&)> cb) { m_OnLevelUp = cb; }
};

} // namespace NeoEngine
