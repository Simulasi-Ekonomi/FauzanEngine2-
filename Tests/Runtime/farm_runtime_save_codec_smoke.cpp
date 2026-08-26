#include "Runtime/FarmRuntimeSaveCodec.h"
#include "Systems/FarmSystem.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    FarmSystem source(4U, 4U, 100); if (!source.Till(1U, 1U) || !source.Tick(3U)) return 1;
    std::vector<uint8_t> bytes; FarmRuntimeSaveError error = FarmRuntimeSaveError::None;
    if (!FarmRuntimeSaveCodec::Encode(source, 7U, bytes, error) || error != FarmRuntimeSaveError::None || bytes.empty()) return 1;
    FarmSystem target(2U, 2U, 10); uint64_t revision = 1U;
    if (!FarmRuntimeSaveCodec::Decode(target, bytes, revision, error) || error != FarmRuntimeSaveError::None || revision != 7U || target.Width()!=4U || target.Height()!=4U || target.TileStateAt(1U,1U)!=FarmTileState::Tilled || target.SimulationTick()!=3U) return 1;
    const std::vector<uint8_t> preservedState = target.Serialize(); const uint64_t preservedRevision = revision;
    RuntimeSaveEnvelope wrong{"other-world", 8U, source.Serialize()}; RuntimePersistenceError persistenceError = RuntimePersistenceError::None; std::vector<uint8_t> wrongBytes;
    if (!RuntimeSaveCodec::Serialize(wrong, wrongBytes, persistenceError) || FarmRuntimeSaveCodec::Decode(target, wrongBytes, revision, error) || error != FarmRuntimeSaveError::WrongKind || target.Serialize()!=preservedState || revision!=preservedRevision) return 1;
    std::vector<uint8_t> malformed = bytes; malformed.back() ^= 0x01U;
    if (FarmRuntimeSaveCodec::Decode(target, malformed, revision, error) || error != FarmRuntimeSaveError::EnvelopeFailed || target.Serialize()!=preservedState || revision!=preservedRevision) return 1;
    if (FarmRuntimeSaveCodec::Encode(source, 0U, bytes, error) || error != FarmRuntimeSaveError::InvalidRevision) return 1;
    std::printf("FARM_RUNTIME_SAVE_CODEC_SMOKE_OK revision=%llu atomic=1 farm=1\n", static_cast<unsigned long long>(revision)); return 0;
}
