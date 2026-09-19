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
#include <cstdio>
#include <cstdint>
#include <curl/curl.h>
#include <json/json.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOG_TAG "ItemSerialTracker"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOG_TAG "ItemSerialTracker"
#define LOGI(...) std::fprintf(stderr, "[INFO][%s] ", LOG_TAG), std::fprintf(stderr, __VA_ARGS__), std::fputc('\n', stderr)
#define LOGE(...) std::fprintf(stderr, "[ERROR][%s] ", LOG_TAG), std::fprintf(stderr, __VA_ARGS__), std::fputc('\n', stderr)
#endif

namespace NeoEngine {

struct OwnershipRecord {
    std::string playerId;
    std::string playerName;
    std::string transferMethod;
    std::string previousOwner;
    std::chrono::system_clock::time_point timestamp;
    std::string transferHash;
};

struct SerialNumber {
    std::string number;
    std::string itemType;
    std::string itemName;
    int quantity = 1;
    std::string source;
    std::string ownerId;
    std::string ownerName;
    std::string originalOwnerId;
    std::vector<OwnershipRecord> ownershipChain;
    std::chrono::system_clock::time_point timestamp;
    bool verified = false;
    bool consumed = false;
    bool contaminated = false;
    std::string serverSignature;
};

class ItemSerialTracker {
public:
    ItemSerialTracker();

    std::string GenerateSerial(const std::string& itemType, const std::string& itemName);
    SerialNumber* RegisterItem(const std::string& ownerId, const std::string& itemType,
                               const std::string& itemName, int quantity, const std::string& source);
    bool VerifyItemSilently(const std::string& internalItemId, const std::string& playerId);
    bool VerifyWithServer(const std::string& serialNumber);
    bool IsItemValid(const std::string& serialNumber) const;
    bool ConsumeItem(const std::string& serialNumber);
    int VerifyPendingBatch(int maxBatch = 10);

    std::string GenerateCurrencySerial(const std::string& currencyType, int amount, const std::string& source);
    std::string GenerateRewardSerial(const std::string& rewardName, const std::string& source);
    bool IsSerialDuplicate(const std::string& serialNumber) const;
    std::string GetAuditTrail(const std::string& playerId) const;
    std::string GetOwnershipChain(const std::string& serialNumber) const;
    bool VerifyOwnershipIntegrity(const std::string& serialNumber) const;

    int GetVerifiedCount() const { return m_VerifiedCount; }
    int GetRejectedCount() const { return m_RejectedCount; }
    int GetTotalItems() const { return m_TotalItems; }

    void SetServerURL(const std::string& url) { std::lock_guard<std::mutex> lock(m_Mutex); m_ServerURL = url; }
    void SetServerPublicKey(const std::string& key) { std::lock_guard<std::mutex> lock(m_Mutex); m_ServerPublicKey = key; }
    void SetOnCheatDetected(std::function<void(const std::string&, const std::string&)> cb) { std::lock_guard<std::mutex> lock(m_Mutex); m_OnCheatDetected = std::move(cb); }
    void SetOnVerified(std::function<void(const std::string&)> cb) { std::lock_guard<std::mutex> lock(m_Mutex); m_OnVerified = std::move(cb); }

    void MarkAllPlayerItemsContaminated(const std::string& playerId, const std::string& playerName);
    bool TransferOwnership(const std::string& serial, const std::string& fromId, const std::string& fromName,
                           const std::string& toId, const std::string& toName, const std::string& method);

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
    std::function<void(const std::string&, const std::string&)> m_OnCheatDetected;
    std::function<void(const std::string&)> m_OnVerified;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);
    std::string GenerateTransferHash(const std::string& serial, const std::string& from,
                                      const std::string& to, const std::string& method,
                                      std::size_t sequence) const;
};

} // namespace NeoEngine
