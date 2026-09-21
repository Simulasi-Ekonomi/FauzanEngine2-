#include "AIManager.h"
#include <iostream>

namespace NeoEngine {

AIManager& AIManager::Get() {
    static AIManager instance;
    return instance;
}

AIManager::AIManager() : initialized(false), timeAccumulator(0.0f) {}

AIManager::~AIManager() {}

bool AIManager::Initialize() {
    hermes = std::make_unique<HermesIntegration>();
    gemma4 = std::make_unique<Gemma4Integration>();
    ruflo = std::make_unique<RufloIntegration>();
    opencode = std::make_unique<OpenCodeIntegration>();

    initialized = true;

    std::cout << "[AI] Initialized\n";
    return true;
}

void AIManager::Shutdown() {
    hermes.reset();
    gemma4.reset();
    ruflo.reset();
    opencode.reset();

    initialized = false;
}

void AIManager::Update(float DeltaTime) {
    if (!initialized) return;

    timeAccumulator += DeltaTime;

    if (timeAccumulator > 1.0f) {
        std::cout << "[AI] Tick\n";

        std::string decision = Think("game_state");
        std::cout << "Decision: " << decision << std::endl;

        timeAccumulator = 0.0f;
    }
}

bool AIManager::IsReady() const {
    return initialized;
}

std::string AIManager::Think(const std::string& context) {
    if (hermes) {
        return "AI thinking on: " + context;
    }
    return "No AI";
}

std::string AIManager::PlanAction(const std::string& state) {
    return "Action for: " + state;
}

}
