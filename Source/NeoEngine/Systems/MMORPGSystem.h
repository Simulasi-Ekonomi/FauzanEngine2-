#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct MMORPGPlayer { std::string id, name, guild; int level=1, exp=0, gold=0; float posX, posY, posZ; };
struct Guild { std::string name; std::vector<std::string> members; int level=1, gold=0; };
struct RaidBoss { std::string id, name; float health=10000, maxHealth=10000; float damage=100; std::vector<std::string> loot; };

class MMORPGSystem {
private:
    std::vector<MMORPGPlayer> m_Players;
    std::vector<Guild> m_Guilds;
    std::vector<RaidBoss> m_RaidBosses;
    
public:
    MMORPGPlayer* CreatePlayer(const std::string& id, const std::string& name, float x, float y){
        m_Players.push_back({id,name,"",1,0,0,x,y,0}); return &m_Players.back();
    }
    Guild* CreateGuild(const std::string& name){ m_Guilds.push_back({name}); return &m_Guilds.back(); }
    bool JoinGuild(const std::string& playerId, const std::string& guildName){
        for(auto& g : m_Guilds){ if(g.name==guildName){ for(auto& p : m_Players){ if(p.id==playerId){ g.members.push_back(playerId); p.guild=guildName; return true; } } } }
        return false;
    }
    RaidBoss* SpawnBoss(const std::string& name, float hp, float dmg){ m_RaidBosses.push_back({name,name,hp,hp,dmg}); return &m_RaidBosses.back(); }
    void DealDamageToBoss(const std::string& bossId, float dmg){ for(auto& b : m_RaidBosses){ if(b.id==bossId){ b.health-=dmg; if(b.health<0)b.health=0; } } }
};

} // namespace NeoEngine
