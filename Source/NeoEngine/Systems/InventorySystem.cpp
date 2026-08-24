#include "InventorySystem.h"

namespace NeoEngine {

bool InventorySystem::AddItem(const std::string& name, const std::string& type, int count) {
    for (auto& item : m_Items) {
        if (item.name == name && item.count < item.maxStack) {
            item.count += count;
            return true;
        }
    }
    m_Items.push_back({"inv_" + std::to_string(m_Items.size()), name, type, count});
    return true;
}

bool InventorySystem::RemoveItem(const std::string& name, int count) {
    for (auto it = m_Items.begin(); it != m_Items.end(); ++it) {
        if (it->name == name) {
            it->count -= count;
            if (it->count <= 0) m_Items.erase(it);
            return true;
        }
    }
    return false;
}

int InventorySystem::GetItemCount(const std::string& name) const {
    for (auto& item : m_Items)
        if (item.name == name) return item.count;
    return 0;
}

// GetItems, AddGold, GetGold sudah inline di header, tidak perlu di sini

} // namespace NeoEngine
