#include "Runtime/FarmRuntimeSession.h"

#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeSaveCodec.h"
#include "Runtime/FarmRuntimeHud.h"
#include "Runtime/RuntimePersistence.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/TextureStaging.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"

namespace NeoEngine {
bool FarmRuntimeSession::Fail(FarmRuntimeSessionError error) { lastError_ = error; return false; }
bool FarmRuntimeSession::Initialize(FarmSystem& farm, FarmWorldTool& world, const FarmSpriteAssetSet& assets, const AssetRegistry& registry, TextureStagingStore& textures, SoftwareRenderer& renderer) {
    initialized_ = false; frameCount_ = 0; lastReceipt_ = {};
    if (!farm.IsReady() || !world.IsReady() || renderer.Width() == 0U || renderer.Height() == 0U || !inputBridge_.Initialize()) return Fail(FarmRuntimeSessionError::NotInitialized);
    farm_ = &farm; world_ = &world; assets_ = &assets; registry_ = &registry; textures_ = &textures; renderer_ = &renderer; initialized_ = true; lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::Frame(InputState& input, uint32_t simulationTicks) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    if (simulationTicks == 0U) return Fail(FarmRuntimeSessionError::InvalidFrameTicks);
    input.BeginFrame();
    if (!inputBridge_.Step(input, *world_)) return Fail(FarmRuntimeSessionError::InputRejected);
    if (!world_->Tick(simulationTicks)) return Fail(FarmRuntimeSessionError::WorldTickRejected);
    if (!rendererBridge_.RenderWorld(*farm_, *world_, *assets_, *registry_, *textures_, *renderer_)) return Fail(FarmRuntimeSessionError::RenderRejected);
    const uint64_t candidateFrame = frameCount_ + 1U; const FarmRuntimeFrameReceipt candidateReceipt{candidateFrame, renderer_->FrameHash(), farm_->Snapshot()};
    ++frameCount_; lastReceipt_ = candidateReceipt; lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::SaveCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    FarmRuntimeSaveError error = FarmRuntimeSaveError::None;
    if (!FarmRuntimeSaveCodec::Encode(*farm_, revision, bytes, error)) return Fail(FarmRuntimeSessionError::CheckpointEncodeFailed);
    lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::RestoreCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    FarmRuntimeSaveError error = FarmRuntimeSaveError::None;
    if (!FarmRuntimeSaveCodec::Decode(*farm_, bytes, revision, error)) return Fail(FarmRuntimeSessionError::CheckpointDecodeFailed);
    lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::SaveWorldCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    const std::vector<uint8_t> payload = world_->Serialize(); RuntimePersistenceError error = RuntimePersistenceError::None; std::vector<uint8_t> candidate;
    if (revision == 0U || payload.empty() || !RuntimeSaveCodec::Serialize({kWorldCheckpointKind, revision, payload}, candidate, error)) return Fail(FarmRuntimeSessionError::WorldCheckpointEncodeFailed);
    bytes = std::move(candidate); lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::RestoreWorldCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    RuntimeSaveEnvelope envelope{}; RuntimePersistenceError error = RuntimePersistenceError::None;
    if (!RuntimeSaveCodec::Deserialize(bytes, envelope, error) || envelope.kind != kWorldCheckpointKind || envelope.revision == 0U || !world_->Deserialize(envelope.payload)) return Fail(FarmRuntimeSessionError::WorldCheckpointDecodeFailed);
    revision = envelope.revision; lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::DrawHud(FarmRuntimeHud& hud, FarmRuntimeHudReceipt& receipt) {
    if (!initialized_ || lastReceipt_.frame == 0U || lastReceipt_.framebufferHash == 0U || !hud.Draw(lastReceipt_, *renderer_)) return Fail(FarmRuntimeSessionError::HudRejected);
    const FarmRuntimeHudReceipt candidate{lastReceipt_.frame, lastReceipt_.framebufferHash, renderer_->FrameHash(), lastReceipt_.telemetry};
    receipt = candidate; lastError_ = FarmRuntimeSessionError::None; return true;
}
} // namespace NeoEngine
