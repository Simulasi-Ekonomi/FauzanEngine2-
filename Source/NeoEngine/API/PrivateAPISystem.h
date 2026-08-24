#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <android/log.h>
#include "../Systems/ServerAuthorityValidator.h"

#define LOG_TAG "PrivateAPI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

// ================================================================
// PROTOKOL API PRIVATE FAUZANENGINE (TIDAK DIDOKUMENTASIKAN PUBLIK)
// ================================================================

enum class APIEndpoint {
    AUTHENTICATE,
    VERIFY_SERIAL,
    REPORT_CHEAT,
    SYNC_MARKETPLACE,
    VALIDATE_TRANSACTION,
    BACKUP_DATA,
    RESTORE_DATA,
    AI_QUERY,
    AI_TRAIN,
    LIVE_OPS_SYNC,
    TELEMETRY_PUSH,
    HEARTBEAT
};

struct APIRequest {
    APIEndpoint endpoint;
    std::string payload;
    std::string sessionToken;
    std::string hmacSignature;
    uint64_t timestamp;
    uint64_t nonce;
    std::string clientId;
    std::string clientVersion;
};

struct APIResponse {
    bool success = false;
    int statusCode = 0;
    std::string data;
    std::string errorMessage;
    std::string serverSignature;
    uint64_t serverTimestamp;
    int rateLimitRemaining = 100;
    int rateLimitReset = 60;
};

struct APISession {
    std::string sessionToken;
    std::string clientId;
    std::string sharedSecret;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
    int requestCount = 0;
    int maxRequests = 1000;
    bool active = true;
    std::vector<std::string> allowedEndpoints;
};

struct APIRateLimit {
    std::string clientId;
    int requestsThisWindow = 0;
    int maxRequests = 100;
    std::chrono::system_clock::time_point windowStart;
    int windowSeconds = 60;
    bool blocked = false;
    std::chrono::system_clock::time_point blockedUntil;
};

class PrivateAPISystem {
private:
    // =========================================================
    // SESSION MANAGEMENT
    // =========================================================
    std::unordered_map<std::string, APISession> m_Sessions;
    std::unordered_map<std::string, APIRateLimit> m_RateLimits;
    std::vector<std::string> m_WhitelistedIPs;
    std::vector<std::string> m_BlacklistedIPs;
    
    // =========================================================
    // SECURITY KEYS (ROTASI SETIAP 24 JAM)
    // =========================================================
    std::string m_MasterSecretKey;
    std::string m_DailyKey;
    std::chrono::system_clock::time_point m_LastKeyRotation;
    static constexpr int KEY_ROTATION_HOURS = 24;
    
    // =========================================================
    // INTERNAL
    // =========================================================
    std::mt19937_64 m_RNG;
    mutable std::mutex m_Mutex;
    int m_TotalRequests = 0;
    int m_BlockedRequests = 0;
    std::string m_ServerBaseURL = "https://api.fauzanengine.com/v2";
    
    // =========================================================
    // CALLBACKS
    // =========================================================
    std::function<void(const APIRequest&)> m_OnRequest;
    std::function<void(const APIResponse&)> m_OnResponse;
    std::function<void(const std::string&)> m_OnSecurityBreach;
    
    ServerAuthorityValidator* m_ServerValidator = nullptr;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    PrivateAPISystem() {
        m_RNG.seed(std::chrono::system_clock::now().time_since_epoch().count());
        RotateKeys();
    }
    
    // ================================================================
    // AUTENTIKASI SESSION (HANDSHAKE CUSTOM)
    // ================================================================
    std::string AuthenticateClient(const std::string& clientId, const std::string& clientSecret,
                                   const std::string& clientVersion, const std::string& clientIP) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        // Cek IP whitelist/blacklist
        if (IsIPBlacklisted(clientIP)) {
            LOGE("Authentication rejected: Blacklisted IP %s", clientIP.c_str());
            if (m_OnSecurityBreach) m_OnSecurityBreach("Blacklisted IP attempt: " + clientIP);
            return "";
        }
        
        // Verifikasi client secret dengan HMAC-SHA256
        std::string expectedHash = GenerateHMAC(clientId + clientVersion, m_MasterSecretKey);
        if (expectedHash != clientSecret) {
            LOGE("Authentication failed: Invalid secret for client %s", clientId.c_str());
            if (m_OnSecurityBreach) m_OnSecurityBreach("Invalid auth attempt: " + clientId);
            return "";
        }
        
        // Generate session token unik
        std::string sessionToken = GenerateSessionToken(clientId);
        std::string sharedSecret = GenerateSharedSecret(sessionToken);
        
        // Buat session
        APISession session;
        session.sessionToken = sessionToken;
        session.clientId = clientId;
        session.sharedSecret = sharedSecret;
        session.createdAt = std::chrono::system_clock::now();
        session.expiresAt = session.createdAt + std::chrono::hours(24);
        session.allowedEndpoints = GetAllowedEndpoints(clientId);
        
        m_Sessions[sessionToken] = session;
        
        LOGI("Client authenticated: %s (version: %s)", clientId.c_str(), clientVersion.c_str());
        return sessionToken;
    }
    
    // ================================================================
    // EXECUTE API REQUEST (DENGAN HMAC VERIFICATION)
    // ================================================================
    APIResponse ExecuteRequest(const APIRequest& request, const std::string& clientIP) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_TotalRequests++;
        
        APIResponse response;
        
        // 1. Cek rate limit
        if (!CheckRateLimit(request.clientId, clientIP)) {
            response.success = false;
            response.statusCode = 429;
            response.errorMessage = "Rate limit exceeded";
            m_BlockedRequests++;
            return response;
        }
        
        // 2. Verifikasi session
        auto sessionIt = m_Sessions.find(request.sessionToken);
        if (sessionIt == m_Sessions.end() || !sessionIt->second.active) {
            response.success = false;
            response.statusCode = 401;
            response.errorMessage = "Invalid or expired session";
            if (m_OnSecurityBreach) m_OnSecurityBreach("Invalid session: " + request.clientId);
            return response;
        }
        
        auto& session = sessionIt->second;
        
        // 3. Cek session expiry
        if (std::chrono::system_clock::now() > session.expiresAt) {
            session.active = false;
            response.success = false;
            response.statusCode = 401;
            response.errorMessage = "Session expired";
            return response;
        }
        
        // 4. Verifikasi HMAC signature
        std::string expectedSignature = GenerateRequestSignature(request, session.sharedSecret);
        if (expectedSignature != request.hmacSignature) {
            LOGE("HMAC verification failed for client %s", request.clientId.c_str());
            response.success = false;
            response.statusCode = 403;
            response.errorMessage = "Invalid signature";
            if (m_OnSecurityBreach) m_OnSecurityBreach("HMAC mismatch: " + request.clientId);
            return response;
        }
        
        // 5. Cek nonce (anti-replay attack)
        // (nonce tracking bisa ditambahkan di sini)
        
        // 6. Cek endpoint permission
        bool endpointAllowed = false;
        for (auto& ep : session.allowedEndpoints) {
            if (ep == EndpointToString(request.endpoint)) {
                endpointAllowed = true;
                break;
            }
        }
        if (!endpointAllowed) {
            response.success = false;
            response.statusCode = 403;
            response.errorMessage = "Endpoint not allowed for this client";
            return response;
        }
        
        // 7. Proses request ke server
        response = SendToServer(request, session);
        
        // 8. Update session counter
        session.requestCount++;
        if (session.requestCount >= session.maxRequests) {
            session.active = false;
        }
        
        // 9. Tambahkan server signature ke response
        response.serverSignature = GenerateHMAC(response.data, m_DailyKey);
        response.serverTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (m_OnResponse) m_OnResponse(response);
        return response;
    }
    
    // ================================================================
    // KIRIM REQUEST KE SERVER (ENKRIPSI CUSTOM)
    // ================================================================
    APIResponse SendToServer(const APIRequest& request, const APISession& session) {
        APIResponse response;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            response.success = false;
            response.errorMessage = "Internal error";
            return response;
        }
        
        std::string url = m_ServerBaseURL + "/" + EndpointToString(request.endpoint);
        
        // Enkripsi payload dengan shared secret
        std::string encryptedPayload = XOREncrypt(request.payload, session.sharedSecret);
        
        // Buat body dengan format custom (binary + base64)
        std::string body = "FAUZANENGINE_API_V2\n";
        body += "Session: " + request.sessionToken + "\n";
        body += "Client: " + request.clientId + "\n";
        body += "Timestamp: " + std::to_string(request.timestamp) + "\n";
        body += "Nonce: " + std::to_string(request.nonce) + "\n";
        body += "HMAC: " + request.hmacSignature + "\n";
        body += "Payload: " + encryptedPayload + "\n";
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/x-fauzanengine-api");
        headers = curl_slist_append(headers, ("X-FE-Version: " + request.clientVersion).c_str());
        headers = curl_slist_append(headers, "X-FE-Protocol: v2.0");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        std::string responseStr;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            response.statusCode = (int)httpCode;
            response.success = (httpCode >= 200 && httpCode < 300);
            response.data = XORDecrypt(responseStr, session.sharedSecret);
        } else {
            response.success = false;
            response.statusCode = 500;
            response.errorMessage = "Server unreachable";
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return response;
    }
    
    // ================================================================
    // ENKRIPSI / DEKRIPSI CUSTOM (XOR + Key Stretching)
    // ================================================================
    std::string XOREncrypt(const std::string& data, const std::string& key) {
        std::string result = data;
        std::string stretchedKey = key;
        while (stretchedKey.length() < data.length()) {
            stretchedKey += GenerateHMAC(stretchedKey, m_DailyKey);
        }
        for (size_t i = 0; i < data.length(); i++) {
            result[i] = data[i] ^ stretchedKey[i % stretchedKey.length()];
        }
        return Base64Encode(result);
    }
    
    std::string XORDecrypt(const std::string& data, const std::string& key) {
        std::string decoded = Base64Decode(data);
        std::string result = decoded;
        std::string stretchedKey = key;
        while (stretchedKey.length() < decoded.length()) {
            stretchedKey += GenerateHMAC(stretchedKey, m_DailyKey);
        }
        for (size_t i = 0; i < decoded.length(); i++) {
            result[i] = decoded[i] ^ stretchedKey[i % stretchedKey.length()];
        }
        return result;
    }
    
    // ================================================================
    // SECURITY FUNCTIONS
    // ================================================================
    std::string GenerateHMAC(const std::string& data, const std::string& key) {
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen = 0;
        HMAC(EVP_sha256(), key.c_str(), key.length(),
             (unsigned char*)data.c_str(), data.length(), result, &resultLen);
        
        std::stringstream ss;
        for (unsigned int i = 0; i < resultLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
        }
        return ss.str();
    }
    
    std::string GenerateSessionToken(const std::string& clientId) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::string raw = clientId + std::to_string(ms) + std::to_string(m_RNG());
        return "FE-SESS-" + GenerateHMAC(raw, m_MasterSecretKey).substr(0, 32);
    }
    
    std::string GenerateSharedSecret(const std::string& sessionToken) {
        return GenerateHMAC(sessionToken + m_DailyKey, m_MasterSecretKey).substr(0, 16);
    }
    
    std::string GenerateRequestSignature(const APIRequest& request, const std::string& sharedSecret) {
        std::string raw = EndpointToString(request.endpoint) + request.payload + 
                         std::to_string(request.timestamp) + std::to_string(request.nonce) +
                         request.clientId + sharedSecret;
        return GenerateHMAC(raw, sharedSecret);
    }
    
    void RotateKeys() {
        auto now = std::chrono::system_clock::now();
        if (m_LastKeyRotation.time_since_epoch().count() == 0 ||
            std::chrono::duration_cast<std::chrono::hours>(now - m_LastKeyRotation).count() >= KEY_ROTATION_HOURS) {
            m_DailyKey = GenerateHMAC(std::to_string(m_RNG()) + std::to_string(now.time_since_epoch().count()), m_MasterSecretKey);
            m_LastKeyRotation = now;
            LOGI("Security keys rotated");
        }
    }
    
    // ================================================================
    // RATE LIMITING
    // ================================================================
    bool CheckRateLimit(const std::string& clientId, const std::string& clientIP) {
        auto now = std::chrono::system_clock::now();
        auto& rl = m_RateLimits[clientId + "_" + clientIP];
        
        if (rl.blocked) {
            if (now < rl.blockedUntil) return false;
            rl.blocked = false;
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - rl.windowStart).count();
        if (elapsed >= rl.windowSeconds) {
            rl.windowStart = now;
            rl.requestsThisWindow = 0;
        }
        
        rl.requestsThisWindow++;
        if (rl.requestsThisWindow > rl.maxRequests) {
            rl.blocked = true;
            rl.blockedUntil = now + std::chrono::seconds(rl.windowSeconds * 2);
            LOGE("Rate limit exceeded for %s", clientId.c_str());
            return false;
        }
        return true;
    }
    
    // ================================================================
    // IP MANAGEMENT
    // ================================================================
    void WhitelistIP(const std::string& ip) { m_WhitelistedIPs.push_back(ip); }
    void BlacklistIP(const std::string& ip) { m_BlacklistedIPs.push_back(ip); }
    
    bool IsIPBlacklisted(const std::string& ip) const {
        for (auto& bl : m_BlacklistedIPs) if (bl == ip) return true;
        return false;
    }
    
    // ================================================================
    // HELPERS
    // ================================================================
    std::string EndpointToString(APIEndpoint ep) const {
        switch (ep) {
            case APIEndpoint::AUTHENTICATE: return "auth";
            case APIEndpoint::VERIFY_SERIAL: return "verify-serial";
            case APIEndpoint::REPORT_CHEAT: return "report-cheat";
            case APIEndpoint::SYNC_MARKETPLACE: return "sync-market";
            case APIEndpoint::VALIDATE_TRANSACTION: return "validate-tx";
            case APIEndpoint::BACKUP_DATA: return "backup";
            case APIEndpoint::RESTORE_DATA: return "restore";
            case APIEndpoint::AI_QUERY: return "ai-query";
            case APIEndpoint::AI_TRAIN: return "ai-train";
            case APIEndpoint::LIVE_OPS_SYNC: return "liveops-sync";
            case APIEndpoint::TELEMETRY_PUSH: return "telemetry";
            case APIEndpoint::HEARTBEAT: return "heartbeat";
        }
        return "unknown";
    }
    
    std::vector<std::string> GetAllowedEndpoints(const std::string& clientId) {
        std::vector<std::string> all;
        all.push_back("auth"); all.push_back("heartbeat"); all.push_back("telemetry");
        if (clientId.find("game_server") != std::string::npos || clientId.find("admin") != std::string::npos) {
            all.push_back("verify-serial"); all.push_back("report-cheat");
            all.push_back("sync-market"); all.push_back("validate-tx");
            all.push_back("backup"); all.push_back("restore");
            all.push_back("ai-query"); all.push_back("ai-train");
            all.push_back("liveops-sync");
        }
        return all;
    }
    
    std::string Base64Encode(const std::string& data) {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int val = 0, valb = -6;
        for (unsigned char c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (result.size() % 4) result.push_back('=');
        return result;
    }
    
    std::string Base64Decode(const std::string& data) {
        static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int val = 0, valb = -8;
        for (unsigned char c : data) {
            if (c == '=') break;
            size_t pos = chars.find(c);
            if (pos == std::string::npos) continue;
            val = (val << 6) + pos;
            valb += 6;
            if (valb >= 0) {
                result.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return result;
    }
    
    // ================================================================
    // GETTERS & SETTERS
    // ================================================================
    int GetTotalRequests() const { return m_TotalRequests; }
    int GetBlockedRequests() const { return m_BlockedRequests; }
    int GetActiveSessions() const { 
        int count = 0;
        for (auto& [k, v] : m_Sessions) if (v.active) count++;
        return count;
    }
    
    void SetMasterSecretKey(const std::string& key) { m_MasterSecretKey = key; RotateKeys(); }
    void SetServerBaseURL(const std::string& url) { m_ServerBaseURL = url; }
    void SetServerValidator(ServerAuthorityValidator* sv) { m_ServerValidator = sv; }
    ServerAuthorityValidator* GetServerValidator() { return m_ServerValidator; }
    void SetOnRequest(std::function<void(const APIRequest&)> cb) { m_OnRequest = cb; }
    void SetOnResponse(std::function<void(const APIResponse&)> cb) { m_OnResponse = cb; }
    void SetOnSecurityBreach(std::function<void(const std::string&)> cb) { m_OnSecurityBreach = cb; }
};

} // namespace NeoEngine
