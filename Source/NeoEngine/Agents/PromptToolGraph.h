#pragma once

#include "AgentCommandGateway.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class PromptToolGraphError : uint8_t { None, InvalidPlan, NodeCapacity, DependencyCapacity, DuplicateNode, UnknownDependency, DependencyCycle, GatewayRejected, ApprovalEvidenceMissing };

struct PromptToolNode {
    std::string nodeId;
    AgentCommand command;
    std::vector<std::string> dependencies;
};

struct PromptToolPlan {
    std::string promptId;
    std::string summary;
    bool dryRun = true;
    std::vector<std::string> documentDigests;
    std::vector<PromptToolNode> nodes;
};

struct PromptToolNodeReceipt {
    std::string nodeId;
    AgentReceipt receipt;
};

class PromptToolGraph {
public:
    static constexpr uint16_t kMaxNodes = 16;
    static constexpr uint16_t kMaxDependenciesPerNode = 8;
    static constexpr uint16_t kMaxDocumentDigests = 8;
    static constexpr size_t kMaxSummaryLength = 2048;

    bool EvaluateDryRun(const PromptToolPlan& plan, const AgentCommandGateway& gateway, std::vector<PromptToolNodeReceipt>& receipts);
    bool ApproveAndIssue(const PromptToolPlan& plan, AgentCommandGateway& gateway, const std::vector<ApprovalEvidence>& evidence, std::vector<PromptToolNodeReceipt>& receipts);
    [[nodiscard]] PromptToolGraphError LastError() const { return lastError_; }

private:
    bool Validate(const PromptToolPlan& plan, bool requireDryRun);
    static bool IsSafeIdentifier(const std::string& value, size_t minLength, size_t maxLength);
    static bool IsTopologicallyOrdered(const PromptToolPlan& plan);

    PromptToolGraphError lastError_ = PromptToolGraphError::None;
};

} // namespace NeoEngine
