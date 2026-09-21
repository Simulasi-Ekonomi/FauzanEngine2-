#include "ItemSerialTracker.h"
#include <mutex>
#include <limits>

namespace NeoEngine {

ItemSerialTracker::ItemSerialTracker() {
    m_RNG.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

std::string ItemSerialTracker::GenerateSerial(const std::string& itemType, const std::string& itemName) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string typeCode = "XX";
    if (itemType == "weapon") typeCode = "WP";
    else if (itemType == "armor") typeCode = "AR";
    else if (itemType == "potion") typeCode = "PT";
    else if (itemType == "material") typeCode = "MT";
    else if (itemType == "currency") typeCode = "CR";
    
    uint64_t randomPart = m_RNG() % 99999;
    uint64_t checksum = (ms + randomPart + itemName.length()) % 9999;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "FE-%s-%04llX-%05llu-%04llu",
             typeCode.c_str(), (unsigned long long)(ms % 65535),
             (unsigned long long)randomPart, (unsigned long long)checksum);
    return std::string(buffer);
}

SerialNumber* ItemSerialTracker::RegisterItem(const std::string& ownerId, const std::string& itemType,
                                              const std::string& itemName, int quantity, const std::string& source) {
    if (ownerId.empty() || itemType.empty() || itemName.empty() || source.empty() || quantity <= 0) return nullptr;
    std::string serial = GenerateSerial(itemType, itemName);
    SerialNumber sn;
    sn.number = serial;
    sn.itemType = itemType;
    sn.itemName = itemName;
    sn.quantity = quantity;
    sn.source = source;
    sn.ownerId = ownerId;
    sn.timestamp = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Registry.find(sn.number) != m_Registry.end()) return nullptr;
    if (m_TotalItems == std::numeric_limits<int>::max()) return nullptr;
    m_Registry.emplace(sn.number, std::move(sn));
    m_PendingVerification.push_back(serial);
    ++m_TotalItems;
    return &m_Registry.find(serial)->second;
}

bool ItemSerialTracker::VerifyItemSilently(const std::string& internalItemId, const std::string& playerId) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& [serial, sn] : m_Registry) {
        if (sn.number == internalItemId) {
            return (sn.ownerId == playerId) && sn.verified && !sn.consumed;
        }
    }
    return false;
}

bool ItemSerialTracker::VerifyWithServer(const std::string& serialNumber) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Registry.find(serialNumber);
    if (it != m_Registry.end()) {
        it->second.verified = true;
        m_VerifiedCount++;
        return true;
    }
    return false;
}

bool ItemSerialTracker::IsItemValid(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_Mutex));
    auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && it->second.verified && !it->second.consumed;
}

bool ItemSerialTracker::ConsumeItem(const std::string& serialNumber) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Registry.find(serialNumber);
    if (it != m_Registry.end() && !it->second.consumed) {
        it->second.consumed = true;
        return true;
    }
    return false;
}

int ItemSerialTracker::VerifyPendingBatch(int maxBatch) {
    if (maxBatch <= 0) return 0;
    int verified = 0;
    while (verified < maxBatch) {
        std::string serial;
        { std::lock_guard<std::mutex> lock(m_Mutex); if (m_PendingVerification.empty()) break; serial = m_PendingVerification.front(); m_PendingVerification.erase(m_PendingVerification.begin()); }
        if (VerifyWithServer(serial)) ++verified; else { std::lock_guard<std::mutex> lock(m_Mutex); if (m_RejectedCount != std::numeric_limits<int>::max()) ++m_RejectedCount; }
    }
    return verified;
}

std::string ItemSerialTracker::GenerateCurrencySerial(const std::string& currencyType, int amount, const std::string& source) {
    return GenerateSerial("currency", currencyType + ":" + std::to_string(amount));
}

std::string ItemSerialTracker::GenerateRewardSerial(const std::string& rewardName, const std::string& source) {
    return GenerateSerial("reward", rewardName);
}

bool ItemSerialTracker::IsSerialDuplicate(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_Mutex));
    auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && it->second.consumed;
}

std::string ItemSerialTracker::GetAuditTrail(const std::string& playerId) const {
    return "Audit for " + playerId;
}

void ItemSerialTracker::MarkAllPlayerItemsContaminated(const std::string& playerId, const std::string& playerName) {
    if (playerId.empty()) return;
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& [serial, item] : m_Registry) if (item.ownerId == playerId) item.serverSignature = "CONTAMINATED:" + playerName;
}

std::string ItemSerialTracker::GenerateTransferHash(const std::string& serial, const std::string& from,
                                                    const std::string& to, const std::string& method) const {
    if (serial.empty() || from.empty() || to.empty() || method.empty()) return {};
    return std::to_string(std::hash<std::string>{}(serial + from + to + method));
}

size_t ItemSerialTracker::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

} // namespace NeoEngine
