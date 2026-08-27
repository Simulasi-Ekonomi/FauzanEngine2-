#include "FarmCommerceCheckpointFile.h"

#include "FarmCommerceCheckpoint.h"
#include "FarmCommerceEntitlementLedger.h"
#include "FarmWorldTool.h"
#include "Runtime/AtomicSaveFile.h"

#include <vector>

namespace NeoEngine {

bool FarmCommerceCheckpointFile::Save(const std::filesystem::path& root, std::string_view slot, const FarmWorldTool& world,
                                      const FarmCommerceEntitlementLedger& ledger, FarmCommerceCheckpointFileError& error) {
    std::vector<uint8_t> bytes;
    if (!FarmCommerceCheckpoint::Save(world, ledger, bytes)) {
        error = FarmCommerceCheckpointFileError::CheckpointRejected;
        return false;
    }
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Write(root, slot, bytes, storageError)) {
        error = FarmCommerceCheckpointFileError::StorageWriteRejected;
        return false;
    }
    error = FarmCommerceCheckpointFileError::None;
    return true;
}

bool FarmCommerceCheckpointFile::Load(const std::filesystem::path& root, std::string_view slot, FarmWorldTool& world,
                                      FarmCommerceEntitlementLedger& ledger, FarmCommerceCheckpointFileError& error) {
    std::vector<uint8_t> bytes;
    AtomicSaveFileError storageError = AtomicSaveFileError::None;
    if (!AtomicSaveFile::Read(root, slot, bytes, storageError)) {
        error = FarmCommerceCheckpointFileError::StorageReadRejected;
        return false;
    }
    if (!FarmCommerceCheckpoint::Load(bytes, world, ledger)) {
        error = FarmCommerceCheckpointFileError::CheckpointRejected;
        return false;
    }
    error = FarmCommerceCheckpointFileError::None;
    return true;
}

} // namespace NeoEngine
