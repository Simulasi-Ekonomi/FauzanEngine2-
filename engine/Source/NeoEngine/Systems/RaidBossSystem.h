#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <functional>

namespace NeoEngine {
struct RaidBoss { std::string id,name; float hp=100000,maxHp=100000; float enrageTimer=300; int level=1; std::vector<std::string> loot; bool alive=true; };
struct RaidParticipant { std::string playerId; float damageDealt=0; int rank=0; };
class RaidBossSystem {
private:
    std::vector<RaidBoss> m_Bosses; std::vector<RaidParticipant> m_Participants;
    std::function<void(const RaidBoss&)> m_OnBossDead; std::function<void(const std::vector<RaidParticipant>&)> m_OnRaidEnd;
public:
    RaidBoss* SpawnBoss(const std::string& name,int level,float hp){ m_Bosses.push_back({name,name,hp,hp,300.0f,level}); return &m_Bosses.back(); }
    float DealDamage(const std::string& playerId,const std::string& bossId,float dmg){
        for(auto& b:m_Bosses){ if(b.id==bossId&&b.alive){ b.hp-=dmg; if(b.hp<=0){ b.hp=0; b.alive=false; if(m_OnBossDead)m_OnBossDead(b); }
            bool found=false; for(auto& p:m_Participants){ if(p.playerId==playerId){ p.damageDealt+=dmg; found=true; break; } }
            if(!found)m_Participants.push_back({playerId,dmg});
            return b.hp;
        } } return -1;
    }
    void EndRaid(){ for(auto& b:m_Bosses)if(b.alive){ b.alive=false; b.hp=0; } if(m_OnRaidEnd)m_OnRaidEnd(m_Participants); }
    void SetOnBossDead(std::function<void(const RaidBoss&)> cb){ m_OnBossDead=cb; }
    void SetOnRaidEnd(std::function<void(const std::vector<RaidParticipant>&)> cb){ m_OnRaidEnd=cb; }
};
}
