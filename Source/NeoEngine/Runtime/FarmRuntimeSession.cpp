#include "Runtime/FarmRuntimeSession.h"

#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeSaveCodec.h"
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
} // namespace NeoEngine
