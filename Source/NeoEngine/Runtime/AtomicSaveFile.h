#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class AtomicSaveFileError : uint8_t { None, InvalidSlot, PayloadLimit, CreateDirectory, OpenWrite, WriteFailure, RenameFailure, Missing, OpenRead, ReadFailure, BackupFailure, RestoreFailure };
class AtomicSaveFile {
public:
    static constexpr size_t kMaxBytes = 1024U * 1024U;
    static bool Write(const std::filesystem::path& root, std::string_view slot, const std::vector<uint8_t>& bytes, AtomicSaveFileError& error);
    static bool Read(const std::filesystem::path& root, std::string_view slot, std::vector<uint8_t>& bytes, AtomicSaveFileError& error);
    static bool Backup(const std::filesystem::path& root, std::string_view slot, AtomicSaveFileError& error);
    static bool RestoreBackup(const std::filesystem::path& root, std::string_view slot, AtomicSaveFileError& error);
};
} // namespace NeoEngine
