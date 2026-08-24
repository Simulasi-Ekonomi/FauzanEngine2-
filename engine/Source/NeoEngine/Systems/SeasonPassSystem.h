#pragma once
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct SeasonReward {
    int tier;
    std::string freeReward;
    std::string premiumReward;
    bool freeClaimed = false;
    bool premiumClaimed = false;
};

struct SeasonPass {
    std::string id, name, theme;
    int maxTier = 50;
    int xpPerTier = 100;
    int currentXP = 0;
    bool premiumOwned = false;
    std::chrono::system_clock::time_point startTime, endTime;
    std::vector<SeasonReward> rewards;
    bool active = true;
};

class SeasonPassSystem {
private:
    std::vector<SeasonPass> m_Passes;
    int m_PlayerLevel = 1;
    int m_PlayerXP = 0;
    std::function<void(int, const std::string&)> m_OnRewardClaimed;
    std::function<void(int, int)> m_OnLevelUp;
    
public:
    SeasonPassSystem() {
        CreateSeason("S1", "Season 1: Rise of Heroes", "fantasy", 50, 30);
    }
    
    void CreateSeason(const std::string& id, const std::string& name, const std::string& theme,
                      int maxTier, int durationDays) {
        SeasonPass sp{id, name, theme, maxTier, 100, 0, false};
        sp.startTime = std::chrono::system_clock::now();
        sp.endTime = sp.startTime + std::chrono::hours(24 * durationDays);
        
        for (int i = 1; i <= maxTier; i++) {
            std::string free = "Gold x" + std::to_string(i * 100);
            std::string premium = "Gems x" + std::to_string(i * 5) + " + Exclusive Item";
            if (i % 10 == 0) {
                free = "Rare Chest";
                premium = "Legendary Skin Tier " + std::to_string(i / 10);
            }
            if (i == maxTier) {
                free = "Epic Mount + 5000 Gold";
                premium = "Mythic Skin + 10000 Gems";
            }
            sp.rewards.push_back({i, free, premium, false, false});
        }
        m_Passes.push_back(sp);
    }
    
    void AddXP(int amount) {
        m_PlayerXP += amount;
        for (auto& sp : m_Passes) {
            if (!sp.active) continue;
            sp.currentXP += amount;
            while (sp.currentXP >= sp.xpPerTier) {
                sp.currentXP -= sp.xpPerTier;
                // Unlock next tier
                for (auto& r : sp.rewards) {
                    if (!r.freeClaimed && r.tier > 0) {
                        if (m_OnRewardClaimed) m_OnRewardClaimed(r.tier, r.freeReward);
                    }
                }
            }
        }
        while (m_PlayerXP >= 500) {
            m_PlayerXP -= 500;
            m_PlayerLevel++;
            if (m_OnLevelUp) m_OnLevelUp(m_PlayerLevel, m_PlayerXP);
        }
    }
    
    bool ClaimReward(int tier, bool premium) {
        for (auto& sp : m_Passes) {
            if (!sp.active) continue;
            for (auto& r : sp.rewards) {
                if (r.tier == tier) {
                    if (premium && !r.premiumClaimed && sp.premiumOwned) {
                        r.premiumClaimed = true;
                        if (m_OnRewardClaimed) m_OnRewardClaimed(tier, r.premiumReward);
                        return true;
                    }
                    if (!premium && !r.freeClaimed) {
                        r.freeClaimed = true;
                        if (m_OnRewardClaimed) m_OnRewardClaimed(tier, r.freeReward);
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    const SeasonPass* GetActiveSeason() const {
        for (auto& sp : m_Passes) if (sp.active) return &sp;
        return nullptr;
    }
    
    int GetCurrentTier() const {
        auto* sp = GetActiveSeason();
        if (!sp) return 0;
        int tier = 0;
        for (auto& r : sp->rewards) if (r.freeClaimed) tier = r.tier;
        return tier;
    }
    
    void BuyPremiumPass() {
        for (auto& sp : m_Passes) if (sp.active) { sp.premiumOwned = true; break; }
    }
    
    void SetOnRewardClaimed(std::function<void(int, const std::string&)> cb) { m_OnRewardClaimed = cb; }
    void SetOnLevelUp(std::function<void(int, int)> cb) { m_OnLevelUp = cb; }
};

} // namespace NeoEngine
