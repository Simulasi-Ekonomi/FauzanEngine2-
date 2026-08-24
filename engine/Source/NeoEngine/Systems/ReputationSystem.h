#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

enum class ReputationLevel { Hated, Hostile, Unfriendly, Neutral, Friendly, Honored, Revered, Exalted };

struct Faction {
    std::string id, name, description;
    std::vector<std::string> alliedFactions;
    std::vector<std::string> enemyFactions;
};

struct Reputation {
    std::string playerId, factionId;
    int points = 0;
    ReputationLevel level = ReputationLevel::Neutral;
};

class ReputationSystem {
private:
    std::vector<Faction> m_Factions;
    std::vector<Reputation> m_Reputations;
    std::function<void(const std::string&, const Reputation&)> m_OnRepChange;
    
public:
    ReputationSystem() {
        m_Factions = {
            {"guardians", "Guardians", "Protectors of the realm", {"priests"}, {"bandits"}},
            {"bandits", "Bandits", "Outlaws and thieves", {}, {"guardians"}},
            {"priests", "Priests", "Holy order of light", {"guardians"}, {"cultists"}},
            {"cultists", "Cultists", "Dark worshippers", {}, {"priests"}},
            {"merchants", "Merchants Guild", "Traders and merchants", {}, {}},
            {"scholars", "Scholars Academy", "Masters of knowledge", {}, {}},
        };
    }
    
    void AddReputation(const std::string& playerId, const std::string& factionId, int points) {
        for (auto& r : m_Reputations) {
            if (r.playerId == playerId && r.factionId == factionId) {
                r.points += points;
                UpdateLevel(r);
                if (m_OnRepChange) m_OnRepChange(playerId, r);
                return;
            }
        }
        Reputation r{playerId, factionId, points};
        UpdateLevel(r);
        m_Reputations.push_back(r);
        if (m_OnRepChange) m_OnRepChange(playerId, r);
    }
    
    void UpdateLevel(Reputation& r) {
        if (r.points < -1000) r.level = ReputationLevel::Hated;
        else if (r.points < -500) r.level = ReputationLevel::Hostile;
        else if (r.points < -100) r.level = ReputationLevel::Unfriendly;
        else if (r.points < 100) r.level = ReputationLevel::Neutral;
        else if (r.points < 500) r.level = ReputationLevel::Friendly;
        else if (r.points < 1000) r.level = ReputationLevel::Honored;
        else if (r.points < 5000) r.level = ReputationLevel::Revered;
        else r.level = ReputationLevel::Exalted;
    }
    
    ReputationLevel GetLevel(const std::string& playerId, const std::string& factionId) const {
        for (auto& r : m_Reputations) {
            if (r.playerId == playerId && r.factionId == factionId) return r.level;
        }
        return ReputationLevel::Neutral;
    }
    
    int GetPoints(const std::string& playerId, const std::string& factionId) const {
        for (auto& r : m_Reputations) {
            if (r.playerId == playerId && r.factionId == factionId) return r.points;
        }
        return 0;
    }
    
    bool IsAtWar(const std::string& factionA, const std::string& factionB) const {
        for (auto& f : m_Factions) {
            if (f.id == factionA) {
                for (auto& e : f.enemyFactions) if (e == factionB) return true;
            }
            if (f.id == factionB) {
                for (auto& e : f.enemyFactions) if (e == factionA) return true;
            }
        }
        return false;
    }
    
    const std::vector<Faction>& GetFactions() const { return m_Factions; }
    void SetOnRepChange(std::function<void(const std::string&, const Reputation&)> cb) { m_OnRepChange = cb; }
};

} // namespace NeoEngine
