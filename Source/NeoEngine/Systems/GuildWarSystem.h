#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace NeoEngine {
struct GuildMember { std::string id; int power=1000; int attacksLeft=3; int defenseLeft=5; };
struct Guild { std::string id,name; std::vector<GuildMember> members; int totalPower=0; int warScore=0; };
struct GuildWar { Guild* attacker; Guild* defender; bool active=false; int roundsLeft=10; std::chrono::system_clock::time_point startTime; };
class GuildWarSystem {
private:
    std::vector<Guild> m_Guilds; std::vector<GuildWar> m_Wars;
    std::function<void(Guild*,Guild*,int)> m_OnWarEnd;
public:
    Guild* CreateGuild(const std::string& id,const std::string& name){ m_Guilds.push_back({id,name}); return &m_Guilds.back(); }
    bool StartWar(const std::string& g1,const std::string& g2){
        Guild *a=nullptr,*b=nullptr;
        for(auto& g:m_Guilds){ if(g.id==g1)a=&g; if(g.id==g2)b=&g; }
        if(!a||!b||a==b)return false;
        m_Wars.push_back({a,b,true,10,std::chrono::system_clock::now()});
        return true;
    }
    int Attack(const std::string& guildId,const std::string& memberId,int targetGuild){
        for(auto& w:m_Wars){ if(!w.active)continue;
            Guild* mine=(w.attacker->id==guildId)?w.attacker:w.defender;
            Guild* theirs=(w.attacker->id==guildId)?w.defender:w.attacker;
            for(auto& m:mine->members){ if(m.id==memberId&&m.attacksLeft>0){
                m.attacksLeft--; int dmg=m.power*(50+rand()%51)/100;
                if(!theirs->members.empty()){ int ti=rand()%theirs->members.size();
                    auto& t=theirs->members[ti]; t.defenseLeft--; theirs->totalPower-=dmg; if(t.defenseLeft<=0)theirs->members.erase(theirs->members.begin()+ti);
                }
                mine->warScore+=dmg; w.roundsLeft--; if(w.roundsLeft<=0){ w.active=false; int winner=(mine->warScore>theirs->warScore)?0:1; if(m_OnWarEnd)m_OnWarEnd(mine,theirs,winner); } return dmg;
            } }
        } return 0;
    }
    void SetOnWarEnd(std::function<void(Guild*,Guild*,int)> cb){ m_OnWarEnd=cb; }
};
}
