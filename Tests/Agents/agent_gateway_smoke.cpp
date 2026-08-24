#include "Agents/AgentCommandGateway.h"
#include <cstdio>
using namespace NeoEngine;
int main() {
    AgentCommandGateway gateway;
    const AgentCommand audit{AgentIdentity::CobaAuditor, AgentCommandKind::AuditRuntime, "coba-audit-01", "farm-alpha", true};
    const AgentCommand templateDry{AgentIdentity::AriesCreator, AgentCommandKind::CreateGameTemplate, "aries-template-01", "tower-defense", true};
    const AgentCommand templateApply{AgentIdentity::AriesCreator, AgentCommandKind::CreateGameTemplate, "aries-template-02", "tower-defense", false};
    const AgentCommand forbidden{AgentIdentity::AriesCreator, AgentCommandKind::MutateEconomy, "aries-economy-01", "farm-alpha", true};
    bool ok = gateway.Evaluate(audit).decision == AgentDecision::DryRunPrepared && gateway.Evaluate(templateDry).decision == AgentDecision::DryRunPrepared && gateway.Evaluate(templateApply).decision == AgentDecision::ApprovalRequired && gateway.Evaluate(forbidden).decision == AgentDecision::Rejected;
    ok = ok && gateway.ApproveAndIssue(templateApply, {false, true, "evidence-01"}).decision == AgentDecision::Rejected;
    ok = ok && gateway.ApproveAndIssue(templateApply, {true, true, "evidence-verified-01"}).decision == AgentDecision::PlanIssued;
    ok = ok && gateway.ApproveAndIssue(templateApply, {true, true, "evidence-verified-01"}).decision == AgentDecision::Rejected;
    if (!ok) { std::fprintf(stderr, "AGENT_GATEWAY_SMOKE_FAIL\n"); return 1; }
    std::printf("AGENT_GATEWAY_SMOKE_OK authority=denied workflow=typed\n");
    return 0;
}
