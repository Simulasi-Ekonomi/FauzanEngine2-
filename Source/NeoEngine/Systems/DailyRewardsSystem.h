#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <functional>

namespace NeoEngine {

struct DailyReward {
    int day;
    std::string reward;
    int amount;
    bool claimed = false;
};

class DailyRewardsSystem {
private:
    std::vector<DailyReward> m_Rewards;
    int m_CurrentDay = 1;
    int m_StreakDays = 0;
    int m_MaxStreak = 7;
    std::chrono::system_clock::time_point m_LastClaim;
    std::function<void(const DailyReward&)> m_OnClaim;

public:
    DailyRewardsSystem() {
        m_Rewards = {
            {1, "Gold", 100}, {2, "Gems", 5}, {3, "Energy", 50},
            {4, "Gold", 200}, {5, "Gems", 10}, {6, "Rare Chest", 1},
            {7, "Gems", 50}, {14, "Gems", 100}, {21, "Gems", 200},
            {30, "Legendary Chest", 1}
        };
    }

    bool ClaimDailyReward() {
        auto now = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::hours>(now - m_LastClaim).count();

        if (diff >= 20) { // Allow claim after 20h
            auto it = std::find_if(m_Rewards.begin(), m_Rewards.end(),
                [this](const DailyReward& r) { return r.day == m_CurrentDay && !r.claimed; });
            if (it != m_Rewards.end()) {
                it->claimed = true;
                m_StreakDays++;
                m_LastClaim = now;
                if (m_OnClaim) m_OnClaim(*it);
                m_CurrentDay++;
                return true;
            }
        }
        return false;
    }

    bool ClaimStreakReward(int day) {
        for (auto& r : m_Rewards) {
            if (r.day == day && m_StreakDays >= day && !r.claimed) {
                r.claimed = true;
                if (m_OnClaim) m_OnClaim(r);
                return true;
            }
        }
        return false;
    }

    int GetCurrentDay() const { return m_CurrentDay; }
    int GetStreak() const { return m_StreakDays; }
    const std::vector<DailyReward>& GetRewards() const { return m_Rewards; }
    void SetOnClaim(std::function<void(const DailyReward&)> cb) { m_OnClaim = cb; }
};

} // namespace NeoEngine
