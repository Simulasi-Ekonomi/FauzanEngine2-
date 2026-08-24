#include "AgentCommandGateway.h"

#include <cctype>

namespace NeoEngine {
bool AgentCommandGateway::IsSafeIdentifier(const std::string& value, size_t minLength, size_t maxLength) {
    if (value.size() < minLength || value.size() > maxLength) return false;
    for (unsigned char c : value) if (!std::isalnum(c) && c != '-' && c != '_' && c != '.') return false;
    return true;
}
bool AgentCommandGateway::IsAuthorityCommand(AgentCommandKind kind) { return kind == AgentCommandKind::MutateRuntime || kind == AgentCommandKind::MutateEconomy; }
bool AgentCommandGateway::IsAllowedFor(AgentIdentity agent, AgentCommandKind kind) {
    if (agent == AgentIdentity::CobaAuditor) return kind == AgentCommandKind::AuditRuntime || kind == AgentCommandKind::RequestTest || kind == AgentCommandKind::RequestRollback;
    return kind == AgentCommandKind::CreateGameTemplate || kind == AgentCommandKind::RequestBuild || kind == AgentCommandKind::RequestTest || kind == AgentCommandKind::RequestRollback;
}
AgentReceipt AgentCommandGateway::Evaluate(const AgentCommand& command) const {
    if (!IsSafeIdentifier(command.requestId, 8, 96) || !IsSafeIdentifier(command.target, 1, 96)) return {AgentDecision::Rejected, "invalid_typed_identifier", {}};
    if (IsAuthorityCommand(command.kind)) return {AgentDecision::Rejected, "runtime_and_economy_authority_forbidden", {}};
    if (!IsAllowedFor(command.agent, command.kind)) return {AgentDecision::Rejected, "agent_capability_forbidden", {}};
    if (command.dryRun) return {AgentDecision::DryRunPrepared, "dry_run_only", "plan-" + command.requestId};
    return {AgentDecision::ApprovalRequired, "human_approval_and_build_test_evidence_required", "plan-" + command.requestId};
}
AgentReceipt AgentCommandGateway::ApproveAndIssue(const AgentCommand& command, const ApprovalEvidence& evidence) {
    const AgentReceipt evaluation = Evaluate(command);
    if (evaluation.decision != AgentDecision::ApprovalRequired) return {AgentDecision::Rejected, "only_non_dry_run_plan_can_be_issued", {}};
    if (!evidence.buildPassed || !evidence.testsPassed || !IsSafeIdentifier(evidence.evidenceRef, 8, 128)) return {AgentDecision::Rejected, "insufficient_verification_evidence", {}};
    if (!m_IssuedRequests.insert(command.requestId).second) return {AgentDecision::Rejected, "request_already_issued", {}};
    return {AgentDecision::PlanIssued, "issued_for_external_supervised_executor", evaluation.planRef};
}
} // namespace NeoEngine
