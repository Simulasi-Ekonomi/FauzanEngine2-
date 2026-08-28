#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace NeoEngine {

enum class FarmProgressCheckpointFileError : uint8_t {
    None,
    EmptyCheckpoint,
    PayloadLimit,
    StorageWriteRejected,
    StorageReadRejected
};

class FarmProgressCheckpointFile {
public:
    static constexpr size_t kMaxBytes = 1024U * 1024U;
    static bool Save(const std::filesystem::path& root, std::string_view slot, std::span<const uint8_t> checkpoint,
                     FarmProgressCheckpointFileError& error);
    static bool Load(const std::filesystem::path& root, std::string_view slot, std::vector<uint8_t>& checkpoint,
                     FarmProgressCheckpointFileError& error);
};

} // namespace NeoEngine
