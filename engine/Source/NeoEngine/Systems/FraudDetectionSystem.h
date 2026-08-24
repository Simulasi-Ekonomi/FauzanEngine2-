#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <cmath>

namespace NeoEngine {

struct TransactionRecord {
    std::string transactionId;
    std::string buyerId, sellerId;
    std::string itemSerial;
    int amount = 0;
    std::chrono::system_clock::time_point timestamp;
    bool flagged = false;
    std::string flagReason;
};

struct FraudPattern {
    std::string patternName;
    int rapidTradeThreshold = 5;       // >5 transaksi dalam 1 menit
    int priceAnomalyMultiplier = 10;   // Harga 10x di atas rata-rata
    int duplicateIPThreshold = 3;      // >3 akun dari IP yang sama
    float tradebackWindowHours = 1.0f; // Trade-back dalam 1 jam
};

class FraudDetectionSystem {
private:
    std::vector<TransactionRecord> m_Transactions;
    std::unordered_map<std::string, std::vector<std::string>> m_IPAccounts;
    std::unordered_map<std::string, int> m_RecentTradeCount;
    std::unordered_map<std::string, float> m_ItemAvgPrices;
    FraudPattern m_Pattern;
    std::function<void(const TransactionRecord&)> m_OnFraudDetected;
    std::function<void(const std::string&)> m_OnSuspiciousPlayer;
    
    float m_CleanupTimer = 0;
    const float m_CleanupInterval = 60.0f;

public:
    bool ValidateTransaction(const std::string& buyerId, const std::string& sellerId,
                            const std::string& itemSerial, int amount,
                            const std::string& buyerIP = "", const std::string& sellerIP = "") {
        
        bool suspicious = false;
        std::string reason;
        
        // 1. Cek rapid trading (bot detection)
        m_RecentTradeCount[buyerId]++;
        m_RecentTradeCount[sellerId]++;
        if (m_RecentTradeCount[buyerId] > m_Pattern.rapidTradeThreshold) {
            suspicious = true;
            reason = "Rapid trading detected (possible bot)";
        }
        
        // 2. Cek price anomaly
        auto& avgPrice = m_ItemAvgPrices[itemSerial];
        if (avgPrice > 0 && amount > avgPrice * m_Pattern.priceAnomalyMultiplier) {
            suspicious = true;
            reason = "Price anomaly: " + std::to_string(amount) + " vs avg " + std::to_string((int)avgPrice);
        }
        // Update average price
        if (avgPrice == 0) avgPrice = amount;
        else avgPrice = avgPrice * 0.9f + amount * 0.1f;
        
        // 3. Cek trade-back (item kembali ke pemilik asli dalam waktu singkat)
        for (auto& t : m_Transactions) {
            if (t.itemSerial == itemSerial && t.buyerId == sellerId && t.sellerId == buyerId) {
                auto diff = std::chrono::duration_cast<std::chrono::minutes>(
                    std::chrono::system_clock::now() - t.timestamp).count();
                if (diff < m_Pattern.tradebackWindowHours * 60) {
                    suspicious = true;
                    reason = "Trade-back detected (possible RMT or laundering)";
                }
            }
        }
        
        // Catat transaksi
        TransactionRecord record{"tx_" + std::to_string(m_Transactions.size()), 
                                buyerId, sellerId, itemSerial, amount,
                                std::chrono::system_clock::now(), suspicious, reason};
        m_Transactions.push_back(record);
        
        if (suspicious) {
            if (m_OnFraudDetected) m_OnFraudDetected(record);
            if (m_OnSuspiciousPlayer) { m_OnSuspiciousPlayer(buyerId); m_OnSuspiciousPlayer(sellerId); }
        }
        
        return !suspicious; // return false jika transaksi mencurigakan
    }
    
    void Update(float dt) {
        m_CleanupTimer += dt;
        if (m_CleanupTimer >= m_CleanupInterval) {
            m_CleanupTimer = 0;
            // Reset counter transaksi cepat setiap menit
            for (auto& [id, count] : m_RecentTradeCount) count = 0;
        }
    }
    
    void SetPattern(const FraudPattern& p) { m_Pattern = p; }
    void SetOnFraudDetected(std::function<void(const TransactionRecord&)> cb) { m_OnFraudDetected = cb; }
    void SetOnSuspiciousPlayer(std::function<void(const std::string&)> cb) { m_OnSuspiciousPlayer = cb; }
};

} // namespace NeoEngine
