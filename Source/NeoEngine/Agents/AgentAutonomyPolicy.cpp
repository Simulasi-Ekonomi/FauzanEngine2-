#include "AgentAutonomyPolicy.h"

#include <algorithm>
#include <cctype>

namespace NeoEngine {
AgentAutonomyDecision AgentAutonomyPolicy::Evaluate(const AgentAutonomyRequest& request) const {
    const bool validId = request.requestId.size() >= 8 && request.requestId.size() <= 96 && std::all_of(request.requestId.begin(), request.requestId.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; });
    if (!validId) { reason_ = "invalid_request_id"; return AgentAutonomyDecision::Rejected; }
    switch (request.operation) {
        case AgentAutonomyOperation::Diagnose: reason_ = "diagnosis_dry_run_only"; return AgentAutonomyDecision::DryRunOnly;
        case AgentAutonomyOperation::DraftRepairPlan: reason_ = "repair_plan_dry_run_only"; return AgentAutonomyDecision::DryRunOnly;
        case AgentAutonomyOperation::ProposeLearningEvaluation: reason_ = "learning_proposal_only"; return AgentAutonomyDecision::DryRunOnly;
        case AgentAutonomyOperation::RunRegression:
            if (!request.evidenceVerified || !request.humanApproved) { reason_ = "verified_evidence_and_human_approval_required"; return AgentAutonomyDecision::ApprovalRequired; }
            reason_ = "bounded_regression_authorized"; return AgentAutonomyDecision::TestAuthorized;
        case AgentAutonomyOperation::ApplySourceRepair: reason_ = "source_mutation_rejected"; return AgentAutonomyDecision::Rejected;
        case AgentAutonomyOperation::MutateRuntime: reason_ = "runtime_mutation_rejected"; return AgentAutonomyDecision::Rejected;
        case AgentAutonomyOperation::DeployArtifact: reason_ = "deployment_rejected"; return AgentAutonomyDecision::Rejected;
        case AgentAutonomyOperation::PublishRelease: reason_ = "publication_rejected"; return AgentAutonomyDecision::Rejected;
        case AgentAutonomyOperation::MutateAuthority: reason_ = "authority_mutation_rejected"; return AgentAutonomyDecision::Rejected;
    }
    reason_ = "unknown_operation";
    return AgentAutonomyDecision::Rejected;
}
} // namespace NeoEngine
