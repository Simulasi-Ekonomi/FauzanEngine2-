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
        lastError = Error::InitializationFailed;
        return false;
    }

    hermes = std::move(newHermes);
    gemma4 = std::move(newGemma4);
    ruflo = std::move(newRuflo);
    opencode = std::move(newOpenCode);
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
    if (timeAccumulator > std::numeric_limits<float>::max() / 2.0f) { lastError = Error::InvalidDeltaTime; return; }
    timeAccumulator += DeltaTime;
    if (timeAccumulator < 1.0f) return;
    timeAccumulator = std::fmod(timeAccumulator, 1.0f);
}

bool AIManager::IsReady() const {
    return initialized && ((hermes && hermes->IsReady()) || (gemma4 && gemma4->IsReady()) ||
                           (ruflo && ruflo->IsReady()) || (opencode && opencode->IsReady()));
}

std::string AIManager::Think(const std::string& context) {
    if (context.size() > 16U * 1024U * 1024U) { lastError = Error::InvalidContext; return {}; }
    if (context.empty()) { lastError = Error::InvalidContext; return {}; }
    if (!IsReady()) { lastError = Error::BackendUnavailable; return {}; }
    if (hermes && hermes->IsReady()) {
        const HermesResponse response = hermes->GenerateText(context);
        if (!response.text.empty()) return response.text;
    }
    if (gemma4 && gemma4->IsReady()) {
        const Gemma4Response response = gemma4->GenerateText(context);
        if (!response.generatedText.empty()) return response.generatedText;
    }
    lastError = Error::BackendUnavailable;
    return {};
}

std::string AIManager::PlanAction(const std::string& state) {
    if (state.size() > 16U * 1024U * 1024U) { lastError = Error::InvalidContext; return {}; }
    if (state.empty()) { lastError = Error::InvalidContext; return {}; }
    return Think("Plan an action for the following game state:\n" + state);
}

}
