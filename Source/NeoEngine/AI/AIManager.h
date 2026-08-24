#pragma once
#include "HermesIntegration.h"
#include "Gemma4Integration.h"
#include "RufloIntegration.h"
#include "OpenCodeIntegration.h"
#include <memory>
#include <string>
#include <vector>

namespace NeoEngine {

class AIManager {
public:
    static AIManager& Get();

    bool Initialize();
    void Shutdown();

    void Update(float DeltaTime);

    bool IsReady() const;

    // === GAME AI ===
    std::string Think(const std::string& context);
    std::string PlanAction(const std::string& state);

private:
    AIManager();
    ~AIManager();

    std::unique_ptr<HermesIntegration> hermes;
    std::unique_ptr<Gemma4Integration> gemma4;
    std::unique_ptr<RufloIntegration> ruflo;
    std::unique_ptr<OpenCodeIntegration> opencode;

    bool initialized;
    float timeAccumulator;
};

}
