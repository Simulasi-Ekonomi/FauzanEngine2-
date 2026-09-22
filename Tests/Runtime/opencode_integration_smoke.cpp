#include "AI/OpenCodeIntegration.h"
#include <cassert>
int main() {
    NeoEngine::OpenCodeIntegration integration;
    assert(integration.GetSupportedLanguages().size() >= 6);
    auto component = integration.GenerateFromTemplate("component", {{"name", "SmokeComponent"}});
    assert(component.code.find("SmokeComponent") != std::string::npos);
    assert(integration.ValidateCode(component));
    auto system = integration.GenerateFromTemplate("system", {{"name", "SmokeSystem"}});
    assert(integration.ValidateCode(system));
    auto unknown = integration.GenerateFromTemplate("unknown", {});
    assert(unknown.code.empty());
    NeoEngine::GeneratedCode bad{"cpp", "void GeneratedFeature() {}", "invalid", 1};
    assert(!integration.ValidateCode(bad));
    return 0;
}
