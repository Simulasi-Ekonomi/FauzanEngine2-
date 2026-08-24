#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace NeoEngine {

enum class VIPLevel { None, Silver, Gold, Diamond, Legend };

struct VIPPerk {
    std::string name;
    float value;
    std::string description;
};

class VIPSubscriptionSystem {
private:
    VIPLevel m_CurrentLevel = VIPLevel::None;
    std::chrono::system_clock::time_point m_ExpiryDate;
    bool m_AutoRenew = false;
    float m_TotalSpent = 0;
    std::vector<VIPPerk> m_Perks;
    std::function<void(VIPLevel)> m_OnLevelUp;

public:
    VIPSubscriptionSystem() {
        m_Perks = {
            {"EXP Boost", 1.2f, "Earn 20% more EXP"},
            {"Gold Boost", 1.3f, "Earn 30% more Gold"},
            {"Free Daily Summon", 1.0f, "One free summon per day"},
            {"Skip Ads", 1.0f, "Skip all rewarded ads"},
            {"Exclusive Skins", 1.0f, "Access to VIP-exclusive skins"},
            {"Priority Support", 1.0f, "Get priority customer support"},
            {"Extra Inventory", 1.5f, "50% more inventory slots"},
            {"Daily Login Bonus", 1.0f, "Double daily login rewards"},
        };
    }

    bool PurchaseVIP(VIPLevel level, int days) {
        float cost = GetVIPCost(level, days);
        m_TotalSpent += cost;
        m_CurrentLevel = level;
        m_ExpiryDate = std::chrono::system_clock::now() + std::chrono::hours(24 * days);
        if (m_OnLevelUp) m_OnLevelUp(level);
        return true;
    }

    float GetVIPCost(VIPLevel level, int days) const {
        switch (level) {
            case VIPLevel::Silver: return days * 0.5f;
            case VIPLevel::Gold: return days * 0.8f;
            case VIPLevel::Diamond: return days * 1.5f;
            case VIPLevel::Legend: return days * 3.0f;
            default: return 0;
        }
    }

    bool IsVIPActive() const {
        if (m_CurrentLevel == VIPLevel::None) return false;
        return std::chrono::system_clock::now() < m_ExpiryDate;
    }

    float GetPerkValue(const std::string& name) const {
        if (!IsVIPActive()) return 1.0f;
        for (auto& p : m_Perks) {
            if (p.name == name) return p.value;
        }
        return 1.0f;
    }

    VIPLevel GetCurrentLevel() const { return m_CurrentLevel; }
    void SetAutoRenew(bool renew) { m_AutoRenew = renew; }
    std::string GetLevelName() const {
        switch (m_CurrentLevel) {
            case VIPLevel::Silver: return "Silver";
            case VIPLevel::Gold: return "Gold";
            case VIPLevel::Diamond: return "Diamond";
            case VIPLevel::Legend: return "Legend";
            default: return "None";
        }
    }
    void SetOnLevelUp(std::function<void(VIPLevel)> cb) { m_OnLevelUp = cb; }
};

} // namespace NeoEngine
