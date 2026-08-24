#pragma once

#include <cstdint>

namespace NeoEngine {

class InputState;

enum class SdlInputBridgeError : uint8_t { None, InvalidConfiguration, VideoInitializationFailed, WindowCreationFailed, NotInitialized, InputQueueRejected };

class SdlInputBridge {
public:
    SdlInputBridge() = default;
    ~SdlInputBridge();

    SdlInputBridge(const SdlInputBridge&) = delete;
    SdlInputBridge& operator=(const SdlInputBridge&) = delete;

    bool InitializeHidden(uint16_t width = 64, uint16_t height = 64);
    bool PumpFrame(InputState& input);
    void Reset();

    [[nodiscard]] bool IsReady() const { return window_ != nullptr; }
    [[nodiscard]] bool QuitRequested() const { return quitRequested_; }
    [[nodiscard]] SdlInputBridgeError LastError() const { return lastError_; }

private:
    void* window_ = nullptr;
    bool videoInitialized_ = false;
    bool quitRequested_ = false;
    SdlInputBridgeError lastError_ = SdlInputBridgeError::None;
};

} // namespace NeoEngine
