#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <algorithm>

namespace NeoEngine {

struct EsportsTeam { std::string id, name, tag; std::vector<std::string> players; int elo=1500; int wins=0, losses=0; };
struct EsportsMatch { std::string teamA, teamB; int scoreA=0, scoreB=0; std::string winner; bool completed=false; };
struct EsportsTournament { std::string id, name; std::vector<EsportsTeam> teams; std::vector<EsportsMatch> matches; int prizePool=0; bool active=false; int round=0; };

class EsportsSystem {
private:
    std::vector<EsportsTeam> m_Teams;
    std::vector<EsportsTournament> m_Tournaments;
    std::vector<EsportsMatch> m_MatchHistory;
    std::function<void(const EsportsMatch&)> m_OnMatchEnd;
    std::function<void(const EsportsTournament&)> m_OnTournamentEnd;

public:
    EsportsTeam* CreateTeam(const std::string& name, const std::string& tag) {
        m_Teams.push_back({"t_"+std::to_string(m_Teams.size()), name, tag, {}, 1500, 0, 0}); return &m_Teams.back();
    }
    EsportsTournament* CreateTournament(const std::string& name, int prize=0) {
        m_Tournaments.push_back({"tr_"+std::to_string(m_Tournaments.size()), name, {}, {}, prize, false, 0}); return &m_Tournaments.back();
    }

    bool JoinTournament(EsportsTournament* t, const std::string& teamId) {
        if(!t || t->active) return false;
        for(auto& team: m_Teams) if(team.id == teamId) { t->teams.push_back(team); return true; }
        return false;
    }

    void StartTournament(EsportsTournament* t) {
        if(!t || t->teams.size() < 2) return;
        t->active = true; t->round = 1;
        // Generate bracket (single elimination)
        for(size_t i=0; i<t->teams.size(); i+=2) {
            if(i+1 < t->teams.size()) t->matches.push_back({t->teams[i].id, t->teams[i+1].id, 0, 0, "", false});
        }
    }

    void ReportMatch(EsportsTournament* t, int matchIdx, int scoreA, int scoreB) {
        if(!t || matchIdx >= t->matches.size()) return;
        auto& m = t->matches[matchIdx];
        m.scoreA = scoreA; m.scoreB = scoreB;
        m.winner = scoreA > scoreB ? m.teamA : m.teamB;
        m.completed = true;
        m_MatchHistory.push_back(m);

        // Update ELO
        for(auto& team: m_Teams) {
            if(team.id == m.teamA) { if(m.winner == team.id) { team.wins++; team.elo += 25; } else { team.losses++; team.elo -= 25; } }
            if(team.id == m.teamB) { if(m.winner == team.id) { team.wins++; team.elo += 25; } else { team.losses++; team.elo -= 25; } }
        }

        if(m_OnMatchEnd) m_OnMatchEnd(m);
        // Cek final
        int completed = 0; for(auto& mm: t->matches) if(mm.completed) completed++;
        if(completed >= t->matches.size()) { t->active = false; if(m_OnTournamentEnd) m_OnTournamentEnd(*t); }
    }

    std::vector<EsportsTeam> GetRankings() const {
        std::vector<EsportsTeam> ranked = m_Teams;
        std::sort(ranked.begin(), ranked.end(), [](auto& a, auto& b) { return a.elo > b.elo; });
        return ranked;
    }

    void SetOnMatchEnd(std::function<void(const EsportsMatch&)> cb) { m_OnMatchEnd = cb; }
    void SetOnTournamentEnd(std::function<void(const EsportsTournament&)> cb) { m_OnTournamentEnd = cb; }
};

} // namespace NeoEngine
