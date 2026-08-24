#pragma once

#include "AgentCommandGateway.h"

#include <cstdint>
#include <string>

namespace NeoEngine {

enum class ManualRepairScope : uint8_t { Navigation, WorldAuthoring, RuntimeIntegration, RegressionOnly };
enum class ManualRepairError : uint8_t { None, NotExplicitUserCommand, InvalidCommandId, InvalidScope, UnsafeRequest, ProposalCapacity };
struct ManualRepairRequest { AgentIdentity agent = AgentIdentity::CobaAuditor; std::string commandId; ManualRepairScope scope = ManualRepairScope::RegressionOnly; std::string symptom; bool explicitUserCommand = false; };
struct ManualRepairProposal { std::string diagnosisKey; std::string patchScopeKey; std::string regressionTarget; bool requiresHumanReview = true; bool executionForbidden = true; };

class AgentManualRepairProtocol {
public:
    bool Propose(const ManualRepairRequest& request, ManualRepairProposal& proposal);
    [[nodiscard]] ManualRepairError LastError() const { return lastError_; }
private:
    bool Fail(ManualRepairError error); bool ValidId(const std::string& value) const; bool IsUnsafe(const std::string& symptom) const;
    ManualRepairError lastError_ = ManualRepairError::None;
};

} // namespace NeoEngine
