#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>

#define LOG_TAG "ServerAuth"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

struct AuthorityRequest {
    std::string requestId;
    std::string playerId;
    std::string action; // "spawn_item", "add_currency", "level_up", "complete_quest", "open_chest"
    std::string params;
    std::chrono::system_clock::time_point timestamp;
    bool approved = false;
    std::string serverHash;
};

struct AuthorityResponse {
    bool valid = false;
    std::string serverHash;
    std::string message;
    std::string rewardSerial; // untuk item yang di-spawn
};

class ServerAuthorityValidator {
private:
    std::vector<AuthorityRequest> m_Requests;
    std::string m_ServerURL = "https://api.fauzanengine.com/validate";
    std::string m_ServerKey;
    mutable std::mutex m_Mutex;
    int m_ApprovedCount = 0;
    int m_RejectedCount = 0;
    
    std::function<void(const AuthorityRequest&)> m_OnRejected;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    // Semua aksi penting HARUS divalidasi server
    AuthorityResponse ValidateAction(const std::string& playerId, const std::string& action,
                                     const std::string& params = "") {
        AuthorityResponse response;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOGE("CURL init failed");
            response.valid = false;
            response.message = "Network error";
            return response;
        }
        
        Json::Value body;
        body["playerId"] = playerId;
        body["action"] = action;
        body["params"] = params;
        body["timestamp"] = (Json::UInt64)std::chrono::system_clock::now().time_since_epoch().count();
        body["appVersion"] = "2.0.0";
        body["checksum"] = std::to_string(std::hash<std::string>{}(playerId + action + params));
        
        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Server-Key: " + m_ServerKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, m_ServerURL.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        std::string responseStr;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            LOGE("Server validation failed for %s", action.c_str());
            response.valid = false;
            response.message = "Server unreachable";
            // IMPORTANT: If server is unreachable, REJECT the action (secure by default)
            if (m_OnRejected) {
                AuthorityRequest req{"req_" + std::to_string(m_Requests.size()), 
                                    playerId, action, params, std::chrono::system_clock::now(), false, ""};
                m_OnRejected(req);
            }
            return response;
        }
        
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(responseStr, root)) {
            response.valid = root.get("valid", false).asBool();
            response.serverHash = root.get("serverHash", "").asString();
            response.message = root.get("message", "").asString();
            response.rewardSerial = root.get("rewardSerial", "").asString();
            
            if (response.valid) m_ApprovedCount++;
            else {
                m_RejectedCount++;
                if (m_OnRejected) {
                    AuthorityRequest req{"req_" + std::to_string(m_Requests.size()), 
                                        playerId, action, params, std::chrono::system_clock::now(), false, ""};
                    m_OnRejected(req);
                }
            }
            
            LOGI("Server validation: %s - %s", action.c_str(), response.valid ? "APPROVED" : "REJECTED");
        }
        
        return response;
    }
    
    // Validasi batch untuk efisiensi
    void ValidateBatch(const std::vector<std::tuple<std::string, std::string, std::string>>& actions) {
        for (auto& [playerId, action, params] : actions) {
            ValidateAction(playerId, action, params);
        }
    }
    
    int GetApprovedCount() const { return m_ApprovedCount; }
    int GetRejectedCount() const { return m_RejectedCount; }
    
    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetServerKey(const std::string& key) { m_ServerKey = key; }
    void SetOnRejected(std::function<void(const AuthorityRequest&)> cb) { m_OnRejected = cb; }
};

} // namespace NeoEngine
