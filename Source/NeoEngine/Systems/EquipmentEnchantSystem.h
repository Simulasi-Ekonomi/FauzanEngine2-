#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {
struct Equipment { std::string name; int level=1; int enchantLevel=0; int maxEnchant=15; int baseStat=10; std::string type; };
struct EnchantResult { bool success; int newLevel; bool destroyed; std::string bonusEffect; };

class EquipmentEnchantSystem {
private:
    std::vector<Equipment> m_Equipment;
    std::function<void(const Equipment&, const EnchantResult&)> m_OnEnchant;
public:
    Equipment* CreateEquipment(const std::string& name, const std::string& type, int baseStat=10) {
        m_Equipment.push_back({name, 1, 0, 15, baseStat, type}); return &m_Equipment.back();
    }
    EnchantResult EnchantItem(int index) {
        if (index < 0 || index >= m_Equipment.size()) return {false, 0, false, ""};
        auto& eq = m_Equipment[index]; if (eq.enchantLevel >= eq.maxEnchant) return {false, eq.enchantLevel, false, ""};
        int successRate = 100 - (eq.enchantLevel * 7); if (successRate < 5) successRate = 5;
        bool success = (rand() % 100) < successRate;
        if (success) {
            eq.enchantLevel++; eq.baseStat += eq.baseStat / 10;
            if (eq.enchantLevel % 5 == 0) eq.baseStat += 5; // Bonus setiap +5
        } else if (eq.enchantLevel >= 10 && (rand() % 100) < 10) { eq.enchantLevel = 0; eq.baseStat = eq.baseStat / 2; }
        EnchantResult res{success, eq.enchantLevel, false, eq.enchantLevel % 5 == 0 ? "Bonus Stat +5" : ""};
        if (m_OnEnchant) m_OnEnchant(eq, res); return res;
    }
    const auto& GetEquipment() const { return m_Equipment; }
    void SetOnEnchant(std::function<void(const Equipment&, const EnchantResult&)> cb) { m_OnEnchant = cb; }
};
}
