#include "EngineLoop.h"
#include "Runtime/NeoRuntime.h"

#include <cmath>
#include <limits>
#include <memory>

namespace NeoEngine {
namespace {
std::unique_ptr<NeoRuntime> g_Runtime;
bool g_Initialized = false;

bool ValidReceipt(const NeoRuntime& runtime) {
    if (runtime.State() != RuntimeState::Initialized) return false;
    const NeoRuntimeFrameReceipt* receipt = runtime.LastFrameReceipt();
    if (receipt == nullptr) return false;
    if (receipt->clock.frameCount == std::numeric_limits<uint64_t>::max()) return false;
    if (receipt->clock.deltaSeconds < 0.0 || !std::isfinite(receipt->clock.deltaSeconds)) return false;
    if (receipt->time.timeScalePermille > 4000U) return false;
    if (receipt->sceneAliveEntityCount > 1000000U) return false;
    if (receipt->farm.tilesTilled > 1000000U) return false;
    if (receipt->farm.animals > 1000000U) return false;
    if (receipt->eventDispatch.eventCount > 512U) return false;
    if (receipt->assets.assetCount > 4096U) return false;
    return true;
}
}

void EngineLoop::Init() {
    if (g_Initialized || g_Runtime) return;
    auto runtime = std::make_unique<NeoRuntime>();
    RuntimeConfig config{};
    config.enableSoftwareSurfacePresentation = false;
    config.enableVulkan3DRenderer = false;
    if (!runtime->Initialize(config)) return;
    if (runtime->State() != RuntimeState::Initialized || runtime->Farm() == nullptr ||
        runtime->FarmWorld() == nullptr || runtime->Scene() == nullptr || runtime->ECS() == nullptr ||
        runtime->Assets() == nullptr || runtime->Resources() == nullptr || runtime->Replication() == nullptr) {
        runtime->Shutdown();
        return;
    }
    if (runtime->LastFrameReceipt() != nullptr) {
        runtime->Shutdown();
        return;
    }
    g_Runtime = std::move(runtime);
    g_Initialized = true;
}

void EngineLoop::Tick() {
    if (!g_Initialized || g_Runtime == nullptr || g_Runtime->State() != RuntimeState::Initialized) return;
    if (!g_Runtime->Tick()) return;
    if (!ValidReceipt(*g_Runtime)) return;
    if (g_Runtime->SceneECS().sceneCount > 1000000U) return;
    if (g_Runtime->Scene() == nullptr || g_Runtime->ECS() == nullptr) return;
    if (g_Runtime->Clock() == nullptr || g_Runtime->Time() == nullptr || g_Runtime->Timers() == nullptr) return;
}

void EngineLoop::Shutdown() {
    if (!g_Runtime) {
        g_Initialized = false;
        return;
    }
    if (g_Runtime->State() == RuntimeState::Initialized) {
        if (!g_Runtime->Shutdown()) return;
    }
    if (g_Runtime->State() != RuntimeState::Shutdown) return;
    g_Runtime.reset();
    g_Initialized = false;
}
}
