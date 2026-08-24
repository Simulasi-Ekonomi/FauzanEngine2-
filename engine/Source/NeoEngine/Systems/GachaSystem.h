#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {

struct GachaItem {
    std::string name;
    int rarity; // 1-5 stars
    float dropRate;
    std::string type; // "character", "weapon", "skin", "currency"
    int count = 1;
};

struct GachaBanner {
    std::string name;
    std::string featured;
    int cost;
    std::vector<GachaItem> items;
    int pityCounter = 0;
    int softPityStart = 75;
    int hardPity = 90;
};

class GachaSystem {
private:
    std::vector<GachaBanner> m_Banners;
    int m_Currency = 0;
    int m_TotalPulls = 0;
    std::function<void(const GachaItem&)> m_OnPull;

public:
    GachaSystem() { SetupDefaultBanner(); }

    void SetupDefaultBanner() {
        GachaBanner banner{"Starter Banner", "SSR Character", 160};
        banner.items = {
            {"SSR Character", 5, 0.6f, "character"},
            {"SSR Weapon", 5, 1.0f, "weapon"},
            {"SR Character", 4, 5.0f, "character"},
            {"SR Weapon", 4, 8.0f, "weapon"},
            {"R Character", 3, 30.0f, "character"},
            {"R Weapon", 3, 55.4f, "weapon"},
        };
        m_Banners.push_back(banner);
    }

    void AddCurrency(int amount) { m_Currency += amount; }

    GachaItem Pull(int bannerIndex) {
        if (bannerIndex < 0 || bannerIndex >= m_Banners.size()) return {};
        auto& banner = m_Banners[bannerIndex];
        banner.pityCounter++;
        m_TotalPulls++;

        float roll = (float)(rand() % 10000) / 100.0f;
        float cumRate = 0;
        GachaItem result;

        // Soft pity logic
        if (banner.pityCounter >= banner.softPityStart) {
            float pityBoost = (banner.pityCounter - banner.softPityStart) * 5.0f;
            // Boost 5-star rates
            for (auto& item : banner.items) {
                if (item.rarity >= 5) item.dropRate += pityBoost;
            }
        }

        // Hard pity: guarantee 5-star
        if (banner.pityCounter >= banner.hardPity) {
            for (auto& item : banner.items) {
                if (item.rarity >= 5) {
                    result = item;
                    banner.pityCounter = 0;
                    if (m_OnPull) m_OnPull(result);
                    return result;
                }
            }
        }

        // Normal roll
        for (auto& item : banner.items) {
            cumRate += item.dropRate;
            if (roll <= cumRate) {
                result = item;
                if (item.rarity >= 5) banner.pityCounter = 0;
                if (m_OnPull) m_OnPull(result);
                return result;
            }
        }

        return result;
    }

    GachaItem Pull10(int bannerIndex) {
        // Simpan hasil pull ke-10 (biasanya ada garansi 4-star+)
        GachaItem lastResult;
        for (int i = 0; i < 10; i++) {
            lastResult = Pull(bannerIndex);
        }
        return lastResult;
    }

    int GetCurrency() const { return m_Currency; }
    int GetTotalPulls() const { return m_TotalPulls; }
    int GetPityCounter(int bannerIndex) const {
        if (bannerIndex < 0 || bannerIndex >= m_Banners.size()) return 0;
        return m_Banners[bannerIndex].pityCounter;
    }
    void SetOnPull(std::function<void(const GachaItem&)> cb) { m_OnPull = cb; }
};

} // namespace NeoEngine
