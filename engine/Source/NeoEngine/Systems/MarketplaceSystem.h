#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <chrono>
#include <functional>
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>
#include <random>
#include <sstream>
#include <iomanip>
#include "ItemSerialTracker.h"

#define LOG_TAG "Marketplace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

enum class TradeCurrency { GOLD, GEMS, DIAMONDS, REAL_MONEY, BARTER };
enum class ListingStatus { ACTIVE, LOCKED, SOLD, CANCELLED, DISPUTED, REFUNDED };
enum class PaymentMethod { GOOGLE_PLAY, APP_STORE, CREDIT_CARD, BANK_TRANSFER, E_WALLET, CRYPTO };
enum class PaymentStatus { PENDING, PROCESSING, COMPLETED, FAILED, REFUNDED, CHARGEBACK };

struct TradeListing {
    std::string listingId;
    std::string sellerId, sellerName;
    std::string itemSerial;       // HIDDEN - tidak pernah ditampilkan
    std::string itemName;
    std::string itemType;
    TradeCurrency currency;
    float price = 0;
    std::string realCurrency = "USD";
    int quantity = 1;
    ListingStatus status = ListingStatus::ACTIVE;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
    bool escrowHeld = false;      // Item ditahan sebelum transaksi selesai
    std::string escrowId;
};

struct RealMoneyTransaction {
    std::string transactionId;
    std::string buyerId, buyerName;
    std::string sellerId, sellerName;
    std::string listingId;
    std::string itemSerial;       // HIDDEN
    float amount;
    std::string currency;
    PaymentMethod paymentMethod;
    PaymentStatus paymentStatus = PaymentStatus::PENDING;
    float devCommission = 0;      // 5% fee
    float sellerPayout = 0;       // amount - commission
    std::string paymentGatewayId;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point completedAt;
    bool itemDelivered = false;
    bool buyerConfirmed = false;
    int autoCompleteSeconds = 86400;  // 24 jam auto complete
};

struct TradeHistory {
    std::string tradeId;
    std::string playerA, playerB;
    std::string itemSerial;       // HIDDEN
    std::string itemName;
    std::string action;           // "sold", "bought", "traded", "cancelled"
    float amount = 0;
    TradeCurrency currency;
    std::chrono::system_clock::time_point timestamp;
};

struct PaymentReceipt {
    std::string receiptId;
    std::string transactionId;
    std::string playerId;
    float totalAmount;
    float devFee;
    float netAmount;
    std::string currency;
    std::chrono::system_clock::time_point issuedAt;
    std::string digitalSignature;
};

class MarketplaceSystem {
private:
    // =========================================================
    // CORE DATA
    // =========================================================
    std::vector<TradeListing> m_Listings;
    std::vector<RealMoneyTransaction> m_RMTransactions;
    std::vector<TradeHistory> m_TradeHistory;
    std::vector<PaymentReceipt> m_Receipts;
    std::queue<std::string> m_DisputeQueue;

    // =========================================================
    // INTEGRATION WITH OTHER SYSTEMS
    // =========================================================
    ItemSerialTracker* m_SerialTracker = nullptr;
    void* m_FraudDetection = nullptr;

    // =========================================================
    // CONFIGURATION
    // =========================================================
    static constexpr float DEV_COMMISSION = 0.05f;  // 5%
    static constexpr float MIN_REAL_PRICE = 0.99f;   // $0.99 minimum
    static constexpr float MAX_REAL_PRICE = 9999.99f;
    static constexpr int MAX_LISTINGS_PER_PLAYER = 50;
    static constexpr int LISTING_DURATION_DAYS = 7;
    static constexpr int AUTO_COMPLETE_HOURS = 24;
    static constexpr int DISPUTE_RESOLVE_DAYS = 7;

    // =========================================================
    // EXTERNAL SERVER ENDPOINTS (TERPISAH DARI GAME SERVER)
    // =========================================================
    std::string m_ItemVerifyServerURL = "https://security.fauzanengine.com/verify-serial";
    std::string m_PaymentGatewayURL = "https://payment.fauzanengine.com/process";
    std::string m_AntiCheatServerURL = "https://security.fauzanengine.com/anti-cheat";
    std::string m_MarketplaceServerURL = "https://marketplace.fauzanengine.com/api";
    std::string m_ServerAPIKey;

    // =========================================================
    // SYNC
    // =========================================================
    mutable std::mutex m_Mutex;
    float m_SyncTimer = 0;
    float m_SyncInterval = 60.0f;
    int m_TotalTransactions = 0;
    float m_TotalDevRevenue = 0;
    float m_TotalMarketVolume = 0;

    // =========================================================
    // CALLBACKS
    // =========================================================
    std::function<void(const TradeListing&)> m_OnListingCreated;
    std::function<void(const RealMoneyTransaction&)> m_OnPaymentComplete;
    std::function<void(const std::string&, const std::string&)> m_OnDispute;
    std::function<void(const PaymentReceipt&)> m_OnReceiptGenerated;
    std::function<void(const std::string&, float)> m_OnDevRevenue;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    void SetSerialTracker(ItemSerialTracker* tracker) { m_SerialTracker = tracker; }

    // ================================================================
    // CREATE LISTING (Jual Item)
    // ================================================================
    TradeListing* CreateListing(const std::string& sellerId, const std::string& sellerName,
                                const std::string& itemSerial, const std::string& itemName,
                                const std::string& itemType, TradeCurrency currency, float price,
                                int quantity = 1, const std::string& realCurrency = "USD") {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // Cek jumlah listing per player
        int playerListings = 0;
        for (auto& l : m_Listings) {
            if (l.sellerId == sellerId && l.status == ListingStatus::ACTIVE) playerListings++;
        }
        if (playerListings >= MAX_LISTINGS_PER_PLAYER) return nullptr;

        // Validasi item via Serial Tracker (HIDDEN - pemain tidak tahu)
        if (m_SerialTracker) {
            if (!m_SerialTracker->VerifyItemSilently(itemSerial, sellerId)) {
                LOGE("Listing rejected: Item verification failed - possible cheat!");
                return nullptr;
            }
        }

        // Validasi harga
        if (currency == TradeCurrency::REAL_MONEY) {
            if (price < MIN_REAL_PRICE || price > MAX_REAL_PRICE) return nullptr;
        }

        // Buat listing
        TradeListing listing;
        listing.listingId = GenerateListingID();
        listing.sellerId = sellerId;
        listing.sellerName = sellerName;
        listing.itemSerial = itemSerial;  // HIDDEN - disimpan hanya di sistem
        listing.itemName = itemName;
        listing.itemType = itemType;
        listing.currency = currency;
        listing.price = price;
        listing.realCurrency = realCurrency;
        listing.quantity = quantity;
        listing.status = ListingStatus::ACTIVE;
        listing.createdAt = std::chrono::system_clock::now();
        listing.expiresAt = listing.createdAt + std::chrono::hours(24 * LISTING_DURATION_DAYS);

        // TAHAN ITEM (Escrow)
        listing.escrowHeld = true;
        listing.escrowId = GenerateEscrowID();

        m_Listings.push_back(listing);

        LOGI("Listing created: %s by %s (Price: %.2f %s)", 
             itemName.c_str(), sellerName.c_str(), price, 
             currency == TradeCurrency::REAL_MONEY ? realCurrency.c_str() : "GOLD");

        if (m_OnListingCreated) m_OnListingCreated(m_Listings.back());
        return &m_Listings.back();
    }

    // ================================================================
    // BUY ITEM (Real Money Transaction)
    // ================================================================
    RealMoneyTransaction* BuyItemWithRealMoney(const std::string& buyerId, const std::string& buyerName,
                                                const std::string& listingId,
                                                PaymentMethod paymentMethod) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // Cari listing
        TradeListing* listing = nullptr;
        for (auto& l : m_Listings) {
            if (l.listingId == listingId && l.status == ListingStatus::ACTIVE) {
                listing = &l;
                break;
            }
        }
        if (!listing) {
            LOGE("Purchase failed: Listing %s not found or not active", listingId.c_str());
            return nullptr;
        }

        // Cek bukan beli item sendiri
        if (listing->sellerId == buyerId) {
            LOGE("Purchase failed: Cannot buy own item");
            return nullptr;
        }

        // Verifikasi item ke server terpisah (SECURITY CHECK)
        if (!VerifyItemToSecurityServer(listing->itemSerial, listing->sellerId, buyerId)) {
            LOGE("Purchase failed: Item verification rejected by security server");
            listing->status = ListingStatus::LOCKED;
            return nullptr;
        }

        // Cek fraud detection (jika tersedia)
        // (System akan diintegrasikan nanti)

        // Kunci listing
        listing->status = ListingStatus::LOCKED;

        // Hitung komisi
        float devCommission = listing->price * DEV_COMMISSION;
        float sellerPayout = listing->price - devCommission;

        // Buat transaksi real money
        RealMoneyTransaction transaction;
        transaction.transactionId = GenerateTransactionID();
        transaction.buyerId = buyerId;
        transaction.buyerName = buyerName;
        transaction.sellerId = listing->sellerId;
        transaction.sellerName = listing->sellerName;
        transaction.listingId = listingId;
        transaction.itemSerial = listing->itemSerial;  // HIDDEN
        transaction.amount = listing->price;
        transaction.currency = listing->realCurrency;
        transaction.paymentMethod = paymentMethod;
        transaction.paymentStatus = PaymentStatus::PENDING;
        transaction.devCommission = devCommission;
        transaction.sellerPayout = sellerPayout;
        transaction.createdAt = std::chrono::system_clock::now();

        // Proses pembayaran ke payment gateway
        std::string gatewayId = ProcessPaymentToGateway(transaction);
        transaction.paymentGatewayId = gatewayId;

        m_RMTransactions.push_back(transaction);
        m_TotalTransactions++;

        LOGI("Real Money Transaction: %s buys %s from %s ($%.2f, fee: $%.2f)", 
             buyerName.c_str(), listing->itemName.c_str(), listing->sellerName.c_str(),
             listing->price, devCommission);

        return &m_RMTransactions.back();
    }

    // ================================================================
    // CONFIRM PAYMENT & DELIVER ITEM
    // ================================================================
    bool ConfirmPaymentAndDeliver(const std::string& transactionId) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& tx : m_RMTransactions) {
            if (tx.transactionId == transactionId && tx.paymentStatus == PaymentStatus::PROCESSING) {
                tx.paymentStatus = PaymentStatus::COMPLETED;
                tx.completedAt = std::chrono::system_clock::now();

                // Transfer kepemilikan via Serial Tracker (HIDDEN)
                if (m_SerialTracker) {
                    m_SerialTracker->TransferOwnership(
                        tx.itemSerial,
                        tx.sellerId, tx.sellerName,
                        tx.buyerId, tx.buyerName,
                        "marketplace_rmt"
                    );
                }

                // Update listing
                for (auto& l : m_Listings) {
                    if (l.listingId == tx.listingId) {
                        l.status = ListingStatus::SOLD;
                        l.escrowHeld = false;
                    }
                }

                tx.itemDelivered = true;
                m_TotalDevRevenue += tx.devCommission;
                m_TotalMarketVolume += tx.amount;

                // Kirim receipt ke buyer
                GenerateReceipt(tx);

                // Notifikasi
                if (m_OnPaymentComplete) m_OnPaymentComplete(tx);
                if (m_OnDevRevenue) m_OnDevRevenue(tx.transactionId, tx.devCommission);

                LOGI("Transaction %s completed! Dev revenue: $%.2f", transactionId.c_str(), tx.devCommission);
                return true;
            }
        }
        return false;
    }

    // ================================================================
    // BUYER CONFIRMATION (Setelah item diterima)
    // ================================================================
    bool BuyerConfirmDelivery(const std::string& transactionId, const std::string& buyerId) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& tx : m_RMTransactions) {
            if (tx.transactionId == transactionId && tx.buyerId == buyerId) {
                tx.buyerConfirmed = true;
                LOGI("Buyer %s confirmed delivery for transaction %s", buyerId.c_str(), transactionId.c_str());
                return true;
            }
        }
        return false;
    }

    // ================================================================
    // DISPUTE SYSTEM (Perlindungan Konsumen)
    // ================================================================
    bool OpenDispute(const std::string& transactionId, const std::string& playerId,
                    const std::string& reason, const std::string& evidence) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& tx : m_RMTransactions) {
            if (tx.transactionId == transactionId) {
                m_DisputeQueue.push(transactionId);

                // Kirim ke server anti-cheat untuk investigasi
                ReportDisputeToServer(transactionId, playerId, reason, evidence);

                if (m_OnDispute) m_OnDispute(transactionId, playerId);

                LOGI("Dispute opened for transaction %s by %s: %s", 
                     transactionId.c_str(), playerId.c_str(), reason.c_str());
                return true;
            }
        }
        return false;
    }

    bool ResolveDispute(const std::string& transactionId, bool refundBuyer) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& tx : m_RMTransactions) {
            if (tx.transactionId == transactionId) {
                if (refundBuyer) {
                    tx.paymentStatus = PaymentStatus::REFUNDED;
                    RefundToBuyer(tx);
                }
                // Kembalikan item ke seller jika perlu
                if (m_SerialTracker) {
                    m_SerialTracker->TransferOwnership(
                        tx.itemSerial,
                        tx.buyerId, tx.buyerName,
                        tx.sellerId, tx.sellerName,
                        "dispute_return"
                    );
                }
                LOGI("Dispute resolved for transaction %s: %s", 
                     transactionId.c_str(), refundBuyer ? "REFUNDED" : "DISMISSED");
                return true;
            }
        }
        return false;
    }

    // ================================================================
    // VERIFIKASI KE SERVER KEAMANAN TERPISAH
    // ================================================================
    bool VerifyItemToSecurityServer(const std::string& serial, const std::string& sellerId, const std::string& buyerId) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOGE("CURL init failed for security verification");
            return false;
        }

        Json::Value body;
        body["serial"] = serial;
        body["sellerId"] = sellerId;
        body["buyerId"] = buyerId;
        body["timestamp"] = (Json::UInt64)std::chrono::system_clock::now().time_since_epoch().count();
        body["appId"] = "FauzanEngine-Marketplace";
        body["checksum"] = GenerateSecurityChecksum(serial, sellerId, buyerId);

        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Server-Key: " + m_ServerAPIKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, m_ItemVerifyServerURL.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            LOGE("Security server unreachable - rejecting transaction");
            return false; // TOLAK jika server keamanan tidak bisa dihubungi
        }

        Json::Value root;
        Json::Reader reader;
        if (reader.parse(response, root)) {
            bool valid = root.get("valid", false).asBool();
            if (!valid) {
                std::string reason = root.get("reason", "Unknown").asString();
                LOGE("Security server rejected: %s", reason.c_str());
            }
            return valid;
        }
        return false;
    }

    // ================================================================
    // PAYMENT GATEWAY INTEGRATION
    // ================================================================
    std::string ProcessPaymentToGateway(const RealMoneyTransaction& tx) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        Json::Value body;
        body["transactionId"] = tx.transactionId;
        body["amount"] = tx.amount;
        body["currency"] = tx.currency;
        body["paymentMethod"] = (int)tx.paymentMethod;
        body["buyerId"] = tx.buyerId;
        body["buyerName"] = tx.buyerName;
        body["sellerId"] = tx.sellerId;
        body["sellerName"] = tx.sellerName;
        body["devCommission"] = tx.devCommission;
        body["sellerPayout"] = tx.sellerPayout;
        body["appId"] = "FauzanEngine";

        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-API-Key: " + m_ServerAPIKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, m_PaymentGatewayURL.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        Json::Value root;
        Json::Reader reader;
        if (reader.parse(response, root)) {
            return root.get("gatewayId", "").asString();
        }
        return "";
    }

    // ================================================================
    // RECEIPT & REFUND
    // ================================================================
    void GenerateReceipt(const RealMoneyTransaction& tx) {
        PaymentReceipt receipt;
        receipt.receiptId = "RCPT-" + tx.transactionId;
        receipt.transactionId = tx.transactionId;
        receipt.playerId = tx.buyerId;
        receipt.totalAmount = tx.amount;
        receipt.devFee = tx.devCommission;
        receipt.netAmount = tx.sellerPayout;
        receipt.currency = tx.currency;
        receipt.issuedAt = std::chrono::system_clock::now();
        receipt.digitalSignature = GenerateReceiptSignature(receipt);

        m_Receipts.push_back(receipt);
        if (m_OnReceiptGenerated) m_OnReceiptGenerated(receipt);

        // Kirim receipt ke buyer
        SendReceiptToPlayer(tx.buyerId, receipt);
    }

    bool RefundToBuyer(const RealMoneyTransaction& tx) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        Json::Value body;
        body["transactionId"] = tx.transactionId;
        body["amount"] = tx.amount;
        body["reason"] = "dispute_refund";

        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-API-Key: " + m_ServerAPIKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, (m_PaymentGatewayURL + "/refund").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return res == CURLE_OK;
    }

    void SendReceiptToPlayer(const std::string& playerId, const PaymentReceipt& receipt) {
        CURL* curl = curl_easy_init();
        if (!curl) return;

        Json::Value body;
        body["playerId"] = playerId;
        body["receiptId"] = receipt.receiptId;
        body["totalAmount"] = receipt.totalAmount;
        body["devFee"] = receipt.devFee;
        body["netAmount"] = receipt.netAmount;
        body["signature"] = receipt.digitalSignature;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);

        curl_easy_setopt(curl, CURLOPT_URL, (m_MarketplaceServerURL + "/receipt").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    void ReportDisputeToServer(const std::string& txId, const std::string& playerId,
                               const std::string& reason, const std::string& evidence) {
        CURL* curl = curl_easy_init();
        if (!curl) return;

        Json::Value body;
        body["transactionId"] = txId;
        body["reporterId"] = playerId;
        body["reason"] = reason;
        body["evidence"] = evidence;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(body);

        curl_easy_setopt(curl, CURLOPT_URL, (m_AntiCheatServerURL + "/dispute").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    // ================================================================
    // ID GENERATORS
    // ================================================================
    std::string GenerateListingID() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return "LST-" + std::to_string(ms) + "-" + std::to_string(rand() % 10000);
    }

    std::string GenerateTransactionID() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return "RMT-" + std::to_string(ms) + "-" + std::to_string(rand() % 10000);
    }

    std::string GenerateEscrowID() {
        return "ESC-" + std::to_string(rand() % 1000000);
    }

    std::string GenerateSecurityChecksum(const std::string& serial, const std::string& sellerId, const std::string& buyerId) {
        std::string raw = serial + sellerId + buyerId + m_ServerAPIKey;
        return std::to_string(std::hash<std::string>{}(raw));
    }

    std::string GenerateReceiptSignature(const PaymentReceipt& receipt) {
        std::string raw = receipt.receiptId + receipt.transactionId + 
                         std::to_string(receipt.totalAmount) + m_ServerAPIKey;
        return std::to_string(std::hash<std::string>{}(raw));
    }

    // ================================================================
    // GETTERS & DASHBOARD
    // ================================================================
    std::vector<TradeListing> GetActiveListings(TradeCurrency currency = TradeCurrency::REAL_MONEY) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::vector<TradeListing> result;
        for (auto& l : m_Listings) {
            if (l.status == ListingStatus::ACTIVE && l.currency == currency) result.push_back(l);
        }
        return result;
    }

    std::vector<TradeListing> GetPlayerListings(const std::string& playerId) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::vector<TradeListing> result;
        for (auto& l : m_Listings) {
            if (l.sellerId == playerId) result.push_back(l);
        }
        return result;
    }

    std::vector<RealMoneyTransaction> GetPlayerTransactions(const std::string& playerId) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::vector<RealMoneyTransaction> result;
        for (auto& tx : m_RMTransactions) {
            if (tx.buyerId == playerId || tx.sellerId == playerId) result.push_back(tx);
        }
        return result;
    }

    PaymentReceipt* GetReceipt(const std::string& receiptId) {
        for (auto& r : m_Receipts) if (r.receiptId == receiptId) return &r;
        return nullptr;
    }

    std::string GetMarketplaceDashboard() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::stringstream ss;
        ss << "=== FAUZANENGINE MARKETPLACE DASHBOARD ===\n";
        ss << "Total Transactions: " << m_TotalTransactions << "\n";
        ss << "Total Market Volume: $" << std::fixed << std::setprecision(2) << m_TotalMarketVolume << "\n";
        ss << "Total Dev Revenue (5%): $" << m_TotalDevRevenue << "\n";
        ss << "Active Listings: " << std::count_if(m_Listings.begin(), m_Listings.end(),
            [](auto& l) { return l.status == ListingStatus::ACTIVE; }) << "\n";
        ss << "Pending Disputes: " << m_DisputeQueue.size() << "\n";
        return ss.str();
    }

    float GetDevRevenue() const { return m_TotalDevRevenue; }
    int GetTotalTransactions() const { return m_TotalTransactions; }

    // ================================================================
    // SETTERS
    // ================================================================
    void SetServerAPIKey(const std::string& key) { m_ServerAPIKey = key; }
    void SetItemVerifyServer(const std::string& url) { m_ItemVerifyServerURL = url; }
    void SetPaymentGatewayURL(const std::string& url) { m_PaymentGatewayURL = url; }
    void SetAntiCheatServer(const std::string& url) { m_AntiCheatServerURL = url; }
    void SetMarketplaceServer(const std::string& url) { m_MarketplaceServerURL = url; }
    void SetOnListingCreated(std::function<void(const TradeListing&)> cb) { m_OnListingCreated = cb; }
    void SetOnPaymentComplete(std::function<void(const RealMoneyTransaction&)> cb) { m_OnPaymentComplete = cb; }
    void SetOnDispute(std::function<void(const std::string&, const std::string&)> cb) { m_OnDispute = cb; }
    void SetOnReceiptGenerated(std::function<void(const PaymentReceipt&)> cb) { m_OnReceiptGenerated = cb; }
    void SetOnDevRevenue(std::function<void(const std::string&, float)> cb) { m_OnDevRevenue = cb; }
};

} // namespace NeoEngine
