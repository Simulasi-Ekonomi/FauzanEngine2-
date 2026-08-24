#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <algorithm>

namespace NeoEngine {

struct WorldBoss {
    std::string id, name;
    float hp = 1000000, maxHp = 1000000;
    float respawnHours = 8;
    std::chrono::system_clock::time_point deathTime;
    bool alive = true;
    int level = 1;
    std::vector<std::string> lootTable;
};

struct WorldBossRanking {
    std::string playerId, playerName;
    float damage = 0;
    int rank = 0;
};

class WorldBossSystem {
private:
    std::vector<WorldBoss> m_Bosses;
    std::unordered_map<std::string, std::vector<WorldBossRanking>> m_Rankings;
    std::function<void(const WorldBoss&, const std::vector<WorldBossRanking>&)> m_OnBossDead;
    
public:
    WorldBossSystem() {
        m_Bosses.push_back({"dragon_king", "Dragon King", 1000000, 1000000, 8, {}, true, 50,
                           {"Dragon Scale", "Dragon Fang", "Legendary Sword", "Gold x10000"}});
        m_Bosses.push_back({"demon_lord", "Demon Lord", 2000000, 2000000, 12, {}, true, 80,
                           {"Demon Horn", "Dark Essence", "Mythic Armor", "Gems x500"}});
        m_Bosses.push_back({"ancient_golem", "Ancient Golem", 500000, 500000, 4, {}, true, 30,
                           {"Golem Core", "Ancient Stone", "Epic Shield", "Gold x3000"}});
    }
    
    void DealDamage(const std::string& bossId, const std::string& playerId, const std::string& playerName, float damage) {
        for (auto& b : m_Bosses) {
            if (b.id == bossId && b.alive) {
                b.hp -= damage;
                if (b.hp <= 0) {
                    b.hp = 0;
                    b.alive = false;
                    b.deathTime = std::chrono::system_clock::now();
                    if (m_OnBossDead) m_OnBossDead(b, m_Rankings[bossId]);
                }
                // Update ranking
                auto& rankings = m_Rankings[bossId];
                bool found = false;
                for (auto& r : rankings) {
                    if (r.playerId == playerId) { r.damage += damage; found = true; break; }
                }
                if (!found) rankings.push_back({playerId, playerName, damage, 0});
                // Sort rankings
                std::sort(rankings.begin(), rankings.end(), [](auto& a, auto& b) { return a.damage > b.damage; });
                for (size_t i = 0; i < rankings.size(); i++) rankings[i].rank = i + 1;
                return;
            }
        }
    }
    
    void UpdateRespawns() {
        auto now = std::chrono::system_clock::now();
        for (auto& b : m_Bosses) {
            if (!b.alive) {
                auto diff = std::chrono::duration_cast<std::chrono::hours>(now - b.deathTime).count();
                if (diff >= b.respawnHours) {
                    b.alive = true;
                    b.hp = b.maxHp;
                    m_Rankings[b.id].clear();
                }
            }
        }
    }
    
    const std::vector<WorldBoss>& GetBosses() const { return m_Bosses; }
    WorldBoss* GetBoss(const std::string& id) {
        for (auto& b : m_Bosses) if (b.id == id) return &b;
        return nullptr;
    }
    void SetOnBossDead(std::function<void(const WorldBoss&, const std::vector<WorldBossRanking>&)> cb) { m_OnBossDead = cb; }
};

} // namespace NeoEngine
