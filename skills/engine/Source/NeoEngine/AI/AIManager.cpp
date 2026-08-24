#include "AIManager.h"

namespace NeoEngine {

static AIManager* g_AIManager = nullptr;

AIManager& AIManager::GetInstance() {
    if (!g_AIManager) g_AIManager = new AIManager();
    return *g_AIManager;
}

AIManager::AIManager()
    : hermes(std::make_unique<HermesEngineIntegration>()),
      gemma4(std::make_unique<Gemma4Integration>()),
      ruflo(std::make_unique<RufloIntegration>()),
      opencode(std::make_unique<OpenCodeIntegration>()),
      initialized(false) {}

AIManager::~AIManager() { ShutdownAll(); }

bool AIManager::InitializeAll() {
    if (initialized) return true;
    bool h = hermes->Initialize();
    bool g = gemma4->Initialize();
    bool r = ruflo->Initialize();
    bool o = opencode->Initialize();
    initialized = h && g && r && o;
    return initialized;
}

void AIManager::ShutdownAll() {
    hermes->Shutdown();
    gemma4->Shutdown();
    ruflo->Shutdown();
    opencode->Shutdown();
    initialized = false;
}

HermesEngineIntegration& AIManager::GetHermes() { return *hermes; }
Gemma4Integration& AIManager::GetGemma4() { return *gemma4; }
RufloIntegration& AIManager::GetRuflo() { return *ruflo; }
OpenCodeIntegration& AIManager::GetOpenCode() { return *opencode; }

bool AIManager::IsAllReady() const {
    return hermes->IsReady() && gemma4->IsReady() && ruflo->IsReady() && opencode->IsReady();
}

std::string AIManager::GetStatusReport() const {
    std::string r = "AI Status:\n";
    r += "Hermes: " + std::string(hermes->IsReady()?"OK":"FAIL") + "\n";
    r += "Gemma4: " + std::string(gemma4->IsReady()?"OK":"FAIL") + "\n";
    r += "Ruflo: " + std::string(ruflo->IsReady()?"OK":"FAIL") + "\n";
    r += "OpenCode: " + std::string(opencode->IsReady()?"OK":"FAIL") + "\n";
    return r;
}

std::string AIManager::GenerateGameNarrative(const std::string& ctx) {
    if (!initialized) return "";
    return hermes->GenerateText(ctx).text;
}

std::string AIManager::GenerateGameCode(const std::string& feature) {
    if (!initialized) return "";
    return opencode->GenerateFromDescription(feature).code;
}

ExecutionResult AIManager::ExecuteGeneratedCode(const std::string& code, const std::string& lang) {
    if (!initialized) return {1, "", "Not initialized", 0, false};
    return ruflo->ExecuteCode(code, lang);
}

std::vector<float> AIManager::GetTextEmbeddings(const std::string& text) {
    if (!initialized) return {};
    return gemma4->GetEmbeddings(text);
}

} // namespace NeoEngine
