#include "AgentManualRepairProtocol.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

namespace NeoEngine {
bool AgentManualRepairProtocol::Fail(ManualRepairError error) { lastError_ = error; return false; }
bool AgentManualRepairProtocol::ValidId(const std::string& value) const { return value.size() >= 8 && value.size() <= 96 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; }); }
bool AgentManualRepairProtocol::IsUnsafe(const std::string& symptom) const { std::string lower; lower.reserve(symptom.size()); for (unsigned char c : symptom) lower.push_back(static_cast<char>(std::tolower(c))); static constexpr std::array<std::string_view, 7> banned{"deploy", "publish", "credential", "password", "economy", "ban", "session"}; return std::any_of(banned.begin(), banned.end(), [&lower](std::string_view word) { return lower.find(word) != std::string::npos; }); }
bool AgentManualRepairProtocol::Propose(const ManualRepairRequest& request, ManualRepairProposal& proposal) {
    proposal = {};
    if (!request.explicitUserCommand) return Fail(ManualRepairError::NotExplicitUserCommand);
    if (!ValidId(request.commandId)) return Fail(ManualRepairError::InvalidCommandId);
    if (request.scope > ManualRepairScope::RegressionOnly) return Fail(ManualRepairError::InvalidScope);
    if (request.symptom.empty() || request.symptom.size() > 240 || IsUnsafe(request.symptom)) return Fail(ManualRepairError::UnsafeRequest);
    switch (request.scope) {
        case ManualRepairScope::Navigation: proposal = {"navigation-route-diagnosis", "systems-grid-navigation-only", "grid_navigation_smoke", true, true}; break;
        case ManualRepairScope::WorldAuthoring: proposal = {"world-authoring-diagnosis", "systems-world-authoring-only", "world_authoring_smoke", true, true}; break;
        case ManualRepairScope::RuntimeIntegration: proposal = {"runtime-integration-diagnosis", "runtime-neo-runtime-only", "runtime_world_vertical_slice_smoke", true, true}; break;
        case ManualRepairScope::RegressionOnly: proposal = {"regression-diagnosis", "no-source-change", "xpbd_regression", true, true}; break;
    }
    lastError_ = ManualRepairError::None; return true;
}
} // namespace NeoEngine
