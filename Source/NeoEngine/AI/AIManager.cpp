#include "AIManager.h"
#include <cmath>
#include <cstdint>
#include <limits>

namespace NeoEngine {

AIManager& AIManager::Get() {
    static AIManager instance;
    return instance;
}

AIManager::AIManager() : initialized(false), timeAccumulator(0.0f), lastError(Error::None) {}
AIManager::~AIManager() { Shutdown(); }

bool AIManager::Initialize() {
    if (initialized) { lastError = Error::AlreadyInitialized; return false; }

    auto newHermes = std::make_unique<HermesIntegration>();
    auto newGemma4 = std::make_unique<Gemma4Integration>();
    auto newRuflo = std::make_unique<RufloIntegration>();
    auto newOpenCode = std::make_unique<OpenCodeIntegration>();
    if (!newHermes || !newGemma4 || !newRuflo || !newOpenCode) { lastError = Error::InitializationFailed; return false; }

    const bool hermesReady = newHermes->Initialize();
    const bool gemmaReady = newGemma4->Initialize();
    const bool rufloReady = newRuflo->Initialize();
    const bool openCodeReady = newOpenCode->Initialize();
    if (!hermesReady && !gemmaReady && !rufloReady && !openCodeReady) {
        newHermes->Shutdown(); newGemma4->Shutdown(); newRuflo->Shutdown(); newOpenCode->Shutdown();
        lastError = Error::InitializationFailed;
        return false;
    }
    if ((hermesReady && !newHermes->IsReady()) || (gemmaReady && !newGemma4->IsReady()) ||
        (rufloReady && !newRuflo->IsReady()) || (openCodeReady && !newOpenCode->IsReady())) {
        newHermes->Shutdown(); newGemma4->Shutdown(); newRuflo->Shutdown(); newOpenCode->Shutdown();
        lastError = Error::InitializationFailed;
        return false;
    }

    hermes = std::move(newHermes);
    gemma4 = std::move(newGemma4);
    ruflo = std::move(newRuflo);
    opencode = std::move(newOpenCode);
    if (!((hermes && hermes->IsReady()) || (gemma4 && gemma4->IsReady()) ||
          (ruflo && ruflo->IsReady()) || (opencode && opencode->IsReady()))) {
        Shutdown();
        lastError = Error::InitializationFailed;
        return false;
    }
    initialized = true;
    timeAccumulator = 0.0f;
    lastError = Error::None;
    return true;
}

void AIManager::Shutdown() {
    if (hermes) hermes->Shutdown();
    if (gemma4) gemma4->Shutdown();
    if (ruflo) ruflo->Shutdown();
    if (opencode) opencode->Shutdown();
    hermes.reset();
    gemma4.reset();
    ruflo.reset();
    opencode.reset();
    initialized = false;
    timeAccumulator = 0.0f;
    lastError = Error::None;
}

void AIManager::Update(float DeltaTime) {
    if (!initialized) return;
    if (!std::isfinite(DeltaTime) || !std::isfinite(timeAccumulator) || DeltaTime < 0.0f || DeltaTime > 0.25f) {
        lastError = Error::InvalidDeltaTime;
        return;
    }
    if (DeltaTime > 1.0e6f - timeAccumulator) { lastError = Error::InvalidDeltaTime; return; }
    if (!std::isfinite(timeAccumulator) || timeAccumulator > std::numeric_limits<float>::max() / 2.0f) { lastError = Error::InvalidDeltaTime; return; }
    const float nextAccumulator = timeAccumulator + DeltaTime;
    if (!std::isfinite(nextAccumulator) || nextAccumulator < timeAccumulator) { lastError = Error::InvalidDeltaTime; return; }
    timeAccumulator = nextAccumulator;
    if (timeAccumulator < 1.0f) return;
    timeAccumulator = std::fmod(timeAccumulator, 1.0f);
    if (!std::isfinite(timeAccumulator) || timeAccumulator < 0.0f || timeAccumulator >= 1.0f) {
        lastError = Error::InvalidDeltaTime;
        return;
    }
}

bool AIManager::IsReady() const noexcept {
    if (!initialized) return false;
    return (hermes && hermes->IsReady()) || (gemma4 && gemma4->IsReady()) ||
           (ruflo && ruflo->IsReady()) || (opencode && opencode->IsReady());
}

std::string AIManager::Think(const std::string& context) {
    constexpr std::size_t kMaxContext = 16U * 1024U * 1024U;
    if (context.size() > kMaxContext || context.size() == std::string::npos || context.find('\0') != std::string::npos) { lastError = Error::InvalidContext; return {}; }
    if (context.empty()) { lastError = Error::InvalidContext; return {}; }
    if (!IsReady()) { lastError = Error::BackendUnavailable; return {}; }
    if (hermes && hermes->IsReady()) {
        const HermesResponse response = hermes->GenerateText(context);
        if (response.text.size() <= kMaxContext && !response.text.empty() && response.text.find('\0') == std::string::npos) return response.text;
    }
    if (gemma4 && gemma4->IsReady()) {
        const Gemma4Response response = gemma4->GenerateText(context);
        if (response.generatedText.size() <= kMaxContext && !response.generatedText.empty() && response.generatedText.find('\0') == std::string::npos) return response.generatedText;
    }
    if (ruflo && ruflo->IsReady()) {
        const ExecutionResult response = ruflo->ExecuteCode(context, "text");
        if (response.success && response.stdout.size() <= kMaxContext && !response.stdout.empty() && response.stdout.find('\0') == std::string::npos &&
            std::isfinite(response.executionTime) && response.executionTime >= 0.0f && response.executionTime <= 86400.0f) return response.stdout;
    }
    if (opencode && opencode->IsReady()) {
        const GeneratedCode response = opencode->GenerateFromDescription(context);
        if (!response.code.empty() && response.code.size() <= kMaxContext && response.code.find('\0') == std::string::npos) return response.code;
    }
    lastError = Error::BackendUnavailable;
    return {};
}

std::string AIManager::PlanAction(const std::string& state) {
    constexpr std::size_t kMaxContext = 16U * 1024U * 1024U;
    if (state.size() > kMaxContext || state.size() == std::string::npos || state.find('\0') != std::string::npos) { lastError = Error::InvalidContext; return {}; }
    if (state.empty()) { lastError = Error::InvalidContext; return {}; }
    constexpr std::size_t kPrefixSize = sizeof("Plan an action for the following game state:\n") - 1U;
    if (state.size() > kMaxContext - kPrefixSize) { lastError = Error::InvalidContext; return {}; }
    const std::string prompt = "Plan an action for the following game state:\n" + state;
    if (prompt.size() > kMaxContext || prompt.find('\0') != std::string::npos) { lastError = Error::InvalidContext; return {}; }
    return Think(prompt);
}

}
