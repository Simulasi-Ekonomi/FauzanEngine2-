#include "Agents/AgentAutonomyPolicy.h"

#include <cstdio>
#include <string_view>

int main() {
    using namespace NeoEngine;
    AgentAutonomyPolicy policy;
    const AgentAutonomyRequest diagnose{AgentIdentity::CobaAuditor, AgentAutonomyOperation::Diagnose, "coba-diag-001", false, false};
    const AgentAutonomyRequest repair{AgentIdentity::AriesCreator, AgentAutonomyOperation::DraftRepairPlan, "aries-repair-001", false, false};
    const AgentAutonomyRequest learning{AgentIdentity::CobaAuditor, AgentAutonomyOperation::ProposeLearningEvaluation, "coba-learn-001", false, false};
    const AgentAutonomyRequest regressionPending{AgentIdentity::CobaAuditor, AgentAutonomyOperation::RunRegression, "coba-test-001", false, false};
    const AgentAutonomyRequest regressionApproved{AgentIdentity::CobaAuditor, AgentAutonomyOperation::RunRegression, "coba-test-002", true, true};
    const AgentAutonomyRequest deploy{AgentIdentity::AriesCreator, AgentAutonomyOperation::DeployArtifact, "aries-deploy-001", true, true};
    const AgentAutonomyRequest authority{AgentIdentity::AriesCreator, AgentAutonomyOperation::MutateAuthority, "aries-authority-001", true, true};
    const bool ok = policy.Evaluate(diagnose) == AgentAutonomyDecision::DryRunOnly && policy.Evaluate(repair) == AgentAutonomyDecision::DryRunOnly && policy.Evaluate(learning) == AgentAutonomyDecision::DryRunOnly && policy.Evaluate(regressionPending) == AgentAutonomyDecision::ApprovalRequired && policy.Evaluate(regressionApproved) == AgentAutonomyDecision::TestAuthorized && policy.Evaluate(deploy) == AgentAutonomyDecision::Rejected && policy.Evaluate(authority) == AgentAutonomyDecision::Rejected;
    if (!ok) return 1;
    std::printf("AGENT_AUTONOMY_POLICY_SMOKE_OK repair=dry-run learning=proposal deploy=rejected authority=rejected\n");
    return 0;
}
