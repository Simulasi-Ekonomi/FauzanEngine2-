#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct Clan { std::string id, name; int level=1; int trophies=0; int totalWins=0; std::vector<std::string> members; };
struct ClanWar { std::string clanA, clanB; int scoreA=0, scoreB=0; bool active=false; std::chrono::system_clock::time_point startTime; int durationHours=24; };

class ClanWarSystem {
private:
    std::vector<Clan> m_Clans;
    std::vector<ClanWar> m_Wars;
    std::function<void(const ClanWar&)> m_OnWarStart;
    std::function<void(const ClanWar&)> m_OnWarEnd;

public:
    Clan* CreateClan(const std::string& id, const std::string& name) { m_Clans.push_back({id,name}); return &m_Clans.back(); }
    bool JoinClan(const std::string& clanId, const std::string& playerId) {
        for(auto& c : m_Clans){ if(c.id==clanId){ c.members.push_back(playerId); return true; } }
        return false;
    }

    bool StartWar(const std::string& clanAId, const std::string& clanBId) {
        ClanWar war{clanAId, clanBId, 0, 0, true, std::chrono::system_clock::now(), 24};
        m_Wars.push_back(war);
        if(m_OnWarStart) m_OnWarStart(war);
        return true;
    }

    void AddWarScore(const std::string& clanId, int points) {
        for(auto& w : m_Wars){
            if(!w.active) continue;
            if(w.clanA == clanId) w.scoreA += points;
            else if(w.clanB == clanId) w.scoreB += points;
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - w.startTime).count();
            if(elapsed >= w.durationHours){
                w.active = false;
                if(m_OnWarEnd) m_OnWarEnd(w);
                // Update clan trophies
                for(auto& c : m_Clans){
                    if(c.id == w.clanA){ if(w.scoreA > w.scoreB){ c.trophies += 20; c.totalWins++; } }
                    else if(c.id == w.clanB){ if(w.scoreB > w.scoreA){ c.trophies += 20; c.totalWins++; } }
                }
            }
        }
    }

    Clan* GetClan(const std::string& id){ for(auto& c : m_Clans) if(c.id==id) return &c; return nullptr; }
    void SetOnWarStart(std::function<void(const ClanWar&)> cb){ m_OnWarStart=cb; }
    void SetOnWarEnd(std::function<void(const ClanWar&)> cb){ m_OnWarEnd=cb; }
};

} // namespace NeoEngine
