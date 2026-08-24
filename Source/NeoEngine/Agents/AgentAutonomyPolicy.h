#pragma once

#include "AgentCommandGateway.h"

#include <cstdint>
#include <string>

namespace NeoEngine {

enum class AgentAutonomyOperation : uint8_t { Diagnose, DraftRepairPlan, ProposeLearningEvaluation, RunRegression, ApplySourceRepair, MutateRuntime, DeployArtifact, PublishRelease, MutateAuthority };
enum class AgentAutonomyDecision : uint8_t { DryRunOnly, ApprovalRequired, TestAuthorized, Rejected };
struct AgentAutonomyRequest { AgentIdentity agent = AgentIdentity::CobaAuditor; AgentAutonomyOperation operation = AgentAutonomyOperation::Diagnose; std::string requestId; bool evidenceVerified = false; bool humanApproved = false; };

class AgentAutonomyPolicy {
public:
    AgentAutonomyDecision Evaluate(const AgentAutonomyRequest& request) const;
    [[nodiscard]] const char* Reason() const { return reason_; }
private:
    mutable const char* reason_ = "uninitialized";
};

} // namespace NeoEngine
