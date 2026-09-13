#include "Runtime/AtomicSaveFile.h"
#include "Runtime/RuntimePersistence.h"

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

int main() {
    using namespace NeoEngine;
    const std::filesystem::path root = "/tmp/fauzan-engine-atomic-save-smoke";
    std::error_code cleanup;
    std::filesystem::remove_all(root, cleanup);
    RuntimeSettingsStore settings;
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> loaded;
    AtomicSaveFileError error = AtomicSaveFileError::None;
    if (!settings.Set("audio.volume", "0.5") || !settings.Serialize(encoded) ||
        !AtomicSaveFile::Write(root, "slot-1", encoded, error) ||
        !AtomicSaveFile::Read(root, "slot-1", loaded, error) || loaded != encoded) {
        return 1;
    }

    if (!AtomicSaveFile::Backup(root, "slot-1", error) || error != AtomicSaveFileError::None) return 1;
    const std::vector<uint8_t> updated{0x42U, 0x43U, 0x44U};
    if (!AtomicSaveFile::Write(root, "slot-1", updated, error) ||
        !AtomicSaveFile::RestoreBackup(root, "slot-1", error) ||
        !AtomicSaveFile::Read(root, "slot-1", loaded, error) || loaded != encoded) {
        return 1;
    }

    if (!AtomicSaveFile::Write(root, "slot-1", updated, error) ||
        AtomicSaveFile::RestoreBackup(root, "missing", error) || error != AtomicSaveFileError::RestoreFailure ||
        !AtomicSaveFile::Read(root, "slot-1", loaded, error) || loaded != updated) {
        return 1;
    }

    RuntimeSettingsStore restored;
    if (!restored.Deserialize(encoded) || !restored.Find("audio.volume") || *restored.Find("audio.volume") != "0.5" ||
        AtomicSaveFile::Write(root, "../escape", encoded, error) || error != AtomicSaveFileError::InvalidSlot ||
        AtomicSaveFile::Read(root, "missing", loaded, error) || error != AtomicSaveFileError::Missing) {
        return 1;
    }

    constexpr unsigned kConcurrentWriters = 16U;
    std::vector<std::vector<uint8_t>> payloads;
    payloads.reserve(kConcurrentWriters);
    for (unsigned index = 0U; index < kConcurrentWriters; ++index) {
        payloads.push_back({static_cast<uint8_t>(index), 0xA5U, static_cast<uint8_t>(index ^ 0x5AU)});
    }
    std::atomic<unsigned> failures{0U};
    std::vector<std::thread> writers;
    writers.reserve(kConcurrentWriters);
    for (unsigned index = 0U; index < kConcurrentWriters; ++index) {
        writers.emplace_back([&, index]() {
            AtomicSaveFileError localError = AtomicSaveFileError::None;
            if (!AtomicSaveFile::Write(root, "concurrent", payloads[index], localError) || localError != AtomicSaveFileError::None) {
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }
    for (std::thread& writer : writers) writer.join();
    if (failures.load(std::memory_order_relaxed) != 0U ||
        !AtomicSaveFile::Read(root, "concurrent", loaded, error) || error != AtomicSaveFileError::None) {
        return 1;
    }
    bool matchesWriterPayload = false;
    for (const std::vector<uint8_t>& payload : payloads) if (loaded == payload) { matchesWriterPayload = true; break; }
    if (!matchesWriterPayload) return 1;

    bool temporaryFileFound = false;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        const std::string name = entry.path().filename().string();
        if (name.find(".tmp") != std::string::npos) { temporaryFileFound = true; break; }
    }
    if (temporaryFileFound) return 1;

    std::filesystem::remove_all(root, cleanup);
    std::printf("ATOMIC_SAVE_FILE_SMOKE_OK write=1 read=1 codec=1 slotValidation=1 backup=1 restore=1 missingRestorePreserved=1 concurrentWrites=1\n");
    return 0;
}
