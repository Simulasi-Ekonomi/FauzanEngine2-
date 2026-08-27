#include "FarmAuthorityCheckpointFile.h"

#include "FarmAuthoritativeService.h"
#include "FarmAuthorityCheckpoint.h"
#include "FarmWorldTool.h"
#include "Runtime/AtomicSaveFile.h"

#include <vector>

namespace NeoEngine {

bool FarmAuthorityCheckpointFile::Save(const std::filesystem::path& root, std::string_view slot, const FarmWorldTool& world,
                                       const FarmAuthoritativeService& service, FarmAuthorityCheckpointFileError& error) {
    std::vector<uint8_t> bytes;
    if (!FarmAuthorityCheckpoint::Save(world, service, bytes)) {
        error = FarmAuthorityCheckpointFileError::CheckpointRejected;
        return false;
    }
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Write(root, slot, bytes, storageError)) {
        error = FarmAuthorityCheckpointFileError::StorageWriteRejected;
        return false;
    }
    error = FarmAuthorityCheckpointFileError::None;
    return true;
}

bool FarmAuthorityCheckpointFile::Load(const std::filesystem::path& root, std::string_view slot, FarmWorldTool& world,
                                       FarmAuthoritativeService& service, FarmAuthorityCheckpointFileError& error) {
    std::vector<uint8_t> bytes;
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Read(root, slot, bytes, storageError)) {
        error = FarmAuthorityCheckpointFileError::StorageReadRejected;
        return false;
    }
    if (!FarmAuthorityCheckpoint::Load(bytes, world, service)) {
        error = FarmAuthorityCheckpointFileError::CheckpointRejected;
        return false;
    }
    error = FarmAuthorityCheckpointFileError::None;
    return true;
}

} // namespace NeoEngine
