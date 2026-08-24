#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <random>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <android/log.h>
#include <sstream>
#include <iomanip>

#define LOG_TAG "FEAuth"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

struct FEUserAccount {
    std::string userId;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::string salt;
    int failedAttempts = 0;
    bool locked = false;
    std::chrono::system_clock::time_point lockUntil;
    bool twoFactorEnabled = false;
    std::string twoFactorSecret;
    std::vector<std::string> linkedDevices;
    std::vector<std::string> loginHistory;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastLogin;
};

struct FEAuthToken {
    std::string token;
    std::string userId;
    std::string deviceId;
    std::chrono::system_clock::time_point issuedAt;
    std::chrono::system_clock::time_point expiresAt;
    bool valid = true;
};

class FEAuthenticationServer {
private:
    std::unordered_map<std::string, FEUserAccount> m_Users;
    std::vector<FEAuthToken> m_ActiveTokens;
    std::vector<std::string> m_BannedUsers;
    mutable std::mutex m_Mutex;
    std::string m_ServerURL = "https://auth.fauzanengine.com";
    std::string m_AppSecret;
    int m_MaxFailedAttempts = 5;
    int m_LockoutMinutes = 30;
    int m_TokenExpiryHours = 168;

    std::function<void(const std::string&)> m_OnLogin;
    std::function<void(const std::string&)> m_OnLoginFailed;
    std::function<void(const std::string&)> m_OnAccountLocked;

    std::string HashPassword(const std::string& password, const std::string& salt) {
        std::string raw = password + salt + m_AppSecret;
        unsigned char hash[32];
        SHA256((unsigned char*)raw.c_str(), raw.length(), hash);
        std::stringstream ss;
        for (int i = 0; i < 32; i++) ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        return ss.str();
    }

    std::string GenerateSalt() {
        return std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }

    std::string GenerateToken(const std::string& userId) {
        std::string raw = userId + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        unsigned char hash[32];
        SHA256((unsigned char*)raw.c_str(), raw.length(), hash);
        std::stringstream ss;
        for (int i = 0; i < 32; i++) ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        return "FE-TOKEN-" + ss.str().substr(0, 32);
    }

public:
    bool RegisterUser(const std::string& username, const std::string& password, const std::string& email) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::string userId = "FE-" + std::to_string(m_Users.size() + 1);
        std::string salt = GenerateSalt();
        FEUserAccount account;
        account.userId = userId;
        account.username = username;
        account.email = email;
        account.passwordHash = HashPassword(password, salt);
        account.salt = salt;
        m_Users[userId] = account;
        return true;
    }

    FEAuthToken* Login(const std::string& username, const std::string& password, const std::string& deviceId) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& [id, user] : m_Users) {
            if (user.username == username) {
                if (HashPassword(password, user.salt) == user.passwordHash) {
                    FEAuthToken token;
                    token.token = GenerateToken(id);
                    token.userId = id;
                    token.deviceId = deviceId;
                    m_ActiveTokens.push_back(token);
                    return &m_ActiveTokens.back();
                }
            }
        }
        return nullptr;
    }

    void SetAppSecret(const std::string& s) { m_AppSecret = s; }
};

} // namespace NeoEngine
