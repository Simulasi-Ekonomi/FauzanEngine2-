#pragma once
#include <string>
#include <chrono>
#include <functional>

namespace NeoEngine {

struct OfflineReward {
    float hoursOffline = 0;
    int gold = 0;
    int exp = 0;
    int energy = 0;
    std::vector<std::string> items;
};

class OfflineRewardSystem {
private:
    std::chrono::system_clock::time_point m_LastOnline;
    int m_MaxHours = 12;
    int m_GoldPerHour = 50;
    int m_ExpPerHour = 25;
    int m_EnergyPerHour = 10;
    std::function<void(const OfflineReward&)> m_OnClaim;
    
public:
    OfflineRewardSystem() {
        m_LastOnline = std::chrono::system_clock::now();
    }
    
    OfflineReward CalculateReward() {
        auto now = std::chrono::system_clock::now();
        float hours = std::chrono::duration_cast<std::chrono::minutes>(now - m_LastOnline).count() / 60.0f;
        hours = std::min(hours, (float)m_MaxHours);
        
        OfflineReward reward;
        reward.hoursOffline = hours;
        reward.gold = (int)(hours * m_GoldPerHour);
        reward.exp = (int)(hours * m_ExpPerHour);
        reward.energy = (int)(hours * m_EnergyPerHour);
        
        if (hours >= 8) reward.items.push_back("Rare Chest");
        if (hours >= 24) reward.items.push_back("Epic Chest");
        
        m_LastOnline = now;
        return reward;
    }
    
    OfflineReward ClaimReward() {
        auto reward = CalculateReward();
        if (m_OnClaim) m_OnClaim(reward);
        return reward;
    }
    
    void SetMaxHours(int h) { m_MaxHours = h; }
    void SetGoldPerHour(int g) { m_GoldPerHour = g; }
    void SetExpPerHour(int e) { m_ExpPerHour = e; }
    void SetOnClaim(std::function<void(const OfflineReward&)> cb) { m_OnClaim = cb; }
};

} // namespace NeoEngine
