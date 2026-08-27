#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace NeoEngine {

class FarmAuthoritativeService;
class FarmWorldTool;

enum class FarmAuthorityCheckpointFileError : uint8_t { None, CheckpointRejected, StorageWriteRejected, StorageReadRejected };

class FarmAuthorityCheckpointFile {
public:
    static bool Save(const std::filesystem::path& root, std::string_view slot, const FarmWorldTool& world,
                     const FarmAuthoritativeService& service, FarmAuthorityCheckpointFileError& error);
    static bool Load(const std::filesystem::path& root, std::string_view slot, FarmWorldTool& world,
                     FarmAuthoritativeService& service, FarmAuthorityCheckpointFileError& error);
};

} // namespace NeoEngine
