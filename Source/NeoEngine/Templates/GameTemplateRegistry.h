#pragma once
#include <span>
#include <string_view>

namespace NeoEngine {
enum class TemplateReadiness : unsigned char { CatalogOnly, ExecutableRuntime };
struct GameTemplateDefinition { std::string_view key; std::string_view genre; TemplateReadiness readiness; bool requiresServerAuthority; };
class GameTemplateRegistry {
public:
    static std::span<const GameTemplateDefinition> All();
    static const GameTemplateDefinition* Find(std::string_view key);
    static bool CanIssueExecutablePlan(std::string_view key);
};
} // namespace NeoEngine
