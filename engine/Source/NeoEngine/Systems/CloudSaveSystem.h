#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <curl/curl.h>
#include <json/json.h>

namespace NeoEngine {

struct CloudSaveSlot {
    std::string slotId;
    std::string playerId;
    std::string saveData;
    std::string checksum;
    std::chrono::system_clock::time_point lastSaved;
    bool synced = false;
};

class CloudSaveSystem {
private:
    std::vector<CloudSaveSlot> m_Slots;
    std::string m_ServerURL = "https://api.fauzanengine.com/save";
    std::string m_APIKey;
    std::function<void(bool)> m_OnSyncComplete;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
public:
    bool SaveToCloud(const std::string& playerId, const std::string& data) {
        std::string checksum = std::to_string(std::hash<std::string>{}(data));
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        Json::Value body;
        body["playerId"] = playerId;
        body["data"] = data;
        body["checksum"] = checksum;
        body["timestamp"] = (Json::UInt64)std::chrono::system_clock::now().time_since_epoch().count();
        
        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-API-Key: " + m_APIKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, (m_ServerURL + "/upload").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            m_Slots.push_back({playerId, playerId, data, checksum, std::chrono::system_clock::now(), true});
            if (m_OnSyncComplete) m_OnSyncComplete(true);
            return true;
        }
        if (m_OnSyncComplete) m_OnSyncComplete(false);
        return false;
    }
    
    std::string LoadFromCloud(const std::string& playerId) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        std::string response;
        std::string url = m_ServerURL + "/download?playerId=" + playerId;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-API-Key: " + m_APIKey).c_str());
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(response, root)) {
            return root.get("data", "").asString();
        }
        return "";
    }
    
    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetAPIKey(const std::string& key) { m_APIKey = key; }
    void SetOnSyncComplete(std::function<void(bool)> cb) { m_OnSyncComplete = cb; }
};

} // namespace NeoEngine
