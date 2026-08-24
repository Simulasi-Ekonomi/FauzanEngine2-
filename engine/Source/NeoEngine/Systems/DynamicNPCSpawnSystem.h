#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace NeoEngine {

struct NPCSpawnRule {
    std::string npcType;
    std::string biomeRequired;
    float minHour=6, maxHour=18;
    int minLevel=1, maxLevel=99;
    float spawnRadius=50.0f;
    int maxPerArea=5;
    float respawnMinutes=5.0f;
    bool isBoss=false;
    std::function<void()> onSpawn;
};

struct SpawnedNPC {
    std::string id, type;
    float posX, posY, posZ;
    float spawnedAt; // game time
    bool dead=false;
};

class DynamicNPCSpawnSystem {
private:
    std::vector<NPCSpawnRule> m_Rules;
    std::vector<SpawnedNPC> m_ActiveNPCs;
    std::unordered_map<std::string, int> m_SpawnCount;
    float m_WorldTime=0;
    std::string m_CurrentBiome="plains";

public:
    void AddRule(const NPCSpawnRule& r) { m_Rules.push_back(r); }

    void Update(float dt, float worldTime, const std::string& biome) {
        m_WorldTime = worldTime; m_CurrentBiome = biome;
        float hour = fmod(worldTime / 3600.0f, 24.0f);

        for(auto& rule: m_Rules) {
            if(rule.biomeRequired != biome && rule.biomeRequired != "any") continue;
            if(hour < rule.minHour || hour > rule.maxHour) continue;

            int count = 0;
            for(auto& n: m_ActiveNPCs) if(n.type == rule.npcType && !n.dead) count++;
            if(count < rule.maxPerArea) {
                float x = (rand()%200 - 100), z = (rand()%200 - 100);
                m_ActiveNPCs.push_back({rule.npcType+"_"+std::to_string(m_ActiveNPCs.size()), rule.npcType, x, 0, z, worldTime, false});
                if(rule.onSpawn) rule.onSpawn();
            }
        }

        // Respawn dead NPCs
        for(auto& n: m_ActiveNPCs) {
            if(n.dead && (worldTime - n.spawnedAt) > 300.0f) n.dead = false; // reset setelah 5 menit
        }
    }

    SpawnedNPC* GetClosestNPC(float x, float z, float maxDist=10.0f) {
        SpawnedNPC* closest=nullptr; float best=maxDist;
        for(auto& n: m_ActiveNPCs) {
            if(n.dead) continue;
            float d=sqrt(pow(n.posX-x,2)+pow(n.posZ-z,2));
            if(d<best){closest=&n;best=d;}
        }
        return closest;
    }

    const std::vector<SpawnedNPC>& GetActiveNPCs() const { return m_ActiveNPCs; }
};

} // namespace NeoEngine
