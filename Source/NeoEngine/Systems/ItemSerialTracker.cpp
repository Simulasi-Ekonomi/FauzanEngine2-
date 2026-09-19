#include "ItemSerialTracker.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
namespace {
std::string TypeCode(const std::string& itemType) {
    if (itemType == "weapon") return "WP";
    if (itemType == "armor") return "AR";
    if (itemType == "potion") return "PT";
    if (itemType == "material") return "MT";
    if (itemType == "currency") return "CR";
    if (itemType == "gem") return "GM";
    if (itemType == "skin") return "SK";
    if (itemType == "key") return "KY";
    if (itemType == "chest") return "CH";
    if (itemType == "reward") return "RW";
    return "XX";
}

std::string StableHashHex(const std::string& input) {
    // Non-cryptographic integrity identifier. Cryptographic authenticity belongs to the server signature.
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : input) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << h;
    return out.str();
}
} // namespace

ItemSerialTracker::ItemSerialTracker()
    : m_RNG(std::random_device{}()) {}

std::string ItemSerialTracker::GenerateSerial(const std::string& itemType, const std::string& itemName) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    ++m_TotalItems;
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    const uint64_t randomPart = m_RNG() % 100000ULL;
    const uint64_t checksum = (static_cast<uint64_t>(ms) + randomPart + itemName.size()) % 10000ULL;

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "FE-%s-%04llX-%05llu-%04llu",
                  TypeCode(itemType).c_str(),
                  static_cast<unsigned long long>(ms & 0xFFFFULL),
                  static_cast<unsigned long long>(randomPart),
                  static_cast<unsigned long long>(checksum));
    return buffer;
}

SerialNumber* ItemSerialTracker::RegisterItem(const std::string& ownerId, const std::string& itemType,
                                               const std::string& itemName, int quantity,
                                               const std::string& source) {
    if (ownerId.empty() || itemType.empty() || itemName.empty() || source.empty() || quantity <= 0)
        return nullptr;

    const std::string serial = GenerateSerial(itemType, itemName);
    const auto now = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(m_Mutex);
    auto [it, inserted] = m_Registry.try_emplace(serial);
    if (!inserted) {
        ++m_RejectedCount;
        return nullptr;
    }

    SerialNumber& sn = it->second;
    sn.number = serial;
    sn.itemType = itemType;
    sn.itemName = itemName;
    sn.quantity = quantity;
    sn.source = source;
    sn.ownerId = ownerId;
    sn.originalOwnerId = ownerId;
    sn.timestamp = now;
    sn.verified = false;
    sn.consumed = false;
    sn.contaminated = false;

    OwnershipRecord initial;
    initial.playerId = ownerId;
    initial.previousOwner = "SYSTEM";
    initial.transferMethod = source;
    initial.timestamp = now;
    initial.transferHash = GenerateTransferHash(serial, "SYSTEM", ownerId, source, 0);
    sn.ownershipChain.push_back(std::move(initial));
    m_PendingVerification.push_back(serial);
    return &sn;
}

bool ItemSerialTracker::VerifyItemSilently(const std::string& internalItemId, const std::string& playerId) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& [serial, sn] : m_Registry) {
        if (serial != internalItemId) continue;
        return sn.ownerId == playerId && sn.verified && !sn.consumed && !sn.contaminated;
    }
    return false;
}

bool ItemSerialTracker::VerifyWithServer(const std::string& serialNumber) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Registry.find(serialNumber);
    if (it == m_Registry.end() || it->second.consumed || it->second.contaminated) {
        ++m_RejectedCount;
        return false;
    }

    // This API is the authoritative verification commit point. Network transport/signature
    // verification is deliberately not faked here; a caller must only invoke it after its
    // trusted authority has accepted the serial.
    if (!it->second.verified) {
        it->second.verified = true;
        ++m_VerifiedCount;
    }
    m_PendingVerification.erase(
        std::remove(m_PendingVerification.begin(), m_PendingVerification.end(), serialNumber),
        m_PendingVerification.end());
    const auto callback = m_OnVerified;
    // Never execute user callbacks while holding the tracker mutex.
    if (callback) callback(serialNumber);
    return true;
}

int ItemSerialTracker::VerifyPendingBatch(int maxBatch) {
    if (maxBatch <= 0) return 0;

    std::vector<std::string> batch;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        const std::size_t count = std::min<std::size_t>(
            static_cast<std::size_t>(maxBatch), m_PendingVerification.size());
        batch.assign(m_PendingVerification.begin(), m_PendingVerification.begin() + count);
    }

    int verified = 0;
    for (const auto& serial : batch) {
        if (VerifyWithServer(serial)) ++verified;
    }
    return verified;
}

bool ItemSerialTracker::IsItemValid(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && it->second.verified &&
           !it->second.consumed && !it->second.contaminated;
}

bool ItemSerialTracker::ConsumeItem(const std::string& serialNumber) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    if (it == m_Registry.end() || it->second.consumed || !it->second.verified || it->second.contaminated)
        return false;
    it->second.consumed = true;
    return true;
}

std::string ItemSerialTracker::GenerateCurrencySerial(const std::string& currencyType, int amount,
                                                      const std::string& source) {
    if (amount <= 0 || currencyType.empty()) return {};
    return GenerateSerial("currency", currencyType + ":" + std::to_string(amount));
}

std::string ItemSerialTracker::GenerateRewardSerial(const std::string& rewardName, const std::string& source) {
    if (rewardName.empty()) return {};
    return GenerateSerial("reward", rewardName);
}

bool ItemSerialTracker::IsSerialDuplicate(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && (it->second.consumed || it->second.contaminated);
}

std::string ItemSerialTracker::GetAuditTrail(const std::string& playerId) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::ostringstream out;
    bool found = false;
    for (const auto& [serial, sn] : m_Registry) {
        if (sn.ownerId != playerId && sn.originalOwnerId != playerId) continue;
        found = true;
        out << serial << " | " << sn.itemType << " | " << sn.itemName
            << " | owner=" << sn.ownerId
            << " | verified=" << (sn.verified ? "1" : "0")
            << " | consumed=" << (sn.consumed ? "1" : "0")
            << " | contaminated=" << (sn.contaminated ? "1" : "0") << '\n';
        for (const auto& record : sn.ownershipChain) {
            out << "  " << record.playerId << " via " << record.transferMethod
                << " from " << record.previousOwner
                << " hash=" << record.transferHash << '\n';
        }
    }
    return found ? out.str() : {};
}

std::string ItemSerialTracker::GetOwnershipChain(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    if (it == m_Registry.end()) return {};

    std::ostringstream out;
    for (std::size_t i = 0; i < it->second.ownershipChain.size(); ++i) {
        const auto& r = it->second.ownershipChain[i];
        out << (i + 1) << ". " << r.playerId << " via " << r.transferMethod
            << " from " << r.previousOwner << " hash=" << r.transferHash << '\n';
    }
    return out.str();
}

bool ItemSerialTracker::VerifyOwnershipIntegrity(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    if (it == m_Registry.end() || it->second.ownershipChain.empty()) return false;

    const auto& chain = it->second.ownershipChain;
    if (chain.front().previousOwner != "SYSTEM" ||
        chain.front().playerId != it->second.originalOwnerId) return false;

    for (std::size_t i = 0; i < chain.size(); ++i) {
        const auto& record = chain[i];
        const std::string from = (i == 0) ? "SYSTEM" : chain[i - 1].playerId;
        if (record.previousOwner != from ||
            record.transferHash != GenerateTransferHash(serialNumber, from, record.playerId,
                                                         record.transferMethod, i))
            return false;
    }
    return chain.back().playerId == it->second.ownerId;
}

void ItemSerialTracker::MarkAllPlayerItemsContaminated(const std::string& playerId,
                                                        const std::string& playerName) {
    if (playerId.empty()) return;

    std::vector<std::string> affected;
    std::function<void(const std::string&, const std::string&)> cheatCallback;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& [serial, sn] : m_Registry) {
            if (sn.ownerId == playerId || sn.originalOwnerId == playerId) {
                if (!sn.consumed) {
                    sn.contaminated = true;
                    affected.push_back(serial);
                }
            }
        }
        cheatCallback = m_OnCheatDetected;
    }

    if (cheatCallback) {
        cheatCallback(playerId, "Contaminated inventory: " + playerName +
                                " affected " + std::to_string(affected.size()) + " item(s)");
    }
}

bool ItemSerialTracker::TransferOwnership(const std::string& serial, const std::string& fromId,
                                          const std::string& fromName, const std::string& toId,
                                          const std::string& toName, const std::string& method) {
    if (serial.empty() || fromId.empty() || toId.empty() || method.empty() || fromId == toId)
        return false;

    std::function<void(const std::string&, const std::string&)> cheatCallback;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(serial);
        if (it == m_Registry.end() || it->second.consumed || it->second.contaminated)
            return false;

        SerialNumber& sn = it->second;
        if (!sn.verified || sn.ownerId != fromId) {
            cheatCallback = m_OnCheatDetected;
        } else {
            const std::size_t sequence = sn.ownershipChain.size();
            OwnershipRecord record;
            record.playerId = toId;
            record.playerName = toName;
            record.previousOwner = fromId;
            record.transferMethod = method;
            record.timestamp = std::chrono::system_clock::now();
            record.transferHash = GenerateTransferHash(serial, fromId, toId, method, sequence);
            sn.ownershipChain.push_back(std::move(record));
            sn.ownerId = toId;
            sn.ownerName = toName;
            return true;
        }
    }

    if (cheatCallback)
        cheatCallback(fromId, "Unauthorized ownership transfer: " + serial);
    return false;
}

std::string ItemSerialTracker::GenerateTransferHash(const std::string& serial, const std::string& from,
                                                    const std::string& to, const std::string& method,
                                                    std::size_t sequence) const {
    return StableHashHex(serial + "|" + from + "|" + to + "|" + method + "|" +
                         std::to_string(sequence));
}

size_t ItemSerialTracker::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    if (!contents || !output || size == 0 || nmemb > std::numeric_limits<size_t>::max() / size) return 0;
    const size_t bytes = size * nmemb;
    output->append(static_cast<const char*>(contents), bytes);
    return bytes;
}

} // namespace NeoEngine
