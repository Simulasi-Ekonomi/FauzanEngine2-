#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdlib>

namespace NeoEngine {

struct Hero {
    std::string id, name, role;
    float posX = 0, posY = 0, posZ = 0;
    float health = 100, maxHealth = 100;
    float mana = 50, maxMana = 50;
    float attack = 20, defense = 10;
    float attackSpeed = 1.0f;
    float critChance = 0.15f;
    int level = 1, star = 1;
    int cost = 1;
    std::string faction, element;
    std::vector<std::string> synergies;
    std::function<void(Hero&, std::vector<Hero>&)> ability;
    float abilityCooldown = 5.0f, abilityTimer = 0;
};

struct BattleResult { bool victory; int round; std::string mvp; int damageDealt; int damageTaken; };

class AutoBattlerSystem {
private:
    std::vector<Hero> m_TeamA, m_TeamB;
    std::unordered_map<std::string, int> m_SynergiesA, m_SynergiesB;
    std::vector<BattleResult> m_History;
    int m_Round = 1;
    int m_Gold = 5;
    int m_Level = 1;
    int m_MaxTeamSize = 1;

public:
    Hero CreateHero(const std::string& name, const std::string& role, int cost, 
                    const std::string& faction, const std::string& element) {
        Hero h{"h_" + std::to_string(rand()), name, role, 0, 0, 0};
        h.cost = cost; h.faction = faction; h.element = element;
        h.synergies = {faction, element};
        if (role == "Tank") { h.health = 200; h.attack = 10; h.defense = 25; }
        else if (role == "Assassin") { h.health = 80; h.attack = 35; h.critChance = 0.3f; h.attackSpeed = 1.8f; }
        else if (role == "Mage") { h.health = 90; h.attack = 40; h.mana = 100; h.abilityCooldown = 3.0f; }
        else if (role == "Support") { h.health = 120; h.attack = 8; h.abilityCooldown = 4.0f; }
        else if (role == "Ranger") { h.health = 100; h.attack = 25; h.attackSpeed = 1.5f; h.critChance = 0.2f; }
        return h;
    }

    bool AddToTeam(Hero hero, bool teamA = true) {
        auto& team = teamA ? m_TeamA : m_TeamB;
        int maxSize = teamA ? m_MaxTeamSize : m_TeamB.size() + 1;
        if (team.size() >= maxSize) return false;
        if (m_Gold < hero.cost) return false;
        m_Gold -= hero.cost;
        team.push_back(hero);
        UpdateSynergies(teamA ? m_SynergiesA : m_SynergiesB, hero);
        return true;
    }

    void UpdateSynergies(std::unordered_map<std::string, int>& syn, const Hero& h) {
        syn[h.faction]++; syn[h.element]++;
    }

    int GetSynergyBonus(const std::unordered_map<std::string, int>& syn, int required) {
        for (auto& [k, v] : syn) if (v >= required) return required;
        return 0;
    }

    BattleResult SimulateBattle() {
        BattleResult result{m_TeamA.size() > 0, m_Round, "", 0, 0};
        std::vector<float> teamAHp, teamBHp;
        for (auto& h : m_TeamA) teamAHp.push_back(h.health);
        for (auto& h : m_TeamB) teamBHp.push_back(h.health);

        // Simple auto-battle simulation
        float time = 0;
        int maxTime = 30; // 30 seconds max
        while (time < maxTime && !teamAHp.empty() && !teamBHp.empty()) {
            time += 0.1f;
            // Each hero attacks a random enemy
            for (size_t i = 0; i < m_TeamA.size(); i++) {
                if (teamBHp.empty()) break;
                int target = rand() % teamBHp.size();
                auto& attacker = m_TeamA[i];
                auto& defender = m_TeamB[target];
                float dmg = attacker.attack * (1.0f - defender.defense / (defender.defense + 100));
                if ((rand() % 100) < attacker.critChance * 100) dmg *= 2.0f;
                teamBHp[target] -= dmg;
                result.damageDealt += int(dmg);
                if (teamBHp[target] <= 0) teamBHp.erase(teamBHp.begin() + target);
            }
            for (size_t i = 0; i < m_TeamB.size(); i++) {
                if (teamAHp.empty()) break;
                int target = rand() % teamAHp.size();
                auto& attacker = m_TeamB[i];
                auto& defender = m_TeamA[target];
                float dmg = attacker.attack * (1.0f - defender.defense / (defender.defense + 100));
                if ((rand() % 100) < attacker.critChance * 100) dmg *= 2.0f;
                teamAHp[target] -= dmg;
                result.damageTaken += int(dmg);
                if (teamAHp[target] <= 0) teamAHp.erase(teamAHp.begin() + target);
            }
        }
        result.victory = !teamAHp.empty();
        if (result.victory) {
            m_Gold += 3 + m_Round;
            if (m_Round % 5 == 0) m_Level++;
        }
        m_Round++;
        m_History.push_back(result);
        return result;
    }

    int GetGold() const { return m_Gold; }
    int GetLevel() const { return m_Level; }
    int GetMaxTeamSize() const { return m_MaxTeamSize; }
    void LevelUp() { m_Level++; m_MaxTeamSize = m_Level / 2 + 1; if (m_MaxTeamSize > 10) m_MaxTeamSize = 10; }
};

} // namespace NeoEngine
