#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <functional>

namespace NeoEngine {

struct BRPlayer { std::string id; float posX, posY, posZ; float health=100; int kills=0; bool alive=true; int team=0; };
struct LootItem { std::string type; float posX, posY, posZ; int ammo=0; float damage=0; int rarity=1; };

class BattleRoyaleSystem {
private:
    std::vector<BRPlayer> m_Players;
    std::vector<LootItem> m_Loot;
    float m_SafeZoneRadius = 500.0f;
    float m_SafeZoneX=0, m_SafeZoneY=0;
    int m_AliveCount=0;
    std::function<void(const std::string&)> m_OnWinner;
    
public:
    void AddPlayer(const std::string& id, float x, float y, int team=0){
        m_Players.push_back({id,x,y,0,100,0,true,team}); m_AliveCount++;
    }
    void AddLoot(float x, float y, const std::string& type="weapon"){ m_Loot.push_back({type,x,y,30,25,1}); }
    
    void UpdateSafeZone(float dt){
        m_SafeZoneRadius -= 0.5f * dt;
        if(m_SafeZoneRadius < 10) m_SafeZoneRadius=10;
        for(auto& p: m_Players){
            if(!p.alive) continue;
            float dx=p.posX - m_SafeZoneX, dy=p.posZ - m_SafeZoneY; // Fixed: p.posY -> p.posZ
            float dist = sqrt(dx*dx + dy*dy);
            if(dist > m_SafeZoneRadius){ p.health -= 5.0f * dt; if(p.health<=0){ p.alive=false; m_AliveCount--; } }
        }
        if(m_AliveCount <= 1 && m_OnWinner){
            for(auto& p: m_Players) if(p.alive) m_OnWinner(p.id);
        }
    }
    
    int GetAliveCount() const { return m_AliveCount; }
    float GetSafeZoneRadius() const { return m_SafeZoneRadius; }
    void SetOnWinner(std::function<void(const std::string&)> cb){ m_OnWinner=cb; }
};

} // namespace NeoEngine
