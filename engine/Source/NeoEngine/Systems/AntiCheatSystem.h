#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <functional>
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>

#define LOG_TAG "AntiCheat"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

// Tingkat hukuman
enum class PunishmentLevel {
    None = 0,
    Warning = 1,         // Peringatan
    ItemDestroyed = 2,   // Item dimusnahkan, uang dikembalikan
    TempBan = 3,         // Ban sementara (1-30 hari)
    PermaBan = 4,        // Ban permanen
    IPBan = 5,           // Ban IP + Device ID
    LegalAction = 6      // Tindakan hukum
};

struct CheatRecord {
    std::string playerId;
    std::string playerName;
    std::string cheatType;       // "speed_hack", "item_duplication", "memory_injection", "packet_manipulation", "currency_hack"
    std::string itemSerial;      // Serial item yang dicurangi (jika ada)
    std::string evidence;        // Bukti cheat
    std::chrono::system_clock::time_point timestamp;
    PunishmentLevel punishment = PunishmentLevel::None;
    bool executed = false;
};

struct BannedPlayer {
    std::string playerId;
    std::string playerName;
    std::string reason;
    std::chrono::system_clock::time_point banTime;
    std::chrono::system_clock::time_point unbanTime; // zero = permanent
    PunishmentLevel level = PunishmentLevel::PermaBan;
    bool isPermanent = false;
    std::string deviceId;
    std::string ipAddress;
};

struct ContaminatedItem {
    std::string serialNumber;
    std::string itemName;
    std::string originalCheaterId;
    std::string originalCheaterName;
    std::vector<std::string> affectedOwners; // pemain tidak bersalah yang punya item ini
    bool destroyed = false;
};

class AntiCheatSystem {
private:
    std::vector<CheatRecord> m_CheatLogs;
    std::vector<BannedPlayer> m_BannedPlayers;
    std::vector<ContaminatedItem> m_ContaminatedItems;
    std::unordered_map<std::string, int> m_WarningCount;
    std::unordered_map<std::string, std::vector<std::string>> m_PlayerItems; // playerId -> serials
    std::string m_ServerURL = "https://api.fauzanengine.com/anti-cheat";
    std::string m_ServerKey;
    mutable std::mutex m_Mutex;
    int m_TotalBans = 0;

    // Callbacks
    std::function<void(const CheatRecord&)> m_OnCheatDetected;
    std::function<void(const BannedPlayer&)> m_OnPlayerBanned;
    std::function<void(const std::string&, const std::string&)> m_OnItemDestroyed; // playerId, serial
    std::function<void(const std::string&, int)> m_OnRefund; // playerId, amount

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    AntiCheatSystem() = default;

    // ================================================================
    // DETEKSI CHEAT & HUKUMAN
    // ================================================================

    // Player PERTAMA yang cheat → BAN PERMANEN
    void DetectAndPunish(const std::string& playerId, const std::string& playerName,
                         const std::string& cheatType, const std::string& itemSerial = "",
                         const std::string& evidence = "") {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // Catat di log
        CheatRecord record{playerId, playerName, cheatType, itemSerial, evidence,
                          std::chrono::system_clock::now(), PunishmentLevel::PermaBan, false};

        // Tentukan hukuman berdasarkan jenis cheat
        if (cheatType == "speed_hack" || cheatType == "wall_hack" || cheatType == "aimbot") {
            record.punishment = PunishmentLevel::PermaBan;
        } else if (cheatType == "item_duplication" || cheatType == "currency_hack") {
            record.punishment = PunishmentLevel::PermaBan;
            // Tandai semua item dari player ini sebagai terkontaminasi
            MarkAllPlayerItemsContaminated(playerId, playerName);
        } else if (cheatType == "packet_manipulation") {
            record.punishment = PunishmentLevel::PermaBan;
        } else if (cheatType == "memory_injection") {
            record.punishment = PunishmentLevel::IPBan;
        }

        // Eksekusi hukuman
        ExecutePunishment(record);
        m_CheatLogs.push_back(record);
        m_TotalBans++;

        // Kirim ke server pusat
        ReportToServer(record);

        if (m_OnCheatDetected) m_OnCheatDetected(record);
    }

    void ExecutePunishment(CheatRecord& record) {
        if (record.executed) return;

        BannedPlayer ban;
        ban.playerId = record.playerId;
        ban.playerName = record.playerName;
        ban.reason = record.cheatType + ": " + record.evidence;
        ban.banTime = std::chrono::system_clock::now();
        ban.level = record.punishment;

        switch (record.punishment) {
            case PunishmentLevel::PermaBan:
                ban.isPermanent = true;
                ban.unbanTime = std::chrono::system_clock::time_point::max();
                break;
            case PunishmentLevel::TempBan:
                ban.isPermanent = false;
                ban.unbanTime = ban.banTime + std::chrono::hours(24 * 7); // 7 hari
                break;
            case PunishmentLevel::IPBan:
                ban.isPermanent = true;
                ban.unbanTime = std::chrono::system_clock::time_point::max();
                break;
            default:
                break;
        }

        m_BannedPlayers.push_back(ban);
        record.executed = true;

        if (m_OnPlayerBanned) m_OnPlayerBanned(ban);

        LOGI("BAN: %s (%s) - %s - %s", 
             record.playerName.c_str(), record.playerId.c_str(), 
             record.cheatType.c_str(), 
             ban.isPermanent ? "PERMANENT" : "TEMPORARY");
    }

    // ================================================================
    // ITEM TERKONTAMINASI - Player tidak bersalah hanya kehilangan item
    // ================================================================

    void MarkAllPlayerItemsContaminated(const std::string& cheaterId, const std::string& cheaterName) {
        auto it = m_PlayerItems.find(cheaterId);
        if (it == m_PlayerItems.end()) return;

        for (auto& serial : it->second) {
            ContaminatedItem ci;
            ci.serialNumber = serial;
            ci.itemName = "Contaminated Item";
            ci.originalCheaterId = cheaterId;
            ci.originalCheaterName = cheaterName;
            m_ContaminatedItems.push_back(ci);
        }
    }

    // Cek apakah item berasal dari cheater (rantai kepemilikan)
    bool IsItemContaminated(const std::string& serialNumber) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& ci : m_ContaminatedItems) {
            if (ci.serialNumber == serialNumber && !ci.destroyed) return true;
        }
        return false;
    }

    // Musnahkan item terkontaminasi (player tidak bersalah)
    void DestroyContaminatedItem(const std::string& serialNumber, 
                                  const std::string& currentOwnerId,
                                  const std::string& currentOwnerName,
                                  int refundAmount = 0) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& ci : m_ContaminatedItems) {
            if (ci.serialNumber == serialNumber && !ci.destroyed) {
                ci.destroyed = true;
                ci.affectedOwners.push_back(currentOwnerName);

                // Kembalikan uang ke player tidak bersalah
                if (refundAmount > 0 && m_OnRefund) {
                    m_OnRefund(currentOwnerId, refundAmount);
                }

                if (m_OnItemDestroyed) {
                    m_OnItemDestroyed(currentOwnerId, serialNumber);
                }

                LOGI("Contaminated item %s destroyed from %s (refund: %d gold)", 
                     serialNumber.c_str(), currentOwnerName.c_str(), refundAmount);
                return;
            }
        }
    }

    // Bersihkan semua item terkontaminasi milik player tertentu
    void CleansePlayerInventory(const std::string& playerId, const std::string& playerName) {
        auto it = m_PlayerItems.find(playerId);
        if (it == m_PlayerItems.end()) return;

        std::vector<std::string> toDestroy;
        for (auto& serial : it->second) {
            if (IsItemContaminated(serial)) {
                toDestroy.push_back(serial);
            }
        }

        for (auto& serial : toDestroy) {
            DestroyContaminatedItem(serial, playerId, playerName, 50); // refund 50 gold per item
        }
    }

    // ================================================================
    // VERIFIKASI TRANSAKSI - Cek sebelum transfer
    // ================================================================

    bool ValidateTransaction(const std::string& sellerId, const std::string& buyerId,
                            const std::string& itemSerial) {
        // Cek apakah seller adalah cheater
        if (IsPlayerBanned(sellerId)) {
            LOGE("Transaction blocked: Seller %s is banned", sellerId.c_str());
            return false;
        }

        // Cek apakah item terkontaminasi
        if (IsItemContaminated(itemSerial)) {
            LOGE("Transaction blocked: Item %s is contaminated (cheat origin)", itemSerial.c_str());
            // Musnahkan item, tapi jangan ban buyer
            DestroyContaminatedItem(itemSerial, sellerId, "Seller", 0);
            return false;
        }

        return true;
    }

    // ================================================================
    // BAN CHECK
    // ================================================================

    bool IsPlayerBanned(const std::string& playerId) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto now = std::chrono::system_clock::now();
        for (auto& b : m_BannedPlayers) {
            if (b.playerId == playerId) {
                if (b.isPermanent || now < b.unbanTime) return true;
            }
        }
        return false;
    }

    bool IsPlayerPermaBanned(const std::string& playerId) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& b : m_BannedPlayers) {
            if (b.playerId == playerId && b.isPermanent) return true;
        }
        return false;
    }

    std::string GetBanReason(const std::string& playerId) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& b : m_BannedPlayers) {
            if (b.playerId == playerId) return b.reason;
        }
        return "";
    }

    // ================================================================
    // REGISTRASI ITEM KE PLAYER (untuk tracking)
    // ================================================================

    void RegisterPlayerItem(const std::string& playerId, const std::string& serial) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PlayerItems[playerId].push_back(serial);
    }

    void TransferPlayerItem(const std::string& fromId, const std::string& toId, 
                           const std::string& serial) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        // Hapus dari pemilik lama
        auto it = m_PlayerItems.find(fromId);
        if (it != m_PlayerItems.end()) {
            it->second.erase(std::remove(it->second.begin(), it->second.end(), serial), it->second.end());
        }
        // Tambah ke pemilik baru
        m_PlayerItems[toId].push_back(serial);
    }

    // ================================================================
    // REPORTING
    // ================================================================

    void ReportToServer(const CheatRecord& record) {
        CURL* curl = curl_easy_init();
        if (!curl) return;

        Json::Value body;
        body["playerId"] = record.playerId;
        body["playerName"] = record.playerName;
        body["cheatType"] = record.cheatType;
        body["evidence"] = record.evidence;
        body["punishment"] = (int)record.punishment;
        body["appId"] = "FauzanEngine-V2";

        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);

        curl_easy_setopt(curl, CURLOPT_URL, (m_ServerURL + "/report").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    std::string GetCheatStatistics() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::string stats = "AntiCheat Statistics:\n";
        stats += "Total bans: " + std::to_string(m_TotalBans) + "\n";
        stats += "Contaminated items: " + std::to_string(m_ContaminatedItems.size()) + "\n";
        stats += "Destroyed items: ";
        int destroyed = 0;
        for (auto& ci : m_ContaminatedItems) if (ci.destroyed) destroyed++;
        stats += std::to_string(destroyed) + "\n";
        return stats;
    }

    // ================================================================
    // SETTERS
    // ================================================================

    void SetServerURL(const std::string& url) { m_ServerURL = url; }
    void SetServerKey(const std::string& key) { m_ServerKey = key; }
    void SetOnCheatDetected(std::function<void(const CheatRecord&)> cb) { m_OnCheatDetected = cb; }
    void SetOnPlayerBanned(std::function<void(const BannedPlayer&)> cb) { m_OnPlayerBanned = cb; }
    void SetOnItemDestroyed(std::function<void(const std::string&, const std::string&)> cb) { m_OnItemDestroyed = cb; }
    void SetOnRefund(std::function<void(const std::string&, int)> cb) { m_OnRefund = cb; }
};

} // namespace NeoEngine
