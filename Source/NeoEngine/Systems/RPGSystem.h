#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct RPGCharacter {
    std::string id, name;
    int level = 1, exp = 0, expToNext = 100;
    int hp = 100, maxHp = 100;
    int mp = 50, maxMp = 50;
    int strength = 10, agility = 10, intelligence = 10;
    int skillPoints = 0;
    std::unordered_map<std::string, int> skills;
};

class RPGSystem {
public:
    RPGCharacter* CreateCharacter(const std::string& id, const std::string& name);
    RPGCharacter* GetCharacter(const std::string& id);
    void AddExp(RPGCharacter* c, int amount);
    bool LearnSkill(RPGCharacter* c, const std::string& skillName, int cost = 1);
    int GetSkillLevel(const RPGCharacter* c, const std::string& skillName) const;
    void TakeDamage(RPGCharacter* c, int damage);
    void Heal(RPGCharacter* c, int amount);
    bool IsAlive(const RPGCharacter* c) const;
    void LevelUp(RPGCharacter* c);

    struct Party {
        std::vector<RPGCharacter*> members;
        RPGCharacter* leader = nullptr;
    };
    Party* CreateParty(RPGCharacter* leader);
    void AddToParty(Party* party, RPGCharacter* member);
    void RemoveFromParty(Party* party, RPGCharacter* member);

    std::function<void(RPGCharacter*, int)> OnLevelUp;
    std::function<void(RPGCharacter*, const std::string&)> OnSkillLearned;

private:
    std::unordered_map<std::string, RPGCharacter> m_Characters;
    std::vector<Party> m_Parties;
};

} // namespace NeoEngine
