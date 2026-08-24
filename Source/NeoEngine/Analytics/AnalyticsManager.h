#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct AnalyticsEvent {
    std::string name;
    std::string category;
    std::unordered_map<std::string, std::string> params;
    std::chrono::system_clock::time_point timestamp;
};

struct PlayerMetrics {
    std::string userId;
    int sessions = 0;
    float totalPlayTime = 0;
    float avgSessionLength = 0;
    int purchases = 0;
    float lifetimeValue = 0;
    int level = 1;
    int retentionDay1 = 0;
    int retentionDay7 = 0;
};

class AnalyticsManager {
public:
    static AnalyticsManager& Get() {
        static AnalyticsManager instance;
        return instance;
    }

    void TrackEvent(const std::string& name, const std::string& category,
                    const std::unordered_map<std::string, std::string>& params = {}) {
        AnalyticsEvent event{name, category, params, std::chrono::system_clock::now()};
        m_Events.push_back(event);
        if (m_OnEvent) m_OnEvent(event);
    }

    void TrackPurchase(const std::string& productId, float price) {
        TrackEvent("purchase", "monetization", {{"product", productId}, {"price", std::to_string(price)}});
    }

    void TrackLevelComplete(int level, float time) {
        TrackEvent("level_complete", "gameplay", {{"level", std::to_string(level)}, {"time", std::to_string(time)}});
    }

    void TrackAdWatched(const std::string& type) {
        TrackEvent("ad_watched", "monetization", {{"type", type}});
    }

    PlayerMetrics& GetPlayerMetrics(const std::string& userId) {
        return m_PlayerMetrics[userId];
    }

    void SetEventCallback(std::function<void(const AnalyticsEvent&)> cb) {
        m_OnEvent = cb;
    }

    std::string GetDashboardJSON() const {
        std::string json = "{\"total_events\":" + std::to_string(m_Events.size()) +
                          ",\"active_players\":" + std::to_string(m_PlayerMetrics.size()) + "}";
        return json;
    }

    int GetTotalEvents() const { return m_Events.size(); }

private:
    AnalyticsManager() = default;
    std::vector<AnalyticsEvent> m_Events;
    std::unordered_map<std::string, PlayerMetrics> m_PlayerMetrics;
    std::function<void(const AnalyticsEvent&)> m_OnEvent;
};

} // namespace NeoEngine
