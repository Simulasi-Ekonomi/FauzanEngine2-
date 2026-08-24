#pragma once

#include "AssetRegistry.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class AssetManifestError : uint8_t { None, Capacity, InvalidEntry, MissingDependency, DuplicateId, Corrupt, UnsupportedVersion, ChecksumMismatch, TrailingBytes, RegistryMismatch };
class AssetManifestSnapshot {
public:
    static constexpr uint8_t kVersion = 1;
    bool Capture(const AssetRegistry& registry);
    bool Serialize(std::vector<uint8_t>& out) const;
    bool Deserialize(const std::vector<uint8_t>& bytes);
    bool MatchesRegistry(const AssetRegistry& registry);
    [[nodiscard]] const std::vector<AssetDefinition>& Entries() const { return entries_; }
    [[nodiscard]] AssetManifestError LastError() const { return lastError_; }
private:
    std::vector<AssetDefinition> entries_;
    AssetManifestError lastError_ = AssetManifestError::None;
};
} // namespace NeoEngine
