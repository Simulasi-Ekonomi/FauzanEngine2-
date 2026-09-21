#pragma once
#include "HermesIntegration.h"
#include "Gemma4Integration.h"
#include "RufloIntegration.h"
#include "OpenCodeIntegration.h"
#include <cstdint>
#include <memory>
#include <string>

namespace NeoEngine {
class AIManager {
public:
    enum class Error : uint8_t { None, AlreadyInitialized, InitializationFailed, InvalidDeltaTime, InvalidContext, BackendUnavailable };
    static AIManager& Get();
    bool Initialize(); void Shutdown(); void Update(float DeltaTime); bool IsReady() const;
    [[nodiscard]] Error LastError() const noexcept { return lastError; }
    std::string Think(const std::string& context); std::string PlanAction(const std::string& state);
private:
    AIManager(); ~AIManager();
    std::unique_ptr<HermesIntegration> hermes; std::unique_ptr<Gemma4Integration> gemma4;
    std::unique_ptr<RufloIntegration> ruflo; std::unique_ptr<OpenCodeIntegration> opencode;
    bool initialized = false; float timeAccumulator = 0.0f; Error lastError = Error::None;
};
}