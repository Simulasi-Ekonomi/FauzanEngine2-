#include "FarmCommerceCheckpoint.h"

#include "FarmCommerceEntitlementLedger.h"
#include "FarmWorldTool.h"

namespace NeoEngine {
namespace {

constexpr uint32_t kMagic = 0x504D4346U; // FCMP
constexpr uint16_t kVersion = 1U;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

template <typename T>
void Append(std::vector<uint8_t>& bytes, T value) {
    for (size_t index = 0U; index < sizeof(T); ++index) bytes.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (index * 8U)) & 0xFFU));
}

template <typename T>
bool Read(std::span<const uint8_t> bytes, size_t& offset, T& value) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(T)) return false;
    uint64_t raw = 0U;
    for (size_t index = 0U; index < sizeof(T); ++index) raw |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8U);
    value = static_cast<T>(raw);
    offset += sizeof(T);
    return true;
}

uint64_t Hash(std::span<const uint8_t> bytes) {
    uint64_t hash = kHashOffset;
    for (uint8_t byte : bytes) { hash ^= byte; hash *= kHashPrime; }
    return hash;
}

} // namespace

bool FarmCommerceCheckpoint::Save(const FarmWorldTool& world, const FarmCommerceEntitlementLedger& ledger, std::vector<uint8_t>& bytes) {
    const std::vector<uint8_t> worldBytes = world.Serialize();
    const std::vector<uint8_t> ledgerBytes = ledger.SerializeState();
    if (worldBytes.empty() || ledgerBytes.empty() || worldBytes.size() > kMaxBytes || ledgerBytes.size() > kMaxBytes ||
        worldBytes.size() + ledgerBytes.size() + 22U > kMaxBytes) return false;
    try {
        std::vector<uint8_t> candidate;
        candidate.reserve(14U + worldBytes.size() + ledgerBytes.size() + sizeof(uint64_t));
        Append<uint32_t>(candidate, kMagic);
        Append<uint16_t>(candidate, kVersion);
        Append<uint32_t>(candidate, static_cast<uint32_t>(worldBytes.size()));
        Append<uint32_t>(candidate, static_cast<uint32_t>(ledgerBytes.size()));
        candidate.insert(candidate.end(), worldBytes.begin(), worldBytes.end());
        candidate.insert(candidate.end(), ledgerBytes.begin(), ledgerBytes.end());
        Append<uint64_t>(candidate, Hash(candidate));
        bytes = std::move(candidate);
        return true;
    } catch (...) {
        return false;
    }
}

bool FarmCommerceCheckpoint::Load(std::span<const uint8_t> bytes, FarmWorldTool& world, FarmCommerceEntitlementLedger& ledger) {
    if (bytes.size() < 22U || bytes.size() > kMaxBytes) return false;
    const size_t hashOffset = bytes.size() - sizeof(uint64_t);
    uint64_t expectedHash = 0U;
    size_t hashReadOffset = hashOffset;
    if (!Read<uint64_t>(bytes, hashReadOffset, expectedHash) || hashReadOffset != bytes.size() || Hash(bytes.first(hashOffset)) != expectedHash) return false;
    size_t offset = 0U;
    uint32_t magic = 0U;
    uint16_t version = 0U;
    uint32_t worldLength = 0U;
    uint32_t ledgerLength = 0U;
    if (!Read<uint32_t>(bytes, offset, magic) || !Read<uint16_t>(bytes, offset, version) || !Read<uint32_t>(bytes, offset, worldLength) || !Read<uint32_t>(bytes, offset, ledgerLength) ||
        magic != kMagic || version != kVersion || worldLength == 0U || ledgerLength == 0U || offset + static_cast<size_t>(worldLength) + static_cast<size_t>(ledgerLength) != hashOffset) return false;
    const std::span<const uint8_t> worldBytes(bytes.data() + offset, worldLength);
    offset += worldLength;
    const std::span<const uint8_t> ledgerBytes(bytes.data() + offset, ledgerLength);
    const std::vector<uint8_t> previousWorld = world.Serialize();
    const std::vector<uint8_t> previousLedger = ledger.SerializeState();
    if (previousWorld.empty() || previousLedger.empty()) return false;
    if (!world.Deserialize(worldBytes) || !ledger.RestoreState(ledgerBytes)) {
        world.Deserialize(previousWorld);
        ledger.RestoreState(previousLedger);
        return false;
    }
    return true;
}

} // namespace NeoEngine
