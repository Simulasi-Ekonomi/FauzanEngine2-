#pragma once

#include "FarmPlayerInputBridge.h"
#include "FarmSpriteRenderAdapter.h"
#include "InputState.h"
#include "Systems/FarmSystem.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
class AssetRegistry;
class FarmWorldTool;
class SoftwareRenderer;
class TextureStagingStore;

enum class FarmRuntimeSessionError : uint8_t { None, NotInitialized, InvalidFrameTicks, InputRejected, WorldTickRejected, RenderRejected, CheckpointEncodeFailed, CheckpointDecodeFailed };
struct FarmRuntimeFrameReceipt { uint64_t frame = 0U; uint64_t framebufferHash = 0U; FarmTelemetrySnapshot telemetry{}; };

// Explicit host-side lifecycle only. It owns neither simulation authority nor
// asset registry; it orchestrates existing bounded components for one frame.
class FarmRuntimeSession {
public:
    bool Initialize(FarmSystem& farm, FarmWorldTool& world, const FarmSpriteAssetSet& assets, const AssetRegistry& registry, TextureStagingStore& textures, SoftwareRenderer& renderer);
    bool Frame(InputState& input, uint32_t simulationTicks = 1);
    bool SaveCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes);
    bool RestoreCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision);
    FarmPlayerInputBridge& InputBridge() { return inputBridge_; }
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
    FarmPlayerInputBridge inputBridge_{};
    FarmSpriteRenderAdapter rendererBridge_{};
    uint64_t frameCount_ = 0;
    FarmRuntimeFrameReceipt lastReceipt_{};
    FarmRuntimeSessionError lastError_ = FarmRuntimeSessionError::NotInitialized;
    bool initialized_ = false;
};
} // namespace NeoEngine
