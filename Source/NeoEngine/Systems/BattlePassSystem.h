#pragma once
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct BattlePassTier {
    int level;
    std::string freeReward;
    std::string premiumReward;
    bool freeClaimed = false;
    bool premiumClaimed = false;
};

enum class PassType { Free, Premium };

class BattlePassSystem {
private:
    std::vector<BattlePassTier> m_Tiers;
    int m_CurrentTier = 1;
    int m_XP = 0;
    int m_XPPerTier = 100;
    int m_MaxTier = 50;
    PassType m_PassType = PassType::Free;
    bool m_Active = true;
    std::chrono::system_clock::time_point m_StartTime, m_EndTime;
    std::function<void(int, const std::string&)> m_OnReward;

public:
    BattlePassSystem() {
        m_StartTime = std::chrono::system_clock::now();
        m_EndTime = m_StartTime + std::chrono::hours(720); // 30 days
        // Generate 50 tiers
        for (int i = 1; i <= m_MaxTier; i++) {
            std::string freeR = "Gold x" + std::to_string(i * 50);
            std::string premR = "Diamond x" + std::to_string(i * 10) + " + Skin Tier " + std::to_string(i / 10 + 1);
            m_Tiers.push_back({i, freeR, premR, false, false});
        }
        // Special rewards setiap milestone
        m_Tiers[9].freeReward = "Rare Chest";
        m_Tiers[9].premiumReward = "Legendary Skin";
        m_Tiers[24].freeReward = "Epic Key";
        m_Tiers[24].premiumReward = "Mythic Skin";
        m_Tiers[49].freeReward = "1000 Gold + Exclusive Title";
        m_Tiers[49].premiumReward = "Ultimate Skin + 5000 Diamonds";
    }

    void AddXP(int amount) {
        if (!m_Active) return;
        m_XP += amount;
        while (m_XP >= m_XPPerTier && m_CurrentTier < m_MaxTier) {
            m_XP -= m_XPPerTier;
            m_CurrentTier++;
            if (m_OnReward) {
                m_OnReward(m_CurrentTier, m_Tiers[m_CurrentTier - 1].freeReward);
            }
        }
    }

    bool ClaimReward(int tier, bool premium) {
        if (tier < 1 || tier > m_CurrentTier || tier > m_Tiers.size()) return false;
        auto& t = m_Tiers[tier - 1];
        if (premium && m_PassType == PassType::Premium && !t.premiumClaimed) {
            t.premiumClaimed = true;
            return true;
        }
        if (!premium && !t.freeClaimed) {
            t.freeClaimed = true;
            return true;
        }
        return false;
    }

    bool UpgradeToPremium() {
        if (m_PassType == PassType::Premium) return false;
        m_PassType = PassType::Premium;
        return true;
    }

    void Update(float dt) {
        auto now = std::chrono::system_clock::now();
        if (now > m_EndTime) m_Active = false;
    }

    int GetCurrentTier() const { return m_CurrentTier; }
    int GetXP() const { return m_XP; }
    int GetMaxTier() const { return m_MaxTier; }
    PassType GetPassType() const { return m_PassType; }
    bool IsActive() const { return m_Active; }
    const auto& GetTiers() const { return m_Tiers; }
    void SetOnReward(std::function<void(int, const std::string&)> cb) { m_OnReward = cb; }
};

} // namespace NeoEngine
