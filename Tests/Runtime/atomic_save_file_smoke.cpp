#include "Runtime/AtomicSaveFile.h"
#include "Runtime/RuntimePersistence.h"

#include <cstdio>
#include <filesystem>

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
    std::filesystem::remove_all(root, cleanup);
    std::printf("ATOMIC_SAVE_FILE_SMOKE_OK write=1 read=1 codec=1 slotValidation=1 backup=1 restore=1 missingRestorePreserved=1\n");
    return 0;
}
