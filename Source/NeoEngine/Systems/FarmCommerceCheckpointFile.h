#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace NeoEngine {

class FarmCommerceEntitlementLedger;
class FarmWorldTool;

enum class FarmCommerceCheckpointFileError : uint8_t { None, CheckpointRejected, StorageWriteRejected, StorageReadRejected };

class FarmCommerceCheckpointFile {
public:
    static bool Save(const std::filesystem::path& root, std::string_view slot, const FarmWorldTool& world,
                     const FarmCommerceEntitlementLedger& ledger, FarmCommerceCheckpointFileError& error);
    static bool Load(const std::filesystem::path& root, std::string_view slot, FarmWorldTool& world,
                     FarmCommerceEntitlementLedger& ledger, FarmCommerceCheckpointFileError& error);
};

} // namespace NeoEngine
