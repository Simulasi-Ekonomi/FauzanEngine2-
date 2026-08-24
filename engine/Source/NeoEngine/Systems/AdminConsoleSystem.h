#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <json/json.h>
#include <curl/curl.h>

namespace NeoEngine {

enum class AdminLevel { Moderator=1, SeniorMod=2, Admin=3, SuperAdmin=4, Developer=5 };

struct AdminAction {
    std::string adminId, adminName;
    AdminLevel level;
    std::string action;
    std::string targetPlayer;
    std::string details;
    std::chrono::system_clock::time_point timestamp;
};

class AdminConsoleSystem {
private:
    std::unordered_map<std::string, AdminLevel> m_Admins;
    std::vector<AdminAction> m_ActionLog;
    std::string m_ServerURL = "https://api.fauzanengine.com/admin";
    std::string m_MasterKey;
    int m_TotalActions = 0;

    std::function<void(const AdminAction&)> m_OnAction;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb); return size * nmemb;
    }

public:
    bool RegisterAdmin(const std::string& id, const std::string& name, AdminLevel level, const std::string& masterKey) {
        if(masterKey != m_MasterKey) return false;
        m_Admins[id] = level; return true;
    }

    bool HasPermission(const std::string& adminId, AdminLevel required) {
        auto it = m_Admins.find(adminId);
        return it != m_Admins.end() && it->second >= required;
    }

    bool ExecuteCommand(const std::string& adminId, const std::string& adminName,
                        const std::string& command, const std::string& targetPlayer,
                        const std::string& details = "") {
        auto it = m_Admins.find(adminId);
        if(it == m_Admins.end()) return false;

        AdminAction action{adminId, adminName, it->second, command, targetPlayer, details, std::chrono::system_clock::now()};
        m_ActionLog.push_back(action); m_TotalActions++;

        // Kirim ke server
        CURL* curl = curl_easy_init();
        if(curl) {
            Json::Value body;
            body["adminId"] = adminId;
            body["adminName"] = adminName;
            body["command"] = command;
            body["target"] = targetPlayer;
            body["details"] = details;
            body["level"] = (int)it->second;
            body["timestamp"] = (Json::UInt64)std::chrono::duration_cast<std::chrono::milliseconds>(
                action.timestamp.time_since_epoch()).count();

            Json::FastWriter writer;
            std::string jsonStr = writer.write(body);
            curl_easy_setopt(curl, CURLOPT_URL, (m_ServerURL + "/log").c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }

        if(m_OnAction) m_OnAction(action);
        return true;
    }

    // Perintah admin yang bisa dipanggil
    bool KickPlayer(const std::string& adminId, const std::string& target) { return ExecuteCommand(adminId, "Admin", "kick", target); }
    bool BanPlayer(const std::string& adminId, const std::string& target, const std::string& reason) { return ExecuteCommand(adminId, "Admin", "ban", target, reason); }
    bool GiveItem(const std::string& adminId, const std::string& target, const std::string& item) { return ExecuteCommand(adminId, "Admin", "give_item", target, item); }
    bool GiveCurrency(const std::string& adminId, const std::string& target, int amount) { return ExecuteCommand(adminId, "Admin", "give_currency", target, std::to_string(amount)); }
    bool BroadcastMessage(const std::string& adminId, const std::string& msg) { return ExecuteCommand(adminId, "Admin", "broadcast", "all", msg); }
    bool MutePlayer(const std::string& adminId, const std::string& target, int minutes) { return ExecuteCommand(adminId, "Admin", "mute", target, std::to_string(minutes)); }
    bool TeleportPlayer(const std::string& adminId, const std::string& target, float x, float y, float z) { return ExecuteCommand(adminId, "Admin", "teleport", target, std::to_string(x)+","+std::to_string(y)+","+std::to_string(z)); }

    int GetTotalActions() const { return m_TotalActions; }
    void SetMasterKey(const std::string& key) { m_MasterKey = key; }
    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetOnAction(std::function<void(const AdminAction&)> cb) { m_OnAction = cb; }
};

} // namespace NeoEngine
