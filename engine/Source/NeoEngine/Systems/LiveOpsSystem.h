#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <curl/curl.h>
#include <json/json.h>

namespace NeoEngine {

struct LiveOpsEvent {
    std::string id, name, description;
    std::chrono::system_clock::time_point startTime, endTime;
    std::string type; // "sale", "event", "update", "maintenance", "push_notification"
    std::unordered_map<std::string, float> parameters;
    bool active = true;
    std::string targetVersion = "all";
};

struct RemoteConfig {
    std::string key;
    std::string value;
    std::string defaultValue;
    std::chrono::system_clock::time_point lastUpdated;
};

class LiveOpsSystem {
private:
    std::vector<LiveOpsEvent> m_Events;
    std::unordered_map<std::string, RemoteConfig> m_Configs;
    std::string m_ServerURL = "https://api.fauzanengine.com/liveops";
    float m_RefreshInterval = 300.0f;
    float m_RefreshTimer = 0;
    std::function<void(const LiveOpsEvent&)> m_OnNewEvent;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
public:
    void Update(float dt) {
        m_RefreshTimer += dt;
        if (m_RefreshTimer >= m_RefreshInterval) {
            m_RefreshTimer = 0;
            RefreshFromServer();
        }
        
        // Nonaktifkan event yang sudah expired
        auto now = std::chrono::system_clock::now();
        for (auto& e : m_Events) {
            if (e.active && now > e.endTime) {
                e.active = false;
            }
        }
    }
    
    void RefreshFromServer() {
        CURL* curl = curl_easy_init();
        if (!curl) return;
        
        std::string responseStr;
        curl_easy_setopt(curl, CURLOPT_URL, (m_ServerURL + "/events").c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(responseStr, root) && root.isArray()) {
                for (const auto& evt : root) {
                    LiveOpsEvent e;
                    e.id = evt.get("id", "").asString();
                    e.name = evt.get("name", "").asString();
                    e.description = evt.get("description", "").asString();
                    e.type = evt.get("type", "").asString();
                    e.active = evt.get("active", true).asBool();
                    if (m_OnNewEvent) m_OnNewEvent(e);
                    m_Events.push_back(e);
                }
            }
        }
    }
    
    std::string GetRemoteConfig(const std::string& key, const std::string& defaultVal = "") {
        auto it = m_Configs.find(key);
        return it != m_Configs.end() ? it->second.value : defaultVal;
    }
    
    std::vector<LiveOpsEvent> GetActiveEvents() const {
        std::vector<LiveOpsEvent> active;
        auto now = std::chrono::system_clock::now();
        for (auto& e : m_Events) {
            if (e.active && now >= e.startTime && now <= e.endTime) {
                active.push_back(e);
            }
        }
        return active;
    }
    
    void SchedulePushNotification(const std::string& title, const std::string& body, int delayMinutes) {
        LiveOpsEvent e;
        e.id = "push_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        e.name = title;
        e.description = body;
        e.type = "push_notification";
        e.startTime = std::chrono::system_clock::now() + std::chrono::minutes(delayMinutes);
        e.endTime = e.startTime + std::chrono::hours(1);
        m_Events.push_back(e);
    }
    
    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetOnNewEvent(std::function<void(const LiveOpsEvent&)> cb) { m_OnNewEvent = cb; }
};

} // namespace NeoEngine
