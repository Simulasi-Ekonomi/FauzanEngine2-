#include "Runtime/NeoRuntime.h"
#include "Systems/FarmProgressCheckpointFile.h"

#include <cstdio>
#include <filesystem>
#include <vector>

int main() {
    using namespace NeoEngine;
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "neoengine-r2-progress-file-smoke";
    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);

    RuntimeConfig config{};
    config.farmWidth = 4U;
    config.farmHeight = 4U;
    config.farmNpcCount = 2U;
    config.enableFarmCurriculum = true;
    config.farmBalance.maxEnergy = 8U;
    config.farmBalance.energyRegenPerTick = 1U;
    config.farmBalance.growthTicks = {6U, 8U, 10U};

    NeoRuntime runtime;
    if (!runtime.Initialize(config) || runtime.Farm() == nullptr || runtime.FarmWorld() == nullptr) return 1;
    std::vector<uint8_t> checkpoint;
    if (!runtime.SaveFarmProgressCheckpoint(19U, checkpoint) || checkpoint.empty()) return 2;
    const std::vector<uint8_t> savedFarm = runtime.Farm()->Serialize();
    const std::vector<uint8_t> savedWorld = runtime.FarmWorld()->Serialize();

    FarmProgressCheckpointFileError fileError = FarmProgressCheckpointFileError::None;
    if (!FarmProgressCheckpointFile::Save(root, "production", checkpoint, fileError) || fileError != FarmProgressCheckpointFileError::None) return 3;
    if (!runtime.Farm()->Till(0U, 0U) || runtime.Farm()->TileStateAt(0U, 0U) != FarmTileState::Tilled) return 4;

    std::vector<uint8_t> loaded;
    if (!FarmProgressCheckpointFile::Load(root, "production", loaded, fileError) || loaded != checkpoint) return 5;
    uint64_t restoredRevision = 0U;
    if (!runtime.RestoreFarmProgressCheckpoint(loaded, restoredRevision) || restoredRevision != 19U ||
        runtime.Farm()->Serialize() != savedFarm || runtime.FarmWorld()->Serialize() != savedWorld) return 6;

    std::vector<uint8_t> missing;
    if (FarmProgressCheckpointFile::Load(root, "missing", missing, fileError) || fileError != FarmProgressCheckpointFileError::StorageReadRejected || !missing.empty()) return 7;

    std::vector<uint8_t> corrupt = checkpoint;
    corrupt.back() ^= 0x01U;
    if (!FarmProgressCheckpointFile::Save(root, "corrupt", corrupt, fileError) || fileError != FarmProgressCheckpointFileError::None) return 8;
    if (runtime.Farm()->Till(1U, 0U) == false) return 10;
    const std::vector<uint8_t> preservedFarm = runtime.Farm()->Serialize();
    const std::vector<uint8_t> preservedWorld = runtime.FarmWorld()->Serialize();
    if (!FarmProgressCheckpointFile::Load(root, "corrupt", loaded, fileError) || fileError != FarmProgressCheckpointFileError::None ||
        runtime.RestoreFarmProgressCheckpoint(loaded, restoredRevision) || runtime.LastError() != RuntimeError::CheckpointDecodeFailed ||
        runtime.Farm()->Serialize() != preservedFarm || runtime.FarmWorld()->Serialize() != preservedWorld) return 11;

    runtime.Shutdown();
    std::filesystem::remove_all(root, cleanupError);
    std::printf("NEO_RUNTIME_FARM_PROGRESS_FILE_SMOKE_OK save=1 restore=1 missing_preserved=1 corrupt_preserved=1\n");
    return 0;
}
