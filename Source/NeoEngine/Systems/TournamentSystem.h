#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {
struct TournamentPlayer { std::string id, name; int seed=0; int wins=0, losses=0; bool eliminated=false; };
struct TournamentMatch { TournamentPlayer* p1, *p2; TournamentPlayer* winner=nullptr; int round=0; bool completed=false; };
struct Tournament { std::string id, name; std::vector<TournamentPlayer> players; std::vector<TournamentMatch> matches; int entryFee=10; int prizePool=0; int round=0; bool active=false; bool completed=false; };

class TournamentSystem {
private:
    std::vector<Tournament> m_Tournaments;
    std::function<void(const Tournament&)> m_OnTournamentComplete;
public:
    Tournament* CreateTournament(const std::string& name, int entryFee=10) {
        m_Tournaments.push_back({"t_"+std::to_string(m_Tournaments.size()), name, {}, {}, entryFee, 0, 0, false, false});
        return &m_Tournaments.back();
    }
    bool RegisterPlayer(Tournament* t, const std::string& id, const std::string& name) {
        if (!t || t->active) return false;
        t->players.push_back({id, name, (int)t->players.size(), 0, 0, false});
        t->prizePool += t->entryFee; return true;
    }
    void StartTournament(Tournament* t) {
        if (!t || t->players.size() < 2) return;
        t->active = true; t->round = 1;
        // Generate bracket
        for (size_t i = 0; i < t->players.size(); i += 2) {
            if (i+1 < t->players.size()) t->matches.push_back({&t->players[i], &t->players[i+1], nullptr, 1, false});
        }
    }
    void ReportMatchWin(Tournament* t, int matchIndex, bool p1Wins) {
        if (!t || matchIndex < 0 || matchIndex >= t->matches.size()) return;
        auto& m = t->matches[matchIndex];
        if (m.completed) return;
        m.winner = p1Wins ? m.p1 : m.p2;
        m.completed = true;
        if (p1Wins) { m.p1->wins++; m.p2->losses++; m.p2->eliminated = true; }
        else { m.p2->wins++; m.p1->losses++; m.p1->eliminated = true; }
        // Cek final
        int activeCount = 0;
        for (auto& p : t->players) if (!p.eliminated) activeCount++;
        if (activeCount <= 1) {
            t->active = false; t->completed = true;
            for (auto& p : t->players) if (!p.eliminated) {
                t->prizePool += 100; // Bonus prize
            }
            if (m_OnTournamentComplete) m_OnTournamentComplete(*t);
        }
    }
    const auto& GetTournaments() const { return m_Tournaments; }
    void SetOnComplete(std::function<void(const Tournament&)> cb) { m_OnTournamentComplete = cb; }
};
}
