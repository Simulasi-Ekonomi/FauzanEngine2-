#include "PromptToolGraph.h"

#include <algorithm>
#include <cctype>

namespace NeoEngine {

bool PromptToolGraph::EvaluateDryRun(const PromptToolPlan& plan, const AgentCommandGateway& gateway, std::vector<PromptToolNodeReceipt>& receipts) {
    receipts.clear();
    if (!Validate(plan, true)) return false;
    receipts.reserve(plan.nodes.size());
    for (const PromptToolNode& node : plan.nodes) {
        const AgentReceipt receipt = gateway.Evaluate(node.command);
        receipts.push_back({node.nodeId, receipt});
        if (receipt.decision != AgentDecision::DryRunPrepared) {
            lastError_ = PromptToolGraphError::GatewayRejected;
            return false;
        }
    }
    lastError_ = PromptToolGraphError::None;
    return true;
}

bool PromptToolGraph::ApproveAndIssue(const PromptToolPlan& plan, AgentCommandGateway& gateway, const std::vector<ApprovalEvidence>& evidence, std::vector<PromptToolNodeReceipt>& receipts) {
    receipts.clear();
    if (!Validate(plan, false) || evidence.size() != plan.nodes.size()) {
        if (lastError_ == PromptToolGraphError::None) lastError_ = PromptToolGraphError::ApprovalEvidenceMissing;
        return false;
    }
    receipts.reserve(plan.nodes.size());
    for (size_t index = 0; index < plan.nodes.size(); ++index) {
        const AgentReceipt receipt = gateway.ApproveAndIssue(plan.nodes[index].command, evidence[index]);
        receipts.push_back({plan.nodes[index].nodeId, receipt});
        if (receipt.decision != AgentDecision::PlanIssued) {
            lastError_ = PromptToolGraphError::GatewayRejected;
            return false;
        }
    }
    lastError_ = PromptToolGraphError::None;
    return true;
}

bool PromptToolGraph::ExecuteIssued(const PromptToolPlan& plan, const AgentCommandGateway& gateway, const std::vector<PromptToolNodeReceipt>& receipts, PromptToolExecutionReceipt& execution) {
    if (!Validate(plan, false)) return false;
    if (consumedPlans_.contains(plan.promptId)) {
        lastError_ = PromptToolGraphError::PlanAlreadyConsumed;
        return false;
    }
    if (consumedPlans_.size() >= kMaxConsumedPlans) {
        lastError_ = PromptToolGraphError::ExecutionCapacity;
        return false;
    }
    if (receipts.size() != plan.nodes.size()) {
        lastError_ = PromptToolGraphError::ReceiptMismatch;
        return false;
    }
    for (size_t index = 0; index < plan.nodes.size(); ++index) {
        const PromptToolNode& node = plan.nodes[index];
        const PromptToolNodeReceipt& nodeReceipt = receipts[index];
        if (nodeReceipt.nodeId != node.nodeId || nodeReceipt.receipt.decision != AgentDecision::PlanIssued) {
            lastError_ = PromptToolGraphError::ReceiptMismatch;
            return false;
        }
        if (!gateway.IsPlanIssued(node.command, nodeReceipt.receipt.planRef)) {
            lastError_ = PromptToolGraphError::PlanNotIssued;
            return false;
        }
    }
    const PromptToolExecutionReceipt candidate{plan.promptId, static_cast<uint16_t>(plan.nodes.size()), false};
    consumedPlans_.insert(plan.promptId);
    execution = candidate;
    lastError_ = PromptToolGraphError::None;
    return true;
}

bool PromptToolGraph::Validate(const PromptToolPlan& plan, bool requireDryRun) {
    lastError_ = PromptToolGraphError::InvalidPlan;
    if (!IsSafeIdentifier(plan.promptId, 8, 96) || plan.summary.empty() || plan.summary.size() > kMaxSummaryLength || plan.nodes.empty() || plan.nodes.size() > kMaxNodes || plan.documentDigests.size() > kMaxDocumentDigests || plan.dryRun != requireDryRun) return false;
    for (const std::string& digest : plan.documentDigests) if (!IsSafeIdentifier(digest, 16, 96)) return false;
    for (size_t index = 0; index < plan.nodes.size(); ++index) {
        const PromptToolNode& node = plan.nodes[index];
        if (!IsSafeIdentifier(node.nodeId, 3, 96) || node.command.dryRun != plan.dryRun || node.dependencies.size() > kMaxDependenciesPerNode) return false;
        for (size_t other = 0; other < index; ++other) if (plan.nodes[other].nodeId == node.nodeId) { lastError_ = PromptToolGraphError::DuplicateNode; return false; }
        for (const std::string& dependency : node.dependencies) {
            if (!IsSafeIdentifier(dependency, 3, 96)) return false;
            const auto found = std::find_if(plan.nodes.begin(), plan.nodes.end(), [&dependency](const PromptToolNode& candidate) { return candidate.nodeId == dependency; });
            if (found == plan.nodes.end()) { lastError_ = PromptToolGraphError::UnknownDependency; return false; }
        }
    }
    if (!IsTopologicallyOrdered(plan)) { lastError_ = PromptToolGraphError::DependencyCycle; return false; }
    return true;
}

bool PromptToolGraph::IsSafeIdentifier(const std::string& value, size_t minLength, size_t maxLength) {
    if (value.size() < minLength || value.size() > maxLength) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; });
}

bool PromptToolGraph::IsTopologicallyOrdered(const PromptToolPlan& plan) {
    for (size_t index = 0; index < plan.nodes.size(); ++index) {
        for (const std::string& dependency : plan.nodes[index].dependencies) {
            const auto found = std::find_if(plan.nodes.begin(), plan.nodes.end(), [&dependency](const PromptToolNode& candidate) { return candidate.nodeId == dependency; });
            if (found == plan.nodes.end() || static_cast<size_t>(std::distance(plan.nodes.begin(), found)) >= index) return false;
        }
    }
    return true;
}

} // namespace NeoEngine
