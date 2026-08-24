#include "FarmAuthorityCheckpoint.h"

#include "FarmAuthoritativeService.h"
#include "FarmWorldTool.h"

namespace NeoEngine {
namespace {
constexpr uint32_t kMagic = 0x50434146U;
constexpr uint16_t kVersion = 1;
template <typename T> void Append(std::vector<uint8_t>& out, T value) { for (size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (i * 8U)) & 0xFFU)); }
template <typename T> bool Read(std::span<const uint8_t> in, size_t& offset, T& value) { if (offset + sizeof(T) > in.size()) return false; uint64_t raw = 0; for (size_t i = 0; i < sizeof(T); ++i) raw |= static_cast<uint64_t>(in[offset + i]) << (i * 8U); value = static_cast<T>(raw); offset += sizeof(T); return true; }
}

std::vector<uint8_t> FarmAuthoritativeService::SerializeAuthorityLedger() const { return ready_ ? gate_.SerializeState() : std::vector<uint8_t>{}; }
bool FarmAuthoritativeService::RestoreAuthorityLedger(std::span<const uint8_t> bytes) { return ready_ && gate_.DeserializeState(bytes); }

bool FarmAuthorityCheckpoint::Save(const FarmWorldTool& world, const FarmAuthoritativeService& service, std::vector<uint8_t>& bytes) {
    bytes.clear();
    const std::vector<uint8_t> worldBytes = world.Serialize();
    const std::vector<uint8_t> ledgerBytes = service.SerializeAuthorityLedger();
    if (worldBytes.empty() || ledgerBytes.empty() || worldBytes.size() > kMaxBytes || ledgerBytes.size() > kMaxBytes || worldBytes.size() + ledgerBytes.size() + 14U > kMaxBytes) return false;
    Append<uint32_t>(bytes, kMagic); Append<uint16_t>(bytes, kVersion); Append<uint32_t>(bytes, static_cast<uint32_t>(worldBytes.size())); Append<uint32_t>(bytes, static_cast<uint32_t>(ledgerBytes.size()));
    bytes.insert(bytes.end(), worldBytes.begin(), worldBytes.end()); bytes.insert(bytes.end(), ledgerBytes.begin(), ledgerBytes.end());
    return true;
}

bool FarmAuthorityCheckpoint::Load(std::span<const uint8_t> bytes, FarmWorldTool& world, FarmAuthoritativeService& service) {
    if (bytes.size() < 14U || bytes.size() > kMaxBytes) return false;
    size_t offset = 0; uint32_t magic = 0, worldLength = 0, ledgerLength = 0; uint16_t version = 0;
    if (!Read(bytes, offset, magic) || !Read(bytes, offset, version) || !Read(bytes, offset, worldLength) || !Read(bytes, offset, ledgerLength) || magic != kMagic || version != kVersion || worldLength == 0 || ledgerLength == 0 || offset + static_cast<size_t>(worldLength) + ledgerLength != bytes.size()) return false;
    const std::span<const uint8_t> worldBytes(bytes.data() + offset, worldLength); offset += worldLength;
    const std::span<const uint8_t> ledgerBytes(bytes.data() + offset, ledgerLength);
    const std::vector<uint8_t> previousWorld = world.Serialize();
    const std::vector<uint8_t> previousLedger = service.SerializeAuthorityLedger();
    if (previousWorld.empty() || previousLedger.empty()) return false;
    if (!world.Deserialize(worldBytes)) {
        world.Deserialize(previousWorld);
        service.RestoreAuthorityLedger(previousLedger);
        return false;
    }
    if (!service.RestoreAuthorityLedger(ledgerBytes)) {
        world.Deserialize(previousWorld);
        service.RestoreAuthorityLedger(previousLedger);
        return false;
    }
    return true;
}

} // namespace NeoEngine
