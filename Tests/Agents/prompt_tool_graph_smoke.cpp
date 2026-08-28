#include "Agents/PromptToolGraph.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    PromptToolPlan dryRun{"prompt-plan-001", "Create a Farm template plan with bounded validation.", true, {"7d6f6b9a3c4e5f617d6f6b9a3c4e5f61"}, {
        {"audit-node", {AgentIdentity::CobaAuditor, AgentCommandKind::AuditRuntime, "coba-audit-001", "farm-runtime", true}, {}},
        {"template-node", {AgentIdentity::AriesCreator, AgentCommandKind::CreateGameTemplate, "aries-create-001", "farm-tool", true}, {"audit-node"}},
        {"test-node", {AgentIdentity::CobaAuditor, AgentCommandKind::RequestTest, "coba-test-001", "farm-smoke", true}, {"template-node"}},
    }};
    AgentCommandGateway gateway;
    PromptToolGraph graph;
    std::vector<PromptToolNodeReceipt> receipts;
    if (!graph.EvaluateDryRun(dryRun, gateway, receipts) || receipts.size() != 3 || graph.LastError() != PromptToolGraphError::None) return 1;

    PromptToolPlan unordered = dryRun;
    unordered.nodes[0].dependencies = {"test-node"};
    if (graph.EvaluateDryRun(unordered, gateway, receipts) || graph.LastError() != PromptToolGraphError::DependencyCycle) return 1;

    PromptToolPlan forbidden = dryRun;
    forbidden.nodes[1].command.kind = AgentCommandKind::MutateEconomy;
    if (graph.EvaluateDryRun(forbidden, gateway, receipts) || graph.LastError() != PromptToolGraphError::GatewayRejected) return 1;

    PromptToolPlan issue = dryRun;
    issue.dryRun = false;
    for (PromptToolNode& node : issue.nodes) node.command.dryRun = false;
    const std::vector<ApprovalEvidence> evidence{{true, true, "evidence-audit-001"}, {true, true, "evidence-template-001"}, {true, true, "evidence-test-001"}};
    AgentCommandGateway issuer;
    if (!graph.ApproveAndIssue(issue, issuer, evidence, receipts) || receipts.size() != 3 || graph.LastError() != PromptToolGraphError::None) return 1;

    PromptToolExecutionReceipt preserved{"untouched", 77U, true};
    std::vector<PromptToolNodeReceipt> tampered = receipts;
    tampered[1].nodeId = "tampered-node";
    if (graph.ExecuteIssued(issue, issuer, tampered, preserved) || graph.LastError() != PromptToolGraphError::ReceiptMismatch || preserved.promptId != "untouched" || preserved.executedNodeCount != 77U || !preserved.externalSideEffectsApplied) return 1;

    AgentCommandGateway notIssued;
    PromptToolGraph directGraph;
    PromptToolExecutionReceipt directExecution{};
    if (directGraph.ExecuteIssued(issue, notIssued, receipts, directExecution) || directGraph.LastError() != PromptToolGraphError::PlanNotIssued) return 1;

    PromptToolExecutionReceipt execution{};
    if (!graph.ExecuteIssued(issue, issuer, receipts, execution) || graph.LastError() != PromptToolGraphError::None || execution.promptId != issue.promptId || execution.executedNodeCount != 3U || execution.externalSideEffectsApplied) return 1;
    PromptToolExecutionReceipt duplicateExecution{"preserved", 88U, true};
    if (graph.ExecuteIssued(issue, issuer, receipts, duplicateExecution) || graph.LastError() != PromptToolGraphError::PlanAlreadyConsumed || duplicateExecution.promptId != "preserved" || duplicateExecution.executedNodeCount != 88U || !duplicateExecution.externalSideEffectsApplied) return 1;

    std::printf("PROMPT_TOOL_GRAPH_SMOKE_OK nodes=3 dryRun=1 approval=typed execution=once authority=denied side_effects=0\n");
    return 0;
}
