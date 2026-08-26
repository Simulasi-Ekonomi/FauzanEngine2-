#pragma once

#include "Runtime/FarmRuntimeSession.h"

#include <cstdint>
#include <string>

namespace NeoEngine {

enum class FarmInteractiveSurfaceDemoError : uint8_t { None, InvalidConfiguration, AssetSetupFailed, RuntimeSetupFailed, HudInputFailed, FrameFailed, PresentFailed, ArtifactWriteFailed };
struct FarmInteractiveSurfaceDemoConfig { uint32_t width = 0U; uint32_t height = 0U; bool hiddenSurface = true; std::string ppmPath{}; };
struct FarmInteractiveSurfaceDemoReceipt { uint64_t frames = 0U; uint64_t presentedFrames = 0U; uint64_t worldFramebufferHash = 0U; uint64_t hudFramebufferHash = 0U; FarmPlayerAction selectedAction = FarmPlayerAction::Till; uint8_t selectedActionMask = 0U; FarmTelemetrySnapshot telemetry{}; FarmRuntimeInventorySnapshot inventory{}; };

// Finite proof harness for the CPU/software Farm interactive path. It owns no
// persistent loop, host, or external gameplay authority.
bool RunFarmInteractiveSurfaceDemo(const FarmInteractiveSurfaceDemoConfig& config, FarmInteractiveSurfaceDemoReceipt& receipt, FarmInteractiveSurfaceDemoError& error);

} // namespace NeoEngine
