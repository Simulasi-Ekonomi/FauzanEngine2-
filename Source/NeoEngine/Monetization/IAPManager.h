#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct IAPProduct {
    std::string id;
    std::string name;
    std::string description;
    float price = 0.99f;
    std::string currency = "USD";
    std::string type; // "consumable", "non_consumable", "subscription"
    bool active = true;
};

struct PurchaseRecord {
    std::string productId;
    std::string userId;
    float price;
    std::string timestamp;
    bool verified = false;
};

class IAPManager {
public:
    static IAPManager& Get() {
        static IAPManager instance;
        return instance;
    }

    void AddProduct(const IAPProduct& product) {
        m_Products[product.id] = product;
    }

    const IAPProduct* GetProduct(const std::string& id) const {
        auto it = m_Products.find(id);
        return it != m_Products.end() ? &it->second : nullptr;
    }

    std::vector<IAPProduct> GetAllProducts() const {
        std::vector<IAPProduct> products;
        for (auto& [id, p] : m_Products) products.push_back(p);
        return products;
    }

    bool ProcessPurchase(const std::string& productId, const std::string& userId) {
        auto* product = GetProduct(productId);
        if (!product || !product->active) return false;
        PurchaseRecord record{productId, userId, product->price, "now", true};
        m_History.push_back(record);
        m_TotalRevenue += product->price;
        if (m_OnPurchase) m_OnPurchase(record);
        return true;
    }

    float GetTotalRevenue() const { return m_TotalRevenue; }
    int GetPurchaseCount() const { return m_History.size(); }

    void SetPurchaseCallback(std::function<void(const PurchaseRecord&)> cb) {
        m_OnPurchase = cb;
    }

    std::string GetAnalyticsJSON() const {
        std::string json = "{\"products\":" + std::to_string(m_Products.size()) +
                          ",\"purchases\":" + std::to_string(m_History.size()) +
                          ",\"revenue\":" + std::to_string(m_TotalRevenue) + "}";
        return json;
    }

private:
    IAPManager() = default;
    std::unordered_map<std::string, IAPProduct> m_Products;
    std::vector<PurchaseRecord> m_History;
    float m_TotalRevenue = 0;
    std::function<void(const PurchaseRecord&)> m_OnPurchase;
};

} // namespace NeoEngine
