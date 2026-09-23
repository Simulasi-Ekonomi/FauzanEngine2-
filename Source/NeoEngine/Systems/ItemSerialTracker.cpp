#include "ItemSerialTracker.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>

namespace NeoEngine {

ItemSerialTracker::ItemSerialTracker() {
    m_RNG.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::string ItemSerialTracker::GenerateSerial(const std::string& itemType, const std::string& itemName) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_TotalItems++;
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::string typeCode = "XX";
    if (itemType == "weapon") typeCode = "WP";
    else if (itemType == "armor") typeCode = "AR";
    else if (itemType == "potion") typeCode = "PT";
    else if (itemType == "material") typeCode = "MT";
    else if (itemType == "currency") typeCode = "CR";
    else if (itemType == "reward") typeCode = "RW";

    const uint64_t randomPart = m_RNG() % 99999ULL;
    const uint64_t checksum = (static_cast<uint64_t>(ms) + randomPart + itemName.size()) % 9999ULL;
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "FE-%s-%04llX-%05llu-%04llu",
                  typeCode.c_str(),
                  static_cast<unsigned long long>(ms % 65535),
                  static_cast<unsigned long long>(randomPart),
                  static_cast<unsigned long long>(checksum));
    return buffer;
}

SerialNumber* ItemSerialTracker::RegisterItem(const std::string& ownerId, const std::string& itemType,
                                              const std::string& itemName, int quantity, const std::string& source) {
    if (quantity <= 0 || ownerId.empty() || itemType.empty() || itemName.empty()) return nullptr;

    const std::string serial = GenerateSerial(itemType, itemName);
    std::lock_guard<std::mutex> lock(m_Mutex);
    SerialNumber sn;
    sn.number = serial;
    sn.itemType = itemType;
    sn.itemName = itemName;
    sn.quantity = quantity;
    sn.source = source;
    sn.ownerId = ownerId;
    sn.timestamp = std::chrono::system_clock::now();
    auto [it, inserted] = m_Registry.emplace(serial, std::move(sn));
    if (!inserted) return nullptr;
    m_PendingVerification.push_back(serial);
    m_AuditEvents.push_back("REGISTER|" + serial + "|" + ownerId + "|" + source);
    return &it->second;
}

bool ItemSerialTracker::VerifyItemSilently(const std::string& internalItemId, const std::string& playerId) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(internalItemId);
    if (it == m_Registry.end()) return false;
    return it->second.ownerId == playerId && it->second.verified && !it->second.consumed;
}

bool ItemSerialTracker::VerifyWithServer(const std::string& serialNumber) {
    std::string url;
    std::string publicKey;
    std::string payload;

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Registry.find(serialNumber) == m_Registry.end()) return false;
        url = m_ServerURL;
        publicKey = m_ServerPublicKey;

        Json::Value request;
        request["serialNumber"] = serialNumber;
        request["publicKey"] = publicKey;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        payload = Json::writeString(writer, request);
    }

    bool verified = false;
    std::string response;

    // A configured server performs authoritative verification. With no public key,
    // the engine remains deterministic/offline for local tooling and smoke tests.
    if (!publicKey.empty() && !url.empty()) {
        CURL* curl = curl_easy_init();
        if (curl != nullptr) {
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &ItemSerialTracker::WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1500L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 3000L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

            const CURLcode result = curl_easy_perform(curl);
            long status = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
            if (result == CURLE_OK && status >= 200 && status < 300) {
                Json::CharReaderBuilder reader;
                Json::Value parsed;
                std::string errors;
                std::unique_ptr<Json::CharReader> jsonReader(reader.newCharReader());
                if (jsonReader->parse(response.data(), response.data() + response.size(), &parsed, &errors)) {
                    verified = parsed.get("verified", false).asBool();
                }
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    } else {
        verified = true;
    }

    std::function<void(const std::string&)> onVerified;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        const auto it = m_Registry.find(serialNumber);
        if (it == m_Registry.end() || it->second.consumed) return false;

        if (verified) {
            if (!it->second.verified) {
                ++m_VerifiedCount;
                it->second.verified = true;
            }
            it->second.serverSignature = publicKey.empty() ? "LOCAL-AUTHORITY" : "REMOTE-AUTHORITY";
            m_AuditEvents.push_back("VERIFY|" + serialNumber);
            onVerified = m_OnVerified;
        } else {
            ++m_RejectedCount;
            m_AuditEvents.push_back("REJECT|" + serialNumber);
        }
    }

    if (onVerified) onVerified(serialNumber);
    return verified;
}

bool ItemSerialTracker::IsItemValid(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && it->second.verified && !it->second.consumed;
}

bool ItemSerialTracker::ConsumeItem(const std::string& serialNumber) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    if (it == m_Registry.end() || it->second.consumed || !it->second.verified) return false;
    it->second.consumed = true;
    m_AuditEvents.push_back("CONSUME|" + serialNumber + "|" + it->second.ownerId);
    return true;
}

int ItemSerialTracker::VerifyPendingBatch(int maxBatch) {
    if (maxBatch <= 0) return 0;

    std::vector<std::string> batch;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        const size_t count = std::min(static_cast<size_t>(maxBatch), m_PendingVerification.size());
        batch.assign(m_PendingVerification.begin(), m_PendingVerification.begin() + count);
        m_PendingVerification.erase(m_PendingVerification.begin(), m_PendingVerification.begin() + count);
    }

    int verified = 0;
    for (const std::string& serial : batch) {
        if (VerifyWithServer(serial)) ++verified;
    }
    return verified;
}

std::string ItemSerialTracker::GenerateCurrencySerial(const std::string& currencyType, int amount, const std::string& source) {
    if (amount <= 0 || currencyType.empty()) return {};
    const std::string serial = GenerateSerial("currency", currencyType + ":" + std::to_string(amount));
    std::lock_guard<std::mutex> lock(m_Mutex);
    SerialNumber sn;
    sn.number = serial;
    sn.itemType = "currency";
    sn.itemName = currencyType;
    sn.quantity = amount;
    sn.source = source;
    sn.timestamp = std::chrono::system_clock::now();
    m_Registry.emplace(serial, std::move(sn));
    m_PendingVerification.push_back(serial);
    m_AuditEvents.push_back("CURRENCY|" + serial + "|" + source);
    return serial;
}

std::string ItemSerialTracker::GenerateRewardSerial(const std::string& rewardName, const std::string& source) {
    if (rewardName.empty()) return {};
    const std::string serial = GenerateSerial("reward", rewardName);
    std::lock_guard<std::mutex> lock(m_Mutex);
    SerialNumber sn;
    sn.number = serial;
    sn.itemType = "reward";
    sn.itemName = rewardName;
    sn.quantity = 1;
    sn.source = source;
    sn.timestamp = std::chrono::system_clock::now();
    m_Registry.emplace(serial, std::move(sn));
    m_PendingVerification.push_back(serial);
    m_AuditEvents.push_back("REWARD|" + serial + "|" + source);
    return serial;
}

bool ItemSerialTracker::IsSerialDuplicate(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && it->second.consumed;
}

std::string ItemSerialTracker::GetAuditTrail(const std::string& playerId) const {
    if (playerId.empty()) return {};
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::ostringstream out;
    bool found = false;
    for (const auto& event : m_AuditEvents) {
        if (event.find("|" + playerId + "|") != std::string::npos ||\n            (event.size() > playerId.size() + 1U && event.compare(event.size() - playerId.size() - 1U, playerId.size() + 1U, "|" + playerId) == 0)) {
            if (found) out << '\n';
            out << event;
            found = true;
        }
    }
    return out.str();
}

void ItemSerialTracker::MarkAllPlayerItemsContaminated(const std::string& playerId, const std::string& playerName) {
    if (playerId.empty()) return;

    std::function<void(const std::string&, const std::string&)> onCheatDetected;
    int affected = 0;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& [serial, item] : m_Registry) {
            if (item.ownerId == playerId && !item.consumed) {
                if (item.verified) {
                    item.verified = false;
                    ++m_RejectedCount;
                }
                ++affected;
                m_AuditEvents.push_back("CONTAMINATE|" + serial + "|" + playerId);
            }
        }
        if (affected > 0) {
            onCheatDetected = m_OnCheatDetected;
        }
    }

    if (onCheatDetected) {
        onCheatDetected(playerId, playerName);
    }
}

bool ItemSerialTracker::TransferOwnership(const std::string& serial, const std::string& fromId,
                                          const std::string& fromName, const std::string& toId,
                                          const std::string& toName, const std::string& method) {
    if (serial.empty() || fromId.empty() || toId.empty() || fromId == toId || method.empty()) return false;

    const std::string transferHash = GenerateTransferHash(serial, fromId, toId, method);
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serial);
    if (it == m_Registry.end() || it->second.consumed || it->second.ownerId != fromId || !it->second.verified) {
        return false;
    }

    it->second.ownerId = toId;
    it->second.serverSignature = transferHash;
    m_AuditEvents.push_back("TRANSFER|" + serial + "|" + fromId + "|" + toId + "|" + method);
    return true;
}

std::string ItemSerialTracker::GenerateTransferHash(const std::string& serial, const std::string& from,
                                                    const std::string& to, const std::string& method) const {
    return std::to_string(std::hash<std::string>{}(serial + "|" + from + "|" + to + "|" + method));
}

size_t ItemSerialTracker::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    if (contents == nullptr || output == nullptr) return 0;
    const size_t total = size * nmemb;
    output->append(static_cast<const char*>(contents), total);
    return total;
}

} // namespace NeoEngine
