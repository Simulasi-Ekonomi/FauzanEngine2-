#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

namespace NeoEngine {

struct LeaderboardEntry { std::string playerId, playerName; int score; int rank=0; float timestamp; };

class LeaderboardSystem {
private:
    std::vector<LeaderboardEntry> m_Entries;
    int m_MaxEntries = 100;
    bool m_Dirty = true;
    std::function<void()> m_OnUpdate;

public:
    void AddScore(const std::string& id, const std::string& name, int score) {
        m_Entries.push_back({id, name, score, 0, 0});
        m_Dirty = true;
        // Hapus entry lama dari player yang sama (keep highest)
        SortAndClean();
    }

    void SortAndClean() {
        if (!m_Dirty) return;
        std::sort(m_Entries.begin(), m_Entries.end(),
            [](const LeaderboardEntry& a, const LeaderboardEntry& b) { return a.score > b.score; });
        // Remove duplicates (keep highest score)
        std::unordered_map<std::string, int> seen;
        std::vector<LeaderboardEntry> unique;
        for (auto& e : m_Entries) {
            if (seen.find(e.playerId) == seen.end()) {
                seen[e.playerId] = e.score;
                unique.push_back(e);
            }
        }
        m_Entries = unique;
        if (m_Entries.size() > m_MaxEntries) m_Entries.resize(m_MaxEntries);
        // Assign ranks
        for (size_t i = 0; i < m_Entries.size(); i++) m_Entries[i].rank = i + 1;
        m_Dirty = false;
        if (m_OnUpdate) m_OnUpdate();
    }

    int GetPlayerRank(const std::string& playerId) const {
        for (auto& e : m_Entries) if (e.playerId == playerId) return e.rank;
        return -1;
    }

    int GetPlayerScore(const std::string& playerId) const {
        for (auto& e : m_Entries) if (e.playerId == playerId) return e.score;
        return 0;
    }

    std::vector<LeaderboardEntry> GetTopN(int n = 10) const {
        std::vector<LeaderboardEntry> top;
        int count = std::min(n, (int)m_Entries.size());
        for (int i = 0; i < count; i++) top.push_back(m_Entries[i]);
        return top;
    }

    const auto& GetEntries() const { return m_Entries; }
    void Clear() { m_Entries.clear(); m_Dirty = true; }
    void SetOnUpdate(std::function<void()> cb) { m_OnUpdate = cb; }
};

} // namespace NeoEngine
