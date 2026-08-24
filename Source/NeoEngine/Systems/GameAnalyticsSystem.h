#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <curl/curl.h>
#include <json/json.h>

namespace NeoEngine {

struct AnalyticsEvent {
    std::string eventName;
    std::string eventCategory;
    std::unordered_map<std::string, std::string> params;
    std::chrono::system_clock::time_point timestamp;
    bool sent = false;
};

struct GameAnalytics {
    int dau = 0; // Daily Active Users
    int mau = 0; // Monthly Active Users
    int newUsers = 0;
    int returningUsers = 0;
    float avgSessionTime = 0;
    float retentionD1 = 0;
    float retentionD7 = 0;
    float retentionD30 = 0;
    float arpu = 0; // Average Revenue Per User
    float arppu = 0; // Average Revenue Per Paying User
    int payingUsers = 0;
    int totalPurchases = 0;
    float totalRevenue = 0;
    float conversionRate = 0;
};

class GameAnalyticsSystem {
private:
    std::vector<AnalyticsEvent> m_Events;
    GameAnalytics m_Analytics;
    std::string m_AnalyticsServerURL = "https://api.fauzanengine.com/analytics";
    std::string m_GameKey = "fauzanengine_prod";
    std::function<void(const GameAnalytics&)> m_OnDashboardUpdate;
    float m_SendTimer = 0;
    float m_SendInterval = 60.0f; // kirim setiap 60 detik
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
public:
    void TrackEvent(const std::string& name, const std::string& category,
                    const std::unordered_map<std::string, std::string>& params = {}) {
        m_Events.push_back({name, category, params, std::chrono::system_clock::now(), false});
    }
    
    void TrackSessionStart(const std::string& userId) {
        TrackEvent("session_start", "engagement", {{"userId", userId}});
        m_Analytics.dau++;
    }
    
    void TrackPurchase(const std::string& userId, const std::string& productId, float price, const std::string& currency) {
        TrackEvent("purchase", "monetization", {
            {"userId", userId}, {"productId", productId}, 
            {"price", std::to_string(price)}, {"currency", currency}
        });
        m_Analytics.totalPurchases++;
        m_Analytics.totalRevenue += price;
        m_Analytics.payingUsers++;
    }
    
    void TrackLevelStart(const std::string& userId, int level) {
        TrackEvent("level_start", "gameplay", {{"userId", userId}, {"level", std::to_string(level)}});
    }
    
    void TrackLevelComplete(const std::string& userId, int level, int score, float timeSeconds) {
        TrackEvent("level_complete", "gameplay", {
            {"userId", userId}, {"level", std::to_string(level)},
            {"score", std::to_string(score)}, {"time", std::to_string(timeSeconds)}
        });
    }
    
    void TrackAdWatched(const std::string& userId, const std::string& adType, const std::string& placement) {
        TrackEvent("ad_watched", "advertising", {
            {"userId", userId}, {"adType", adType}, {"placement", placement}
        });
    }
    
    void TrackError(const std::string& errorType, const std::string& message, const std::string& stackTrace = "") {
        TrackEvent("error", "technical", {
            {"errorType", errorType}, {"message", message}, {"stackTrace", stackTrace}
        });
    }
    
    void TrackSocialAction(const std::string& userId, const std::string& action) {
        TrackEvent("social", "social", {{"userId", userId}, {"action", action}});
    }
    
    void Update(float dt) {
        m_SendTimer += dt;
        if (m_SendTimer >= m_SendInterval && !m_Events.empty()) {
            m_SendTimer = 0;
            SendBatchToServer();
        }
    }
    
    void SendBatchToServer() {
        CURL* curl = curl_easy_init();
        if (!curl) return;
        
        Json::Value batch(Json::arrayValue);
        int count = 0;
        for (auto& e : m_Events) {
            if (e.sent) continue;
            if (count >= 50) break; // max batch 50
            
            Json::Value evt;
            evt["name"] = e.eventName;
            evt["category"] = e.eventCategory;
            evt["timestamp"] = (Json::UInt64)std::chrono::duration_cast<std::chrono::milliseconds>(
                e.timestamp.time_since_epoch()).count();
            Json::Value params;
            for (auto& [k, v] : e.params) params[k] = v;
            evt["params"] = params;
            evt["gameKey"] = m_GameKey;
            
            batch.append(evt);
            e.sent = true;
            count++;
        }
        
        Json::FastWriter writer;
        std::string jsonBody = writer.write(batch);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, (m_AnalyticsServerURL + "/batch").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    GameAnalytics GetDashboard() const { return m_Analytics; }
    int GetEventCount() const { return m_Events.size(); }
    void SetGameKey(const std::string& key) { m_GameKey = key; }
    void SetAnalyticsServer(const std::string& url) { m_AnalyticsServerURL = url; }
};

} // namespace NeoEngine
