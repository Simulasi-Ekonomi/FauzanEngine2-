#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

enum class CosmeticType { Skin, Avatar, Frame, Emote, Effect, Pet, Mount, Title, Banner };

struct CosmeticItem {
    std::string id;
    std::string name;
    CosmeticType type;
    int price = 0;
    std::string currency = "gold";
    int rarity = 1;
    bool limited = false;
    int stock = -1;
    int ownedCount = 0;
};

class CosmeticShopSystem {
private:
    std::vector<CosmeticItem> m_Shop;
    std::vector<CosmeticItem> m_Inventory;
    std::unordered_map<std::string, CosmeticItem*> m_Equipped;
    int m_Gold = 0;
    int m_Gems = 0;
    std::function<void(const CosmeticItem&)> m_OnPurchase;
    std::function<void(const CosmeticItem&)> m_OnEquip;

public:
    CosmeticShopSystem() {
        m_Shop = {
            {"skin_fire", "Fire Demon Skin", CosmeticType::Skin, 500, "gold", 4},
            {"skin_ice", "Ice Queen Skin", CosmeticType::Skin, 500, "gold", 4},
            {"avatar_crown", "Crown Avatar", CosmeticType::Avatar, 200, "gold", 3},
            {"frame_gold", "Golden Frame", CosmeticType::Frame, 300, "gold", 3},
            {"emote_dab", "Dab Emote", CosmeticType::Emote, 100, "gold", 1},
            {"emote_floss", "Floss Emote", CosmeticType::Emote, 100, "gold", 1},
            {"effect_lightning", "Lightning Trail", CosmeticType::Effect, 800, "gold", 4},
            {"pet_dragon", "Baby Dragon", CosmeticType::Pet, 2000, "gems", 5, true, 100},
            {"mount_phoenix", "Phoenix Mount", CosmeticType::Mount, 5000, "gems", 5, true, 50},
            {"title_legend", "The Legend", CosmeticType::Title, 1000, "gems", 4},
            {"banner_warrior", "Warrior Banner", CosmeticType::Banner, 400, "gold", 2},
        };
    }

    bool PurchaseItem(const std::string& id) {
        for (auto& item : m_Shop) {
            if (item.id == id) {
                int cost = item.price;
                if (item.currency == "gold" && m_Gold >= cost) { m_Gold -= cost; }
                else if (item.currency == "gems" && m_Gems >= cost) { m_Gems -= cost; }
                else return false;
                if (item.stock > 0) item.stock--;
                item.ownedCount++;
                m_Inventory.push_back(item);
                if (m_OnPurchase) m_OnPurchase(item);
                return true;
            }
        }
        return false;
    }

    bool EquipItem(const std::string& id) {
        for (auto& item : m_Inventory) {
            if (item.id == id && item.ownedCount > 0) {
                m_Equipped[std::to_string((int)item.type)] = &item;
                if (m_OnEquip) m_OnEquip(item);
                return true;
            }
        }
        return false;
    }

    const CosmeticItem* GetEquipped(CosmeticType type) const {
        auto it = m_Equipped.find(std::to_string((int)type));
        return it != m_Equipped.end() ? it->second : nullptr;
    }

    const std::vector<CosmeticItem>& GetShop() const { return m_Shop; }
    const std::vector<CosmeticItem>& GetInventory() const { return m_Inventory; }
    void AddGold(int a) { m_Gold += a; }
    void AddGems(int a) { m_Gems += a; }
    int GetGold() const { return m_Gold; }
    int GetGems() const { return m_Gems; }
    void SetOnPurchase(std::function<void(const CosmeticItem&)> cb) { m_OnPurchase = cb; }
    void SetOnEquip(std::function<void(const CosmeticItem&)> cb) { m_OnEquip = cb; }
};

} // namespace NeoEngine
