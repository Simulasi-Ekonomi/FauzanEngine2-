#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct TransmogAppearance {
    std::string id, name, sourceItem;
    int cost = 0;
    bool unlocked = false;
};

struct TransmogCollection {
    std::string playerId;
    std::vector<std::string> unlockedIds;
    std::unordered_map<std::string, std::string> appliedLooks; // slot -> appearance id
};

class TransmogSystem {
private:
    std::vector<TransmogAppearance> m_Appearances;
    std::unordered_map<std::string, TransmogCollection> m_Collections;
    int m_TransmogCurrency = 0;
    std::function<void(const std::string&, const TransmogAppearance&)> m_OnUnlock;
    
public:
    TransmogSystem() {
        m_Appearances = {
            {"iron_helm", "Iron Helmet Look", "Iron Helmet", 50, false},
            {"gold_helm", "Golden Helmet Look", "Golden Helmet", 200, false},
            {"dragon_helm", "Dragon Helmet Look", "Dragon Helmet", 500, false},
            {"mage_robe", "Mage Robe Look", "Mage Robe", 75, false},
            {"knight_armor", "Knight Armor Look", "Knight Armor", 150, false},
            {"shadow_cloak", "Shadow Cloak Look", "Shadow Cloak", 300, false},
            {"phoenix_sword", "Phoenix Sword Look", "Phoenix Sword", 400, false},
            {"thunder_axe", "Thunder Axe Look", "Thunder Axe", 350, false},
        };
    }
    
    bool UnlockAppearance(const std::string& playerId, const std::string& appearanceId) {
        for (auto& a : m_Appearances) {
            if (a.id == appearanceId) {
                a.unlocked = true;
                m_Collections[playerId].unlockedIds.push_back(appearanceId);
                if (m_OnUnlock) m_OnUnlock(playerId, a);
                return true;
            }
        }
        return false;
    }
    
    bool ApplyLook(const std::string& playerId, const std::string& slot, const std::string& appearanceId) {
        auto it = m_Collections.find(playerId);
        if (it == m_Collections.end()) return false;
        for (auto& id : it->second.unlockedIds) {
            if (id == appearanceId) {
                it->second.appliedLooks[slot] = appearanceId;
                return true;
            }
        }
        return false;
    }
    
    std::string GetAppliedLook(const std::string& playerId, const std::string& slot) const {
        auto it = m_Collections.find(playerId);
        if (it != m_Collections.end()) {
            auto sit = it->second.appliedLooks.find(slot);
            if (sit != it->second.appliedLooks.end()) return sit->second;
        }
        return "";
    }
    
    const std::vector<TransmogAppearance>& GetAppearances() const { return m_Appearances; }
    const std::vector<std::string>* GetUnlocked(const std::string& playerId) const {
        auto it = m_Collections.find(playerId);
        return it != m_Collections.end() ? &it->second.unlockedIds : nullptr;
    }
    void AddCurrency(int amount) { m_TransmogCurrency += amount; }
    void SetOnUnlock(std::function<void(const std::string&, const TransmogAppearance&)> cb) { m_OnUnlock = cb; }
};

} // namespace NeoEngine
