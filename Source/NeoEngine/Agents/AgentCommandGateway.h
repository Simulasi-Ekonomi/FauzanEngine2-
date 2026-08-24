#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

namespace NeoEngine {

enum class AgentIdentity : uint8_t { CobaAuditor, AriesCreator };
enum class AgentCommandKind : uint8_t { AuditRuntime, CreateGameTemplate, RequestBuild, RequestTest, RequestRollback, MutateRuntime, MutateEconomy };
enum class AgentDecision : uint8_t { Rejected, DryRunPrepared, ApprovalRequired, PlanIssued };

struct AgentCommand {
    AgentIdentity agent = AgentIdentity::CobaAuditor;
    AgentCommandKind kind = AgentCommandKind::AuditRuntime;
    std::string requestId;
    std::string target;
    bool dryRun = true;
};

struct ApprovalEvidence {
    bool buildPassed = false;
    bool testsPassed = false;
    std::string evidenceRef;
};

struct AgentReceipt {
    AgentDecision decision = AgentDecision::Rejected;
    std::string reason;
    std::string planRef;
};

class AgentCommandGateway {
public:
    AgentReceipt Evaluate(const AgentCommand& command) const;
    AgentReceipt ApproveAndIssue(const AgentCommand& command, const ApprovalEvidence& evidence);

private:
    static bool IsSafeIdentifier(const std::string& value, size_t minLength, size_t maxLength);
    static bool IsAuthorityCommand(AgentCommandKind kind);
    static bool IsAllowedFor(AgentIdentity agent, AgentCommandKind kind);
    std::unordered_set<std::string> m_IssuedRequests;
};

} // namespace NeoEngine
