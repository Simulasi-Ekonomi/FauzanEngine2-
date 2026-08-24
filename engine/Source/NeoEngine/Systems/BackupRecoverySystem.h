#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <curl/curl.h>
#include <json/json.h>

namespace NeoEngine {

struct BackupSnapshot {
    std::string playerId;
    std::string playerName;
    std::string data;
    std::string checksum;
    std::chrono::system_clock::time_point timestamp;
    int version = 1;
};

class BackupRecoverySystem {
private:
    std::unordered_map<std::string, std::vector<BackupSnapshot>> m_Backups;
    int m_MaxBackupsPerPlayer = 10;
    std::string m_ServerURL = "https://api.fauzanengine.com/backup";
    std::string m_APIKey;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
public:
    bool CreateBackup(const std::string& playerId, const std::string& playerName, const std::string& data) {
        auto& backups = m_Backups[playerId];
        if (backups.size() >= m_MaxBackupsPerPlayer) {
            backups.erase(backups.begin()); // Hapus backup terlama
        }
        
        std::string checksum = std::to_string(std::hash<std::string>{}(data));
        backups.push_back({playerId, playerName, data, checksum, std::chrono::system_clock::now(), (int)backups.size() + 1});
        
        // Simpan ke cloud
        SaveToCloud(backups.back());
        return true;
    }
    
    std::string RestoreBackup(const std::string& playerId, int version = -1) {
        auto it = m_Backups.find(playerId);
        if (it == m_Backups.end() || it->second.empty()) {
            // Coba ambil dari cloud
            return LoadFromCloud(playerId, version);
        }
        
        if (version < 0 || version >= it->second.size()) {
            return it->second.back().data; // Kembalikan versi terbaru
        }
        return it->second[version].data;
    }
    
    void SaveToCloud(const BackupSnapshot& backup) {
        CURL* curl = curl_easy_init();
        if (!curl) return;
        
        Json::Value body;
        body["playerId"] = backup.playerId;
        body["playerName"] = backup.playerName;
        body["data"] = backup.data;
        body["checksum"] = backup.checksum;
        body["version"] = backup.version;
        body["timestamp"] = (Json::UInt64)std::chrono::duration_cast<std::chrono::milliseconds>(
            backup.timestamp.time_since_epoch()).count();
        
        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-API-Key: " + m_APIKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, (m_ServerURL + "/save").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    std::string LoadFromCloud(const std::string& playerId, int version = -1) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        std::string url = m_ServerURL + "/load?playerId=" + playerId;
        if (version >= 0) url += "&version=" + std::to_string(version);
        
        std::string response;
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
    
    void SetMaxBackups(int max) { m_MaxBackupsPerPlayer = max; }
    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetAPIKey(const std::string& key) { m_APIKey = key; }
};

} // namespace NeoEngine
