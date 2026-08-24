#pragma once
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <functional>

namespace NeoEngine {

struct MatchmakingPlayer {
    std::string playerId, playerName;
    int elo = 1000;
    int level = 1;
    int winStreak = 0;
    int lossStreak = 0;
    float waitTime = 0;
    std::string preferredMode; // "casual", "ranked", "tournament"
    bool inQueue = false;
};

struct MatchedTeam {
    std::vector<MatchmakingPlayer> players;
    int avgElo = 0;
};

class PlayerMatchmakingSystem {
private:
    std::vector<MatchmakingPlayer> m_Queue;
    int m_TeamSize = 5;
    int m_EloRange = 200;
    float m_ExpandInterval = 10.0f;
    float m_ExpandTimer = 0;
    std::function<void(const MatchedTeam&, const MatchedTeam&)> m_OnMatchFound;
    
public:
    void JoinQueue(const std::string& id, const std::string& name, int elo, int level, 
                   const std::string& mode = "casual") {
        m_Queue.push_back({id, name, elo, level, 0, 0, 0, mode, true});
    }
    
    void LeaveQueue(const std::string& playerId) {
        m_Queue.erase(std::remove_if(m_Queue.begin(), m_Queue.end(),
            [&](const MatchmakingPlayer& p) { return p.playerId == playerId; }), m_Queue.end());
    }
    
    void Update(float dt) {
        for (auto& p : m_Queue) p.waitTime += dt;
        
        m_ExpandTimer += dt;
        if (m_ExpandTimer >= m_ExpandInterval) {
            m_ExpandTimer = 0;
            m_EloRange += 50; // Perlebar range setiap 10 detik
        }
        
        if (m_Queue.size() >= m_TeamSize * 2) {
            // Sort by ELO
            std::sort(m_Queue.begin(), m_Queue.end(), [](auto& a, auto& b) {
                return a.elo > b.elo;
            });
            
            MatchedTeam teamA, teamB;
            // Alternate pick (snake draft)
            for (size_t i = 0; i < m_TeamSize * 2 && i < m_Queue.size(); i++) {
                if (i % 2 == 0) teamA.players.push_back(m_Queue[i]);
                else teamB.players.push_back(m_Queue[i]);
            }
            
            if (teamA.players.size() >= m_TeamSize && teamB.players.size() >= m_TeamSize) {
                // Hitung rata2 ELO
                int sumA = 0, sumB = 0;
                for (auto& p : teamA.players) sumA += p.elo;
                for (auto& p : teamB.players) sumB += p.elo;
                teamA.avgElo = sumA / teamA.players.size();
                teamB.avgElo = sumB / teamB.players.size();
                
                // Hapus dari queue
                for (auto& p : teamA.players) LeaveQueue(p.playerId);
                for (auto& p : teamB.players) LeaveQueue(p.playerId);
                
                if (m_OnMatchFound) m_OnMatchFound(teamA, teamB);
                m_EloRange = 200;
            }
        }
    }
    
    int GetQueueSize() const { return m_Queue.size(); }
    void SetTeamSize(int size) { m_TeamSize = size; }
    void SetOnMatchFound(std::function<void(const MatchedTeam&, const MatchedTeam&)> cb) { m_OnMatchFound = cb; }
};

} // namespace NeoEngine
