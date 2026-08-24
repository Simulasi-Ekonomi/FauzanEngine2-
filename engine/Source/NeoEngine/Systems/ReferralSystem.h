#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct ReferralRecord {
    std::string referrerId;
    std::string referredId;
    int rewardsClaimed = 0;
    bool active = true;
};

class ReferralSystem {
private:
    std::vector<ReferralRecord> m_Referrals;
    std::unordered_map<std::string, int> m_ReferralCount;
    int m_BaseReward = 100;
    int m_MilestoneReward = 500;
    std::function<void(const std::string&, int)> m_OnReferralReward;
    std::function<void(const std::string&, int)> m_OnMilestone;

public:
    bool AddReferral(const std::string& referrer, const std::string& referred) {
        m_Referrals.push_back({referrer, referred, 0, true});
        m_ReferralCount[referrer]++;
        int count = m_ReferralCount[referrer];
        if (m_OnReferralReward) m_OnReferralReward(referrer, m_BaseReward);
        // Milestone rewards
        int milestone = 0;
        if (count == 5) milestone = 1;
        else if (count == 10) milestone = 2;
        else if (count == 25) milestone = 3;
        else if (count == 50) milestone = 4;
        else if (count == 100) milestone = 5;
        if (milestone > 0 && m_OnMilestone) m_OnMilestone(referrer, milestone);
        return true;
    }

    int GetReferralCount(const std::string& playerId) const {
        auto it = m_ReferralCount.find(playerId);
        return it != m_ReferralCount.end() ? it->second : 0;
    }

    int GetTotalReferrals() const { return m_Referrals.size(); }
    void SetOnReferralReward(std::function<void(const std::string&, int)> cb) { m_OnReferralReward = cb; }
    void SetOnMilestone(std::function<void(const std::string&, int)> cb) { m_OnMilestone = cb; }
};

} // namespace NeoEngine
