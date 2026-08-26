#include "Runtime/FarmRuntimeSaveCodec.h"

#include "Systems/FarmSystem.h"

namespace NeoEngine {
bool FarmRuntimeSaveCodec::Encode(const FarmSystem& farm, uint64_t revision, std::vector<uint8_t>& bytes, FarmRuntimeSaveError& error) {
    if (!farm.IsReady() || revision == 0U) { error = FarmRuntimeSaveError::InvalidRevision; return false; }
    std::vector<uint8_t> candidate; RuntimePersistenceError persistenceError = RuntimePersistenceError::None;
    if (!RuntimeSaveCodec::Serialize({kKind, revision, farm.Serialize()}, candidate, persistenceError)) { error = FarmRuntimeSaveError::EnvelopeFailed; return false; }
    bytes = std::move(candidate); error = FarmRuntimeSaveError::None; return true;
}
bool FarmRuntimeSaveCodec::Decode(FarmSystem& farm, const std::vector<uint8_t>& bytes, uint64_t& revision, FarmRuntimeSaveError& error) {
    RuntimeSaveEnvelope envelope{}; RuntimePersistenceError persistenceError = RuntimePersistenceError::None;
    if (!RuntimeSaveCodec::Deserialize(bytes, envelope, persistenceError)) { error = FarmRuntimeSaveError::EnvelopeFailed; return false; }
    if (envelope.kind != kKind || envelope.revision == 0U) { error = envelope.kind != kKind ? FarmRuntimeSaveError::WrongKind : FarmRuntimeSaveError::InvalidRevision; return false; }
    FarmSystem candidate(1U, 1U, 0); if (!candidate.Deserialize(envelope.payload)) { error = FarmRuntimeSaveError::FarmDeserializeFailed; return false; }
    if (!farm.Deserialize(envelope.payload)) { error = FarmRuntimeSaveError::FarmDeserializeFailed; return false; }
    revision = envelope.revision; error = FarmRuntimeSaveError::None; return true;
}
} // namespace NeoEngine
