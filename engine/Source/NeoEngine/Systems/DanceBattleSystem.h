#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <functional>

namespace NeoEngine {

struct DanceMove { std::string name; int difficulty=1; float score=100; float duration=1.0f; };
struct DancePlayer { std::string id, name; float score=0; int combo=0; int streak=0; };

class DanceBattleSystem {
private:
    std::vector<DanceMove> m_Moves;
    std::vector<DancePlayer> m_Players;
    std::function<void(const std::string&, float)> m_OnScoreUpdate;

public:
    DanceBattleSystem() {
        m_Moves={{"Wave",1,100},{"Spin",2,200},{"Freeze",3,300},{"Moonwalk",4,500},{"Headspin",5,1000}};
    }
    DancePlayer* AddPlayer(const std::string& id, const std::string& name) {
        m_Players.push_back({id,name}); return &m_Players.back();
    }
    bool PerformMove(const std::string& playerId, int moveIndex, float accuracy) {
        if(moveIndex<0||moveIndex>=m_Moves.size())return false;
        for(auto& p:m_Players)if(p.id==playerId){
            float s=m_Moves[moveIndex].score*accuracy;
            p.score+=s; p.combo++;
            if(accuracy>0.95f)p.streak++;else p.streak=0;
            if(m_OnScoreUpdate)m_OnScoreUpdate(playerId,s);
            return true;
        }
        return false;
    }
    void SetOnScoreUpdate(std::function<void(const std::string&, float)> cb){m_OnScoreUpdate=cb;}
};

} // namespace NeoEngine
