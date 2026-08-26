#include "Runtime/FarmRuntimeSession.h"

#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeSaveCodec.h"
#include "Runtime/FarmRuntimeHud.h"
#include "Runtime/RuntimePersistence.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/TextureStaging.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"

#include <limits>

namespace NeoEngine {
namespace {
FarmActionAvailability AvailabilityAtPlayer(const FarmSystem& farm, const FarmWorldTool& world) {
    const FarmCharacterState& player = world.Character(); const FarmTileState state = farm.TileStateAt(player.x, player.z);
    return {state == FarmTileState::Empty, state == FarmTileState::Tilled && farm.ItemCount(FarmItem::WheatSeed) != 0U, state == FarmTileState::Growing && !farm.IsWateredAt(player.x, player.z), state == FarmTileState::Harvestable};
}
constexpr size_t kProgressCheckpointMaxBytes = RuntimeSaveCodec::kMaxPayloadBytes;
void AppendU32(std::vector<uint8_t>& bytes, uint32_t value) {
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xFFU));
}
bool ReadU32(const std::vector<uint8_t>& bytes, size_t& offset, uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(uint32_t)) return false;
    value = 0U;
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) value |= static_cast<uint32_t>(bytes[offset + shift / 8U]) << shift;
    offset += sizeof(uint32_t);
    return true;
}
bool AppendBlob(std::vector<uint8_t>& bytes, const std::vector<uint8_t>& blob) {
    if (blob.size() > std::numeric_limits<uint32_t>::max()) return false;
    AppendU32(bytes, static_cast<uint32_t>(blob.size()));
    bytes.insert(bytes.end(), blob.begin(), blob.end());
    return true;
}
bool ReadBlob(const std::vector<uint8_t>& bytes, size_t& offset, std::vector<uint8_t>& blob) {
    uint32_t length = 0U;
    if (!ReadU32(bytes, offset, length) || offset > bytes.size() || bytes.size() - offset < length) return false;
    blob.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
    offset += length;
    return true;
}
}
bool FarmRuntimeSession::Fail(FarmRuntimeSessionError error) { lastError_ = error; return false; }
bool FarmRuntimeSession::Initialize(FarmSystem& farm, FarmWorldTool& world, const FarmSpriteAssetSet& assets, const AssetRegistry& registry, TextureStagingStore& textures, SoftwareRenderer& renderer, RuntimeTimeSystem* time, CurriculumSystem* curriculum) {
    initialized_ = false; frameCount_ = 0; lastReceipt_ = {}; lastTimeEvents_.clear(); lastCurriculumEvents_.clear();
    if (!farm.IsReady() || !world.IsReady() || renderer.Width() == 0U || renderer.Height() == 0U || !inputBridge_.Initialize() || (time != nullptr && !time->IsReady()) || (curriculum != nullptr && !curriculum->IsReady())) return Fail(FarmRuntimeSessionError::NotInitialized);
    farm_ = &farm; world_ = &world; assets_ = &assets; registry_ = &registry; textures_ = &textures; renderer_ = &renderer; time_ = time; curriculum_ = curriculum; initialized_ = true; lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::Frame(InputState& input, uint32_t simulationTicks) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    if (simulationTicks == 0U) return Fail(FarmRuntimeSessionError::InvalidFrameTicks);
    input.BeginFrame();
    uint32_t effectiveSimulationTicks = simulationTicks;
    lastTimeEvents_.clear();
    if (time_ != nullptr && !time_->AdvanceFixedTicks(simulationTicks, lastTimeEvents_, effectiveSimulationTicks)) return Fail(FarmRuntimeSessionError::TimeRejected);
    if (effectiveSimulationTicks != 0U && !inputBridge_.Step(input, *world_)) return Fail(FarmRuntimeSessionError::InputRejected);
    if (effectiveSimulationTicks != 0U && !world_->Tick(effectiveSimulationTicks)) return Fail(FarmRuntimeSessionError::WorldTickRejected);
    if (!rendererBridge_.RenderWorld(*farm_, *world_, *assets_, *registry_, *textures_, *renderer_)) return Fail(FarmRuntimeSessionError::RenderRejected);
    const FarmRuntimeInventorySnapshot inventory{farm_->ItemCount(FarmItem::WheatSeed), farm_->ItemCount(FarmItem::WheatProduce)};
    const RuntimeTimeSnapshot timeSnapshot = time_ == nullptr ? RuntimeTimeSnapshot{} : time_->Snapshot();
    CurriculumProgressReceipt curriculumReceipt{};
    lastCurriculumEvents_.clear();
    if (curriculum_ != nullptr) {
        const CurriculumObservation observation{timeSnapshot, farm_->Snapshot()};
        if (!curriculum_->Evaluate(observation, lastCurriculumEvents_) || !curriculum_->Snapshot(curriculumReceipt)) return Fail(FarmRuntimeSessionError::CurriculumRejected);
    }
    const uint64_t candidateFrame = frameCount_ + 1U; const FarmRuntimeFrameReceipt candidateReceipt{candidateFrame, renderer_->FrameHash(), farm_->Snapshot(), inventory, inputBridge_.LastReceipt(), AvailabilityAtPlayer(*farm_, *world_), timeSnapshot, curriculumReceipt};
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
bool FarmRuntimeSession::SaveProgressCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    if (revision == 0U || time_ == nullptr || curriculum_ == nullptr) return Fail(FarmRuntimeSessionError::CheckpointEncodeFailed);
    const std::vector<uint8_t> farmPayload = farm_->Serialize();
    const std::vector<uint8_t> timePayload = [&]() { std::vector<uint8_t> value; time_->Serialize(value); return value; }();
    const std::vector<uint8_t> curriculumPayload = [&]() { std::vector<uint8_t> value; curriculum_->Serialize(value); return value; }();
    std::vector<uint8_t> payload;
    if (farmPayload.empty() || timePayload.empty() || curriculumPayload.empty() || !AppendBlob(payload, farmPayload) || !AppendBlob(payload, timePayload) || !AppendBlob(payload, curriculumPayload) || payload.size() > kProgressCheckpointMaxBytes) return Fail(FarmRuntimeSessionError::CheckpointEncodeFailed);
    RuntimePersistenceError error = RuntimePersistenceError::None;
    std::vector<uint8_t> candidate;
    if (!RuntimeSaveCodec::Serialize({kProgressCheckpointKind, revision, std::move(payload)}, candidate, error)) return Fail(FarmRuntimeSessionError::CheckpointEncodeFailed);
    bytes = std::move(candidate); lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::RestoreProgressCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision) {
    if (!initialized_) return Fail(FarmRuntimeSessionError::NotInitialized);
    if (time_ == nullptr || curriculum_ == nullptr) return Fail(FarmRuntimeSessionError::CheckpointDecodeFailed);
    RuntimeSaveEnvelope envelope{}; RuntimePersistenceError error = RuntimePersistenceError::None;
    if (!RuntimeSaveCodec::Deserialize(bytes, envelope, error) || envelope.kind != kProgressCheckpointKind || envelope.revision == 0U || envelope.payload.size() > kProgressCheckpointMaxBytes) return Fail(FarmRuntimeSessionError::CheckpointDecodeFailed);
    size_t offset = 0U;
    std::vector<uint8_t> farmPayload, timePayload, curriculumPayload;
    if (!ReadBlob(envelope.payload, offset, farmPayload) || !ReadBlob(envelope.payload, offset, timePayload) || !ReadBlob(envelope.payload, offset, curriculumPayload) || offset != envelope.payload.size()) return Fail(FarmRuntimeSessionError::CheckpointDecodeFailed);
    FarmSystem candidateFarm = *farm_;
    RuntimeTimeSystem candidateTime = *time_;
    CurriculumSystem candidateCurriculum = *curriculum_;
    if (!candidateFarm.Deserialize(farmPayload) || !candidateTime.Deserialize(timePayload) || !candidateCurriculum.Deserialize(curriculumPayload)) return Fail(FarmRuntimeSessionError::CheckpointDecodeFailed);
    *farm_ = std::move(candidateFarm);
    *time_ = std::move(candidateTime);
    *curriculum_ = std::move(candidateCurriculum);
    frameCount_ = 0U;
    lastReceipt_ = {};
    lastTimeEvents_.clear();
    lastCurriculumEvents_.clear();
    revision = envelope.revision;
    lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::DrawHud(FarmRuntimeHud& hud, FarmRuntimeHudReceipt& receipt) {
    if (!initialized_ || lastReceipt_.frame == 0U || lastReceipt_.framebufferHash == 0U) return Fail(FarmRuntimeSessionError::HudRejected);
    hud.SetActionAvailability(lastReceipt_.availability);
    if (!hud.Draw(lastReceipt_, inputBridge_.SelectedAction(), *registry_, *textures_, assets_->harvestableTile, *renderer_)) return Fail(FarmRuntimeSessionError::HudRejected);
    const FarmRuntimeHudReceipt candidate{lastReceipt_.frame, lastReceipt_.framebufferHash, renderer_->FrameHash(), lastReceipt_.telemetry, lastReceipt_.inventory, lastReceipt_.input, lastReceipt_.availability};
    receipt = candidate; lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::RouteHudPointer(FarmRuntimeHud& hud, float x, float y, UiPointerPhase phase, FarmActionPanelReceipt& receipt) {
    if (!initialized_ || !hud.RoutePointer(x, y, phase, inputBridge_, receipt)) return Fail(FarmRuntimeSessionError::HudInputRejected);
    lastError_ = FarmRuntimeSessionError::None; return true;
}
bool FarmRuntimeSession::RouteHudKeyboard(FarmRuntimeHud& hud, UiKeyboardKey key, FarmActionPanelReceipt& receipt) {
    if (!initialized_ || !hud.RouteKeyboard(key, inputBridge_, receipt)) return Fail(FarmRuntimeSessionError::HudInputRejected);
    lastError_ = FarmRuntimeSessionError::None; return true;
}
} // namespace NeoEngine
