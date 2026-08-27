#include "Runtime/NeoRuntime.h"
#include "Runtime/RuntimePersistence.h"

#include <cstdio>

using namespace NeoEngine;

namespace {
AuthorityCommand FarmCommand(const char* id, uint64_t sequence, uint64_t clientTick, uint16_t x, uint16_t z) {
    return {"runtime-farm-player", "runtime-farm-session", id, "farm.till", sequence, clientTick,
            {static_cast<uint8_t>(x & 0xFFU), static_cast<uint8_t>(x >> 8U), static_cast<uint8_t>(z & 0xFFU), static_cast<uint8_t>(z >> 8U)}};
}

bool EncodeEnvelope(RuntimeSaveEnvelope envelope, std::vector<uint8_t>& bytes) {
    RuntimePersistenceError error = RuntimePersistenceError::None;
    return RuntimeSaveCodec::Serialize(envelope, bytes, error);
}
} // namespace

int main() {
    RuntimeConfig config{};
    config.farmWidth = 4U;
    config.farmHeight = 4U;
    config.farmNpcCount = 4U;
    NeoRuntime runtime;
    std::vector<uint8_t> checkpoint;
    bool ok = !runtime.SaveFarmProgressCheckpoint(1U, checkpoint) && runtime.LastError() == RuntimeError::CheckpointEncodeFailed;
    ok = ok && runtime.Initialize(config) && runtime.FarmAuthority() != nullptr && runtime.FarmWorld() != nullptr && runtime.Time() != nullptr && runtime.Scene() != nullptr;
    ok = ok && runtime.FarmAuthority()->Submit(FarmCommand("checkpoint-cmd-001", 1U, 1U, 0U, 0U), 1U).Accepted();
    ok = ok && runtime.FarmWorld()->SetCharacterState({1U, 1U, 2U}) && runtime.Tick();
    std::vector<uint8_t> savedWorld = runtime.FarmWorld()->Serialize();
    std::vector<uint8_t> savedTime;
    ok = ok && runtime.Time()->Serialize(savedTime) && runtime.SaveFarmProgressCheckpoint(77U, checkpoint) && !checkpoint.empty();
    const uint64_t savedAuthorityRevision = runtime.FarmAuthority()->Revision();

    ok = ok && runtime.FarmWorld()->SetCharacterState({2U, 2U, 3U}) && runtime.FarmWorld()->PlayerTill(2U, 2U) && runtime.Tick();
    uint64_t restoredRevision = 0U;
    ok = ok && runtime.FarmWorld()->Serialize() != savedWorld && runtime.RestoreFarmProgressCheckpoint(checkpoint, restoredRevision) && restoredRevision == 77U && runtime.FarmWorld()->Serialize() == savedWorld && runtime.FarmAuthority()->Revision() == savedAuthorityRevision;
    std::vector<uint8_t> restoredTime;
    ok = ok && runtime.Time()->Serialize(restoredTime) && restoredTime == savedTime && runtime.LastFrameReceipt() == nullptr && runtime.LastFarmRenderReceipt() == nullptr;
    ok = ok && runtime.FarmAuthority()->Submit(FarmCommand("checkpoint-cmd-002", 2U, 2U, 1U, 0U), 2U).Accepted() && runtime.FarmAuthority()->Revision() == savedAuthorityRevision + 1U;

    const std::vector<uint8_t> preservedWorld = runtime.FarmWorld()->Serialize();
    std::vector<uint8_t> preservedTime;
    ok = ok && runtime.Time()->Serialize(preservedTime);
    const uint64_t preservedAuthorityRevision = runtime.FarmAuthority()->Revision();
    const std::vector<uint8_t> preservedScene = runtime.Scene()->Serialize();
    std::vector<uint8_t> checksumCorrupt = checkpoint;
    checksumCorrupt.back() ^= 0x01U;
    uint64_t ignoredRevision = 0U;
    ok = ok && !runtime.RestoreFarmProgressCheckpoint(checksumCorrupt, ignoredRevision) && runtime.LastError() == RuntimeError::CheckpointDecodeFailed && runtime.FarmWorld()->Serialize() == preservedWorld && runtime.FarmAuthority()->Revision() == preservedAuthorityRevision;
    std::vector<uint8_t> trailing = checkpoint;
    trailing.push_back(0U);
    ok = ok && !runtime.RestoreFarmProgressCheckpoint(trailing, ignoredRevision) && runtime.FarmWorld()->Serialize() == preservedWorld && runtime.Time()->Serialize(restoredTime) && restoredTime == preservedTime;
    RuntimeSaveEnvelope decoded{};
    RuntimePersistenceError persistenceError = RuntimePersistenceError::None;
    std::vector<uint8_t> wrongKind;
    ok = ok && RuntimeSaveCodec::Deserialize(checkpoint, decoded, persistenceError) && EncodeEnvelope({"wrong-kind", decoded.revision, decoded.payload}, wrongKind) && !runtime.RestoreFarmProgressCheckpoint(wrongKind, ignoredRevision) && runtime.Scene()->Serialize() == preservedScene;
    std::vector<uint8_t> corruptPayload = decoded.payload;
    corruptPayload[4] ^= 0x01U;
    std::vector<uint8_t> validEnvelopeWithCorruptWorld;
    ok = ok && EncodeEnvelope({decoded.kind, decoded.revision, corruptPayload}, validEnvelopeWithCorruptWorld) && !runtime.RestoreFarmProgressCheckpoint(validEnvelopeWithCorruptWorld, ignoredRevision) && runtime.FarmWorld()->Serialize() == preservedWorld;

    uint64_t permitId = 0U;
    uint32_t buildingId = 0U;
    ok = ok && runtime.FarmWorld()->SetGovernmentPolicy(FarmGovernmentPolicy::ConstructionPermits, true) && runtime.FarmWorld()->IssueBuildingPermit(FarmBuildingType::Barn, permitId) && runtime.FarmWorld()->PlaceBuilding(permitId, 3U, 3U, buildingId);
    const std::vector<uint8_t> topologyWorld = runtime.FarmWorld()->Serialize();
    const std::vector<uint8_t> topologyScene = runtime.Scene()->Serialize();
    ok = ok && !runtime.RestoreFarmProgressCheckpoint(checkpoint, ignoredRevision) && runtime.LastError() == RuntimeError::CheckpointDecodeFailed && runtime.FarmWorld()->Serialize() == topologyWorld && runtime.Scene()->Serialize() == topologyScene;
    ok = ok && runtime.Shutdown();
    if (!ok) {
        std::fprintf(stderr, "NEO_RUNTIME_CHECKPOINT_SMOKE_FAIL\n");
        return 1;
    }
    std::printf("NEO_RUNTIME_CHECKPOINT_SMOKE_OK progress=atomic topology=preserved authority=rebound\n");
    return 0;
}
