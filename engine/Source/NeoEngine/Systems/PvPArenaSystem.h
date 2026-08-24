#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

namespace NeoEngine {
struct PvPPlayer { std::string id,name; int elo=1000; int wins=0,losses=0; int streak=0; };
struct PvPMatch { std::string p1,p2; int eloChange; bool completed=false; };
class PvPArenaSystem {
private:
    std::vector<PvPPlayer> m_Players; std::vector<PvPMatch> m_Matches;
    std::function<void(const PvPPlayer&,int)> m_OnEloChange;
public:
    void RegisterPlayer(const std::string& id,const std::string& name){ m_Players.push_back({id,name}); }
    PvPMatch* FindMatch(const std::string& playerId){
        PvPPlayer* p=nullptr; for(auto& pl:m_Players)if(pl.id==playerId)p=&pl;
        if(!p)return nullptr;
        PvPPlayer* best=nullptr; int bestDiff=99999;
        for(auto& pl:m_Players){ if(pl.id==playerId)continue; int diff=std::abs(pl.elo-p->elo); if(diff<bestDiff){ bestDiff=diff; best=&pl; } }
        if(!best)return nullptr; m_Matches.push_back({p->id,best->id,0,false}); return &m_Matches.back();
    }
    void ResolveMatch(PvPMatch* match,bool p1Wins){
        PvPPlayer *a=nullptr,*b=nullptr;
        for(auto& p:m_Players){ if(p.id==match->p1)a=&p; if(p.id==match->p2)b=&p; }
        if(!a||!b)return;
        float expected=1.0f/(1.0f+pow(10.0f,(b->elo-a->elo)/400.0f));
        int eloChange=(int)(32.0f*((p1Wins?1.0f:0.0f)-expected));
        a->elo+=eloChange; b->elo-=eloChange;
        if(p1Wins){ a->wins++; a->streak++; b->losses++; b->streak=0; } else { b->wins++; b->streak++; a->losses++; a->streak=0; }
        match->eloChange=eloChange; match->completed=true;
    }
    const auto& GetPlayers()const{ return m_Players; }
    PvPPlayer* GetPlayer(const std::string& id){ for(auto& p:m_Players)if(p.id==id)return &p; return nullptr; }
    void SetOnEloChange(std::function<void(const PvPPlayer&,int)> cb){ m_OnEloChange=cb; }
};
}
