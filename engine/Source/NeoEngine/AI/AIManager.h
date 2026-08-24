#pragma once
#include "HermesIntegration.h"
#include "Gemma4Integration.h"
#include "RufloIntegration.h"
#include "OpenCodeIntegration.h"
#include <memory>

namespace NeoEngine {

class AIManager {
public:
    static AIManager& GetInstance();

    bool InitializeAll();
    void ShutdownAll();

    HermesIntegration& GetHermes();
    Gemma4Integration& GetGemma4();
    RufloIntegration& GetRuflo();
    OpenCodeIntegration& GetOpenCode();

    bool IsAllReady() const;
    std::string GetStatusReport() const;

    std::string GenerateGameNarrative(const std::string& context);
    std::string GenerateGameCode(const std::string& gameFeature);
    ExecutionResult ExecuteGeneratedCode(const std::string& code, const std::string& language);
    std::vector<float> GetTextEmbeddings(const std::string& text);

private:
    AIManager();
    ~AIManager();

    std::unique_ptr<HermesIntegration> hermes;
    std::unique_ptr<Gemma4Integration> gemma4;
    std::unique_ptr<RufloIntegration> ruflo;
    std::unique_ptr<OpenCodeIntegration> opencode;

    bool initialized;
};

}
