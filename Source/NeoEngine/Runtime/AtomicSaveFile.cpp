#include "AtomicSaveFile.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace NeoEngine {
namespace {
bool ValidSlot(std::string_view slot) {
    return !slot.empty() && slot.size() <= 48U &&
           std::all_of(slot.begin(), slot.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
}

bool ReadPath(const std::filesystem::path& path, std::vector<uint8_t>& bytes, AtomicSaveFileError& error) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        error = AtomicSaveFileError::Missing;
        return false;
    }
    const uintmax_t size = std::filesystem::file_size(path, ec);
    if (ec) {
        error = AtomicSaveFileError::OpenRead;
        return false;
    }
    if (size > AtomicSaveFile::kMaxBytes) {
        error = AtomicSaveFileError::PayloadLimit;
        return false;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = AtomicSaveFileError::OpenRead;
        return false;
    }
    std::vector<uint8_t> parsed(static_cast<size_t>(size));
    if (size > 0U) stream.read(reinterpret_cast<char*>(parsed.data()), static_cast<std::streamsize>(size));
    if (!stream && size > 0U) {
        error = AtomicSaveFileError::ReadFailure;
        return false;
    }
    bytes = std::move(parsed);
    error = AtomicSaveFileError::None;
    return true;
}

bool WritePath(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    if (!bytes.empty()) stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.flush();
    return static_cast<bool>(stream);
}
} // namespace

bool AtomicSaveFile::Write(const std::filesystem::path& root, std::string_view slot, const std::vector<uint8_t>& bytes,
                           AtomicSaveFileError& error) {
    if (!ValidSlot(slot)) {
        error = AtomicSaveFileError::InvalidSlot;
        return false;
    }
    if (bytes.size() > kMaxBytes) {
        error = AtomicSaveFileError::PayloadLimit;
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) {
        error = AtomicSaveFileError::CreateDirectory;
        return false;
    }
    const std::filesystem::path finalPath = root / (std::string(slot) + ".sav");
    const std::filesystem::path tempPath = root / (std::string(slot) + ".tmp");
    if (!WritePath(tempPath, bytes)) {
        std::filesystem::remove(tempPath, ec);
        error = AtomicSaveFileError::WriteFailure;
        return false;
    }
    std::filesystem::rename(tempPath, finalPath, ec);
    if (ec) {
        std::filesystem::remove(tempPath, ec);
        error = AtomicSaveFileError::RenameFailure;
        return false;
    }
    error = AtomicSaveFileError::None;
    return true;
}

bool AtomicSaveFile::Read(const std::filesystem::path& root, std::string_view slot, std::vector<uint8_t>& bytes,
                          AtomicSaveFileError& error) {
    if (!ValidSlot(slot)) {
        error = AtomicSaveFileError::InvalidSlot;
        return false;
    }
    return ReadPath(root / (std::string(slot) + ".sav"), bytes, error);
}

bool AtomicSaveFile::Backup(const std::filesystem::path& root, std::string_view slot, AtomicSaveFileError& error) {
    if (!ValidSlot(slot)) {
        error = AtomicSaveFileError::InvalidSlot;
        return false;
    }
    std::vector<uint8_t> bytes;
    AtomicSaveFileError readError = AtomicSaveFileError::None;
    if (!ReadPath(root / (std::string(slot) + ".sav"), bytes, readError)) {
        error = AtomicSaveFileError::BackupFailure;
        return false;
    }
    std::error_code ec;
    const std::filesystem::path tempPath = root / (std::string(slot) + ".bak.tmp");
    const std::filesystem::path backupPath = root / (std::string(slot) + ".bak");
    if (!WritePath(tempPath, bytes)) {
        std::filesystem::remove(tempPath, ec);
        error = AtomicSaveFileError::BackupFailure;
        return false;
    }
    std::filesystem::rename(tempPath, backupPath, ec);
    if (ec) {
        std::filesystem::remove(tempPath, ec);
        error = AtomicSaveFileError::BackupFailure;
        return false;
    }
    error = AtomicSaveFileError::None;
    return true;
}

bool AtomicSaveFile::RestoreBackup(const std::filesystem::path& root, std::string_view slot,
                                   AtomicSaveFileError& error) {
    if (!ValidSlot(slot)) {
        error = AtomicSaveFileError::InvalidSlot;
        return false;
    }
    std::vector<uint8_t> bytes;
    AtomicSaveFileError readError = AtomicSaveFileError::None;
    if (!ReadPath(root / (std::string(slot) + ".bak"), bytes, readError)) {
        error = AtomicSaveFileError::RestoreFailure;
        return false;
    }
    AtomicSaveFileError writeError = AtomicSaveFileError::None;
    if (!Write(root, slot, bytes, writeError)) {
        error = AtomicSaveFileError::RestoreFailure;
        return false;
    }
    error = AtomicSaveFileError::None;
    return true;
}
} // namespace NeoEngine
