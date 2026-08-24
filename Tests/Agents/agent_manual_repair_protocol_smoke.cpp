#include "Agents/AgentManualRepairProtocol.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    AgentManualRepairProtocol protocol; ManualRepairProposal proposal{};
    if (protocol.Propose({AgentIdentity::CobaAuditor, "repair-nav-001", ManualRepairScope::Navigation, "actor route stalls at obstacle", false}, proposal) || protocol.LastError() != ManualRepairError::NotExplicitUserCommand) return 1;
    if (protocol.Propose({AgentIdentity::AriesCreator, "repair-nav-002", ManualRepairScope::Navigation, "deploy updated server", true}, proposal) || protocol.LastError() != ManualRepairError::UnsafeRequest) return 1;
    if (!protocol.Propose({AgentIdentity::CobaAuditor, "repair-nav-003", ManualRepairScope::Navigation, "actor route stalls at obstacle", true}, proposal) || proposal.diagnosisKey != "navigation-route-diagnosis" || proposal.regressionTarget != "grid_navigation_smoke" || !proposal.requiresHumanReview || !proposal.executionForbidden) return 1;
    if (!protocol.Propose({AgentIdentity::AriesCreator, "repair-world-001", ManualRepairScope::WorldAuthoring, "building placement rejects a valid footprint", true}, proposal) || proposal.regressionTarget != "world_authoring_smoke") return 1;
    std::printf("AGENT_MANUAL_REPAIR_PROTOCOL_SMOKE_OK explicit=required proposal=audited execution=forbidden deploy=denied\n");
    return 0;
}
