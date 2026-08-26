#pragma once

#include "FarmRuntimeSession.h"

#include <cstdint>

namespace NeoEngine {
class SoftwareRenderer;
enum class FarmRuntimeHudError : uint8_t { None, InvalidReceipt, SetupFailed, DrawFailed };
class FarmRuntimeHud {
public:
    bool Draw(const FarmRuntimeFrameReceipt& receipt, SoftwareRenderer& renderer);
    [[nodiscard]] FarmRuntimeHudError LastError() const { return lastError_; }
private:
    FarmRuntimeHudError lastError_ = FarmRuntimeHudError::None;
};
} // namespace NeoEngine
