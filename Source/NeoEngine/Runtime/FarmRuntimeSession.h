#pragma once

#include "FarmActionPanelController.h"
#include "FarmPlayerInputBridge.h"
#include "FarmSpriteRenderAdapter.h"
#include "InputState.h"
#include "RuntimeTimeSystem.h"
#include "Systems/CurriculumSystem.h"
#include "Systems/FarmSystem.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
class AssetRegistry;
class FarmWorldTool;
class SoftwareRenderer;
class TextureStagingStore;
class FarmRuntimeHud;

enum class FarmRuntimeSessionError : uint8_t { None, NotInitialized, InvalidFrameTicks, InputRejected, WorldTickRejected, RenderRejected, CheckpointEncodeFailed, CheckpointDecodeFailed, WorldCheckpointEncodeFailed, WorldCheckpointDecodeFailed, HudRejected, HudInputRejected, TimeRejected, CurriculumRejected };
struct FarmRuntimeInventorySnapshot { uint32_t wheatSeeds = 0U; uint32_t wheatProduce = 0U; };
enum class FarmOnboardingStep : uint8_t { Till, Plant, Water, Harvest, Complete };
struct FarmOnboardingReceipt { FarmOnboardingStep nextStep = FarmOnboardingStep::Till; bool complete = false; FarmError lastError = FarmError::None; };
struct FarmRuntimeFrameReceipt { uint64_t frame = 0U; uint64_t framebufferHash = 0U; FarmTelemetrySnapshot telemetry{}; FarmRuntimeInventorySnapshot inventory{}; FarmPlayerInputReceipt input{}; FarmActionAvailability availability{}; RuntimeTimeSnapshot time{}; CurriculumProgressReceipt curriculum{}; FarmOnboardingReceipt onboarding{}; };
struct FarmRuntimeHudReceipt { uint64_t frame = 0U; uint64_t worldFramebufferHash = 0U; uint64_t hudFramebufferHash = 0U; FarmTelemetrySnapshot telemetry{}; FarmRuntimeInventorySnapshot inventory{}; FarmPlayerInputReceipt input{}; FarmActionAvailability availability{}; };

// Explicit host-side lifecycle only. It owns neither simulation authority nor
// asset registry; it orchestrates existing bounded components for one frame.
class FarmRuntimeSession {
public:
    static constexpr const char* kWorldCheckpointKind = "farm-runtime-world";
    static constexpr const char* kProgressCheckpointKind = "farm-runtime-progress";
    bool Initialize(FarmSystem& farm, FarmWorldTool& world, const FarmSpriteAssetSet& assets, const AssetRegistry& registry, TextureStagingStore& textures, SoftwareRenderer& renderer, RuntimeTimeSystem* time = nullptr, CurriculumSystem* curriculum = nullptr);
    bool Frame(InputState& input, uint32_t simulationTicks = 1);
    bool SaveCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes);
    bool RestoreCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision);
    bool SaveWorldCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes);
    bool RestoreWorldCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision);
    bool SaveProgressCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes);
    bool RestoreProgressCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision);
    bool DrawHud(FarmRuntimeHud& hud, FarmRuntimeHudReceipt& receipt);
    bool RouteHudPointer(FarmRuntimeHud& hud, float x, float y, UiPointerPhase phase, FarmActionPanelReceipt& receipt);
    bool RouteHudKeyboard(FarmRuntimeHud& hud, UiKeyboardKey key, FarmActionPanelReceipt& receipt);
    FarmPlayerInputBridge& InputBridge() { return inputBridge_; }
    RuntimeTimeSystem* Time() { return time_; }
    const RuntimeTimeSystem* Time() const { return time_; }
    const std::vector<RuntimeTimeEvent>& LastTimeEvents() const { return lastTimeEvents_; }
    CurriculumSystem* Curriculum() { return curriculum_; }
    const CurriculumSystem* Curriculum() const { return curriculum_; }
    const std::vector<CurriculumEvent>& LastCurriculumEvents() const { return lastCurriculumEvents_; }
    [[nodiscard]] uint64_t FrameCount() const { return frameCount_; }
    [[nodiscard]] FarmRuntimeFrameReceipt LastFrameReceipt() const { return lastReceipt_; }
    [[nodiscard]] FarmRuntimeSessionError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }
private:
    bool Fail(FarmRuntimeSessionError error);
    FarmSystem* farm_ = nullptr;
    FarmWorldTool* world_ = nullptr;
    const FarmSpriteAssetSet* assets_ = nullptr;
    const AssetRegistry* registry_ = nullptr;
    TextureStagingStore* textures_ = nullptr;
    SoftwareRenderer* renderer_ = nullptr;
    RuntimeTimeSystem* time_ = nullptr;
    CurriculumSystem* curriculum_ = nullptr;
    FarmPlayerInputBridge inputBridge_{};
    FarmSpriteRenderAdapter rendererBridge_{};
    uint64_t frameCount_ = 0;
    FarmRuntimeFrameReceipt lastReceipt_{};
    FarmRuntimeSessionError lastError_ = FarmRuntimeSessionError::NotInitialized;
    std::vector<RuntimeTimeEvent> lastTimeEvents_{};
    std::vector<CurriculumEvent> lastCurriculumEvents_{};
    bool initialized_ = false;
};
} // namespace NeoEngine
