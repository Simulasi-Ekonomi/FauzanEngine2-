#include "RPGSystem.h"
#include <android/log.h>
#include <algorithm>

#define LOG_TAG_RPG "RPGSystem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_RPG, __VA_ARGS__)

namespace NeoEngine {

RPGCharacter* RPGSystem::CreateCharacter(const std::string& id, const std::string& name) {
    auto& c = m_Characters[id];
    c.id = id; c.name = name;
    LOGI("Character created: %s", name.c_str());
    return &c;
}

RPGCharacter* RPGSystem::GetCharacter(const std::string& id) {
    auto it = m_Characters.find(id);
    return it != m_Characters.end() ? &it->second : nullptr;
}

void RPGSystem::AddExp(RPGCharacter* c, int amount) {
    if (!c) return;
    c->exp += amount;
    while (c->exp >= c->expToNext) {
        c->exp -= c->expToNext;
        LevelUp(c);
    }
}

void RPGSystem::LevelUp(RPGCharacter* c) {
    c->level++;
    c->expToNext = c->level * 100;
    c->maxHp += 20; c->hp = c->maxHp;
    c->maxMp += 10; c->mp = c->maxMp;
    c->strength += 2; c->agility += 2; c->intelligence += 2;
    c->skillPoints += 3;
    if (OnLevelUp) OnLevelUp(c, c->level);
}

bool RPGSystem::LearnSkill(RPGCharacter* c, const std::string& skillName, int cost) {
    if (!c || c->skillPoints < cost) return false;
    c->skills[skillName]++;
    c->skillPoints -= cost;
    if (OnSkillLearned) OnSkillLearned(c, skillName);
    return true;
}

int RPGSystem::GetSkillLevel(const RPGCharacter* c, const std::string& skillName) const {
    if (!c) return 0;
    auto it = c->skills.find(skillName);
    return it != c->skills.end() ? it->second : 0;
}

void RPGSystem::TakeDamage(RPGCharacter* c, int damage) { if (c) c->hp = std::max(0, c->hp - damage); }
void RPGSystem::Heal(RPGCharacter* c, int amount) { if (c) c->hp = std::min(c->maxHp, c->hp + amount); }
bool RPGSystem::IsAlive(const RPGCharacter* c) const { return c && c->hp > 0; }

RPGSystem::Party* RPGSystem::CreateParty(RPGCharacter* leader) {
    Party p; p.leader = leader; p.members.push_back(leader);
    m_Parties.push_back(p);
    return &m_Parties.back();
}

void RPGSystem::AddToParty(Party* party, RPGCharacter* member) { if (party && member) party->members.push_back(member); }
void RPGSystem::RemoveFromParty(Party* party, RPGCharacter* member) {
    if (!party) return;
    auto it = std::find(party->members.begin(), party->members.end(), member);
    if (it != party->members.end()) party->members.erase(it);
}

} // namespace NeoEngine
