#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace NeoEngine {

struct IdleUpgrade {
    std::string name;
    int level = 0;
    int maxLevel = 100;
    float baseCost = 10;
    float costMultiplier = 1.15f;
    float incomePerSecond = 0;
    float incomeMultiplier = 1.0f;
};

class IdleGameSystem {
private:
    std::vector<IdleUpgrade> m_Upgrades;
    float m_Money = 0;
    float m_TotalMoneyEarned = 0;
    float m_MoneyPerSecond = 1.0f;
    int m_PrestigeCount = 0;
    float m_PrestigeMultiplier = 1.0f;

public:
    IdleGameSystem() {
        m_Upgrades.push_back({"Lemonade Stand", 0, 100, 5, 1.12f, 1.0f, 1.0f});
        m_Upgrades.push_back({"Newspaper Delivery", 0, 80, 20, 1.14f, 3.0f, 1.0f});
        m_Upgrades.push_back({"Car Wash", 0, 60, 100, 1.16f, 10.0f, 1.0f});
        m_Upgrades.push_back({"Pizza Shop", 0, 50, 500, 1.18f, 30.0f, 1.0f});
        m_Upgrades.push_back({"Software Company", 0, 10, 5000, 1.20f, 200.0f, 1.0f});
    }

    bool BuyUpgrade(int index) {
        if (index < 0 || index >= m_Upgrades.size()) return false;
        auto& u = m_Upgrades[index];
        if (u.level >= u.maxLevel) return false;
        float cost = GetUpgradeCost(index);
        if (m_Money < cost) return false;
        m_Money -= cost;
        m_MoneyPerSecond += u.incomePerSecond * m_PrestigeMultiplier;
        u.level++;
        return true;
    }

    float GetUpgradeCost(int index) const {
        auto& u = m_Upgrades[index];
        return u.baseCost * pow(u.costMultiplier, u.level) * pow(1.02f, m_PrestigeCount);
    }

    void Update(float dt) {
        float income = m_MoneyPerSecond * dt * m_PrestigeMultiplier;
        m_Money += income;
        m_TotalMoneyEarned += income;
    }

    bool CanPrestige() { return m_Money >= 1000000 * pow(2, m_PrestigeCount); }

    void Prestige() {
        if (!CanPrestige()) return;
        m_PrestigeCount++;
        m_PrestigeMultiplier = 1.0f + m_PrestigeCount * 0.5f;
        m_Money = 0;
        for (auto& u : m_Upgrades) u.level = 0;
    }

    float GetMoney() const { return m_Money; }
    float GetMoneyPerSecond() const { return m_MoneyPerSecond * m_PrestigeMultiplier; }
    int GetPrestigeCount() const { return m_PrestigeCount; }
    const std::vector<IdleUpgrade>& GetUpgrades() const { return m_Upgrades; }
};

} // namespace NeoEngine
