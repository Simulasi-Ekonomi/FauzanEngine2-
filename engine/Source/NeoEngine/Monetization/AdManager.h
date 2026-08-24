#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

enum class AdType { Banner, Interstitial, Rewarded, Native };

struct AdPlacement {
    std::string id;
    AdType type;
    int frequencyPerSession = 3;
    int cooldownSeconds = 120;
    bool enabled = true;
};

class AdManager {
public:
    static AdManager& Get() {
        static AdManager instance;
        return instance;
    }

    void AddPlacement(const AdPlacement& placement) {
        m_Placements[placement.id] = placement;
    }

    bool ShowAd(const std::string& placementId) {
        auto it = m_Placements.find(placementId);
        if (it == m_Placements.end() || !it->second.enabled) return false;
        m_Impressions++;
        m_Revenue += 0.01f; // estimasi per impression
        if (m_OnAdShown) m_OnAdShown(placementId);
        return true;
    }

    void ShowRewardedAd(std::function<void(bool)> callback) {
        m_Impressions++;
        m_Revenue += 0.05f; // rewarded ads biasanya lebih mahal
        if (callback) callback(true);
    }

    float GetRevenue() const { return m_Revenue; }
    int GetImpressions() const { return m_Impressions; }

    void SetAdCallback(std::function<void(const std::string&)> cb) {
        m_OnAdShown = cb;
    }

    std::string GetAnalyticsJSON() const {
        return "{\"impressions\":" + std::to_string(m_Impressions) +
               ",\"revenue\":" + std::to_string(m_Revenue) + "}";
    }

private:
    AdManager() = default;
    std::unordered_map<std::string, AdPlacement> m_Placements;
    int m_Impressions = 0;
    float m_Revenue = 0;
    std::function<void(const std::string&)> m_OnAdShown;
};

} // namespace NeoEngine
