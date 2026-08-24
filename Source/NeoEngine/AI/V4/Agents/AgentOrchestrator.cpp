#include "AgentOrchestrator.h"
#include <algorithm>

namespace NeoEngine {

AgentOrchestrator::AgentOrchestrator() {}
AgentOrchestrator::~AgentOrchestrator() {}

void AgentOrchestrator::AddAgent(uint32_t id, const std::string& role) {
    agents_.push_back({id, role});
}

void AgentOrchestrator::RemoveAgent(uint32_t id) {
    agents_.erase(
        std::remove_if(agents_.begin(), agents_.end(),
            [id](const AgentInfo& info) { return info.id == id; }),
        agents_.end());
}

void AgentOrchestrator::SubmitCommand(const std::string& role, const AgentCommand& cmd) {
    pendingCommands_.push_back(cmd);
}

void AgentOrchestrator::ProcessCommands(std::function<void(uint32_t, const std::string&)> executor) {
    for (auto& cmd : pendingCommands_) {
        executor(cmd.agentID, cmd.action);
    }
    pendingCommands_.clear();
}

size_t AgentOrchestrator::GetAgentCount() const {
    return agents_.size();
}

} // namespace
