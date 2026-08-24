#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <functional>
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>

#define LOG_TAG "ItemSerialTracker"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

struct OwnershipRecord {
    std::string playerId;
    std::string playerName;
    std::string transferMethod; // "drop", "trade", "gift", "mail", "auction", "shop", "reward", "admin"
    std::string previousOwner;
    std::chrono::system_clock::time_point timestamp;
    std::string transferHash; // hash verifikasi transfer
};

struct SerialNumber {
    std::string number;
    std::string itemType;
    std::string itemName;
    int quantity = 1;
    std::string source;
    std::string currentOwnerId;
    std::string currentOwnerName;
    std::string originalOwnerId;
    std::vector<OwnershipRecord> ownershipChain;
    std::chrono::system_clock::time_point timestamp;
    bool verified = false;
    bool consumed = false;
    bool hidden = true; // SERIAL TERSEMBUNYI - tidak pernah ditampilkan ke pemain
    std::string serverSignature;
    std::string itemHash; // hash unik item untuk verifikasi internal
};

class ItemSerialTracker {
private:
    std::unordered_map<std::string, SerialNumber> m_Registry;
    std::vector<std::string> m_PendingVerification;
    std::string m_ServerURL = "https://api.fauzanengine.com/verify-item";
    std::string m_ServerPublicKey;
    mutable std::mutex m_Mutex;
    std::mt19937_64 m_RNG;
    int m_VerifiedCount = 0;
    int m_RejectedCount = 0;
    int m_TotalItems = 0;
    int m_TotalTransfers = 0;
    std::function<void(const std::string&, const std::string&)> m_OnCheatDetected;
    std::function<void(const std::string&)> m_OnVerified;
    std::function<void(const std::string&, const std::string&, const std::string&)> m_OnTransfer;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    std::string GenerateTransferHash(const std::string& serial, const std::string& from, 
                                      const std::string& to, const std::string& method) {
        std::string raw = serial + from + to + method + std::to_string(m_TotalTransfers);
        return std::to_string(std::hash<std::string>{}(raw));
    }
    
    std::string GenerateItemHash(const std::string& serial, const std::string& type, 
                                  const std::string& name, int quantity) {
        std::string raw = serial + type + name + std::to_string(quantity) + 
                         std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        return std::to_string(std::hash<std::string>{}(raw));
    }
    
public:
    ItemSerialTracker() {
        m_RNG.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    std::string GenerateSerial(const std::string& itemType, const std::string& itemName) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_TotalItems++;
        
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::string typeCode;
        if (itemType == "weapon") typeCode = "WP";
        else if (itemType == "armor") typeCode = "AR";
        else if (itemType == "potion") typeCode = "PT";
        else if (itemType == "material") typeCode = "MT";
        else if (itemType == "currency") typeCode = "CR";
        else if (itemType == "gem") typeCode = "GM";
        else if (itemType == "skin") typeCode = "SK";
        else if (itemType == "key") typeCode = "KY";
        else if (itemType == "chest") typeCode = "CH";
        else if (itemType == "reward") typeCode = "RW";
        else typeCode = "XX";
        
        uint64_t randomPart = m_RNG() % 99999;
        uint64_t checksum = (ms + randomPart + itemName.length()) % 9999;
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "FE-%s-%04llX-%05llu-%04llu", 
                 typeCode.c_str(), 
                 (unsigned long long)(ms % 65535),
                 (unsigned long long)randomPart,
                 (unsigned long long)checksum);
        
        return std::string(buffer);
    }
    
    SerialNumber* RegisterItem(const std::string& ownerId, const std::string& ownerName,
                               const std::string& itemType, const std::string& itemName, 
                               int quantity, const std::string& source) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        std::string serial = GenerateSerial(itemType, itemName);
        SerialNumber sn;
        sn.number = serial;
        sn.itemType = itemType;
        sn.itemName = itemName;
        sn.quantity = quantity;
        sn.source = source;
        sn.currentOwnerId = ownerId;
        sn.currentOwnerName = ownerName;
        sn.originalOwnerId = ownerId;
        sn.timestamp = std::chrono::system_clock::now();
        sn.verified = false;
        sn.consumed = false;
        sn.hidden = true;
        sn.itemHash = GenerateItemHash(serial, itemType, itemName, quantity);
        
        // Catat kepemilikan awal
        OwnershipRecord initialRecord;
        initialRecord.playerId = ownerId;
        initialRecord.playerName = ownerName;
        initialRecord.transferMethod = source;
        initialRecord.previousOwner = "SYSTEM";
        initialRecord.timestamp = sn.timestamp;
        initialRecord.transferHash = GenerateTransferHash(serial, "SYSTEM", ownerId, source);
        sn.ownershipChain.push_back(initialRecord);
        
        m_Registry[sn.number] = sn;
        m_PendingVerification.push_back(sn.number);
        
        LOGI("Registered item: %s (Serial: HIDDEN) owned by %s", itemName.c_str(), ownerName.c_str());
        return &m_Registry[sn.number];
    }
    
    // TRANSFER KEPEMILIKAN - mencatat rantai A → B → C → D
    bool TransferOwnership(const std::string& serialNumber, 
                           const std::string& fromPlayerId, const std::string& fromPlayerName,
                           const std::string& toPlayerId, const std::string& toPlayerName,
                           const std::string& transferMethod) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        auto it = m_Registry.find(serialNumber);
        if (it == m_Registry.end()) {
            LOGE("Transfer failed: Serial %s not found", serialNumber.c_str());
            return false;
        }
        
        auto& sn = it->second;
        
        // Verifikasi pemilik saat ini
        if (sn.currentOwnerId != fromPlayerId) {
            LOGE("CHEAT: Transfer from wrong owner! Serial: %s, Expected: %s, Got: %s",
                 serialNumber.c_str(), sn.currentOwnerId.c_str(), fromPlayerId.c_str());
            if (m_OnCheatDetected) m_OnCheatDetected(fromPlayerId, "Unauthorized transfer attempt");
            return false;
        }
        
        if (sn.consumed) {
            LOGE("CHEAT: Transfer of consumed item! Serial: %s", serialNumber.c_str());
            return false;
        }
        
        // Buat record transfer baru
        OwnershipRecord record;
        record.playerId = toPlayerId;
        record.playerName = toPlayerName;
        record.transferMethod = transferMethod;
        record.previousOwner = fromPlayerId + " (" + fromPlayerName + ")";
        record.timestamp = std::chrono::system_clock::now();
        record.transferHash = GenerateTransferHash(serialNumber, fromPlayerId, toPlayerId, transferMethod);
        
        sn.ownershipChain.push_back(record);
        sn.currentOwnerId = toPlayerId;
        sn.currentOwnerName = toPlayerName;
        m_TotalTransfers++;
        
        LOGI("Ownership transferred: %s | %s → %s | Method: %s", 
             itemName.c_str(), fromPlayerName.c_str(), toPlayerName.c_str(), transferMethod.c_str());
        
        if (m_OnTransfer) m_OnTransfer(serialNumber, fromPlayerId, toPlayerId);
        
        // Kirim ke server untuk audit
        SyncTransferToServer(serialNumber, record);
        
        return true;
    }
    
    void SyncTransferToServer(const std::string& serial, const OwnershipRecord& record) {
        CURL* curl = curl_easy_init();
        if (!curl) return;
        
        Json::Value body;
        body["serial"] = serial;
        body["fromPlayer"] = record.previousOwner;
        body["toPlayer"] = record.playerName;
        body["toPlayerId"] = record.playerId;
        body["method"] = record.transferMethod;
        body["transferHash"] = record.transferHash;
        body["appId"] = "FauzanEngine-V2";
        
        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);
        
        curl_easy_setopt(curl, CURLOPT_URL, (m_ServerURL + "/transfer").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    // Dapatkan rantai kepemilikan lengkap
    std::string GetOwnershipChain(const std::string& serialNumber) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(serialNumber);
        if (it == m_Registry.end()) return "Serial not found";
        
        std::string chain = "Ownership Chain for " + it->second.itemName + ":\n";
        for (size_t i = 0; i < it->second.ownershipChain.size(); i++) {
            auto& r = it->second.ownershipChain[i];
            chain += "  " + std::to_string(i+1) + ". " + r.playerName + " [" + r.playerId + "]";
            chain += " via " + r.transferMethod;
            if (i > 0) chain += " from " + it->second.ownershipChain[i-1].playerName;
            chain += "\n";
        }
        return chain;
    }
    
    // Verifikasi serial (hidden - pemain tidak pernah tahu nomor serinya)
    bool VerifyItemSilently(const std::string& internalItemId, const std::string& playerId) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        // Pemain tidak bisa mencari dengan serial, hanya engine yang bisa
        for (auto& [serial, sn] : m_Registry) {
            if (sn.itemHash == internalItemId) {
                if (sn.currentOwnerId != playerId) {
                    LOGE("CHEAT: Item owned by %s, not %s", sn.currentOwnerName.c_str(), playerId.c_str());
                    return false;
                }
                return sn.verified && !sn.consumed;
            }
        }
        return false;
    }
    
    // Cek integritas rantai kepemilikan
    bool VerifyOwnershipIntegrity(const std::string& serialNumber) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(serialNumber);
        if (it == m_Registry.end()) return false;
        
        auto& chain = it->second.ownershipChain;
        if (chain.empty()) return false;
        
        for (size_t i = 1; i < chain.size(); i++) {
            // Verifikasi hash transfer
            std::string expectedHash = GenerateTransferHash(
                serialNumber, 
                chain[i].previousOwner,
                chain[i].playerId,
                chain[i].transferMethod
            );
            if (expectedHash != chain[i].transferHash) {
                LOGE("Integrity FAILED: Transfer hash mismatch at step %zu for serial %s", i, serialNumber.c_str());
                return false;
            }
        }
        return true;
    }
    
    // Getter
    std::string GetCurrentOwner(const std::string& serialNumber) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(serialNumber);
        return it != m_Registry.end() ? it->second.currentOwnerName : "Unknown";
    }
    
    int GetOwnershipChainLength(const std::string& serialNumber) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(serialNumber);
        return it != m_Registry.end() ? it->second.ownershipChain.size() : 0;
    }
    
    int GetTotalTransfers() const { return m_TotalTransfers; }
    int GetVerifiedCount() const { return m_VerifiedCount; }
    int GetRejectedCount() const { return m_RejectedCount; }
    int GetTotalItems() const { return m_TotalItems; }
    
    // Setters
    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetServerPublicKey(const std::string& key) { m_ServerPublicKey = key; }
    void SetOnCheatDetected(std::function<void(const std::string&, const std::string&)> cb) { m_OnCheatDetected = cb; }
    void SetOnVerified(std::function<void(const std::string&)> cb) { m_OnVerified = cb; }
    void SetOnTransfer(std::function<void(const std::string&, const std::string&, const std::string&)> cb) { m_OnTransfer = cb; }
};

} // namespace NeoEngine
