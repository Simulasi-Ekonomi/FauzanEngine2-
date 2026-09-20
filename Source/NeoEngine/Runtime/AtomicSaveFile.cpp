#include "AtomicSaveFile.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <fstream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace NeoEngine {
namespace {
bool ValidSlot(std::string_view slot) {
    return !slot.empty() && slot.size() <= 48U &&
           std::all_of(slot.begin(), slot.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
}

std::filesystem::path MakeTempPath(const std::filesystem::path& root, std::string_view slot, std::string_view suffix) {
    static std::atomic<uint64_t> sequence{0U};
    const uint64_t id = sequence.fetch_add(1U, std::memory_order_relaxed);
    return root / (std::string(slot) + std::string(suffix) + "." + std::to_string(id) + ".tmp");
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

bool SyncDirectory(const std::filesystem::path& root) {
#if defined(_WIN32)
    (void)root;
    return true;
#else
    const int fd = ::open(root.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) return false;
    const int result = ::fsync(fd);
    ::close(fd);
    return result == 0;
#endif
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
    const std::filesystem::path tempPath = MakeTempPath(root, slot, "");
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

    if (!Flush(root, slot, error) || !SyncDirectory(root)) {
        if (error == AtomicSaveFileError::None) error = AtomicSaveFileError::WriteFailure;
        return false;
    }

    error = AtomicSaveFileError::None;
    return true;
}

bool AtomicSaveFile::Flush(const std::filesystem::path& root, std::string_view slot, AtomicSaveFileError& error) {
    if (!ValidSlot(slot)) {
        error = AtomicSaveFileError::InvalidSlot;
        return false;
    }

    const std::filesystem::path path = root / (std::string(slot) + ".sav");
#if defined(_WIN32)
    HANDLE handle = CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        error = AtomicSaveFileError::OpenWrite;
        return false;
    }
    const BOOL ok = FlushFileBuffers(handle);
    CloseHandle(handle);
    if (!ok) {
        error = AtomicSaveFileError::WriteFailure;
        return false;
    }
#else
    const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        error = AtomicSaveFileError::OpenWrite;
        return false;
    }
    const int result = ::fsync(fd);
    ::close(fd);
    if (result != 0) {
        error = AtomicSaveFileError::WriteFailure;
        return false;
    }
#endif

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
    const std::filesystem::path tempPath = MakeTempPath(root, slot, ".bak");
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
    if (!SyncDirectory(root)) {
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
