#include "FarmProgressCheckpointFile.h"

#include "Runtime/AtomicSaveFile.h"

#include <algorithm>

namespace NeoEngine {
namespace {
FarmProgressCheckpointFileError MapWriteError(AtomicSaveFileError error) {
    return error == AtomicSaveFileError::PayloadLimit ? FarmProgressCheckpointFileError::PayloadLimit : FarmProgressCheckpointFileError::StorageWriteRejected;
}
FarmProgressCheckpointFileError MapReadError(AtomicSaveFileError error) {
    return error == AtomicSaveFileError::PayloadLimit ? FarmProgressCheckpointFileError::PayloadLimit : FarmProgressCheckpointFileError::StorageReadRejected;
}
}

bool FarmProgressCheckpointFile::Save(const std::filesystem::path& root, std::string_view slot, std::span<const uint8_t> checkpoint,
                                      FarmProgressCheckpointFileError& error) {
    error = FarmProgressCheckpointFileError::None;
    if (checkpoint.empty()) { error = FarmProgressCheckpointFileError::EmptyCheckpoint; return false; }
    if (checkpoint.size() > kMaxBytes) { error = FarmProgressCheckpointFileError::PayloadLimit; return false; }
    std::vector<uint8_t> owned(checkpoint.begin(), checkpoint.end());
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Write(root, slot, owned, storageError)) { error = MapWriteError(storageError); return false; }
    return true;
}

bool FarmProgressCheckpointFile::Load(const std::filesystem::path& root, std::string_view slot, std::vector<uint8_t>& checkpoint,
                                      FarmProgressCheckpointFileError& error) {
    error = FarmProgressCheckpointFileError::None;
    checkpoint.clear();
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Read(root, slot, checkpoint, storageError)) { error = MapReadError(storageError); checkpoint.clear(); return false; }
    if (checkpoint.empty()) { error = FarmProgressCheckpointFileError::EmptyCheckpoint; checkpoint.clear(); return false; }
    if (checkpoint.size() > kMaxBytes) { error = FarmProgressCheckpointFileError::PayloadLimit; checkpoint.clear(); return false; }
    return true;
}

} // namespace NeoEngine
