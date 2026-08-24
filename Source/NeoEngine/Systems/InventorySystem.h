#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace NeoEngine {

struct InventoryItem {
    std::string id;
    std::string name;
    std::string type;
    int count = 1;
    int maxStack = 99;
    int value = 0;
    float damage = 0;
    float defense = 0;
    float healAmount = 0;
};

class InventorySystem {
public:
    bool AddItem(const std::string& name, const std::string& type, int count = 1);
    bool RemoveItem(const std::string& name, int count = 1);
    int GetItemCount(const std::string& name) const;
    bool HasItem(const std::string& name) const;
    const std::vector<InventoryItem>& GetItems() const { return m_Items; }
    int GetGold() const { return m_Gold; }
    void AddGold(int amount) { m_Gold += amount; }
    bool SpendGold(int amount) { if (m_Gold < amount) return false; m_Gold -= amount; return true; }

private:
    std::vector<InventoryItem> m_Items;
    int m_MaxSlots = 50;
    int m_Gold = 0;
};

} // namespace NeoEngine
